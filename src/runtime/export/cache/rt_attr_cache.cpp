#include "rt_attr_cache.hpp"
#include "../rt_common.hpp"

#include "runtime/AttributeCache.hpp"
#include "runtime/PyString.hpp"
#include "runtime/shape/Shape.hpp"
#include "runtime/types/api.hpp"

namespace {

// [性能优化] 使用 variant 检查代替 get_address(std::function::target<>)\n// 见
// uses_default_getattribute / uses_default_setattribute

bool uses_default_getattribute(const py::PyObject *obj)
{
	// [性能优化] 避免 get_address (std::function::target<>) 的高开销（~1.46s）。
	// 只有 Python 层显式 def __getattribute__ 才会将 slot 存为 PyObject* variant。
	// 内建类型（list, dict, str 等）的 __getattribute__ 都是 C++ lambda (function variant)，
	// 无需地址比较即可确认为默认实现。
	const auto &ga = obj->type()->underlying_type().__getattribute__;
	if (!ga.has_value()) return false;
	// 如果 slot 存的是 PyObject*，则是用户自定义的 __getattribute__
	if (std::holds_alternative<py::PyObject *>(*ga)) return false;
	// type 元类自身的 __getattribute__ 有特殊行为
	if (obj->type() == py::types::type()) return false;
	return true;
}

bool uses_default_setattribute(const py::PyObject *obj)
{
	const auto &sa = obj->type()->underlying_type().__setattribute__;
	if (!sa.has_value()) return false;
	if (std::holds_alternative<py::PyObject *>(*sa)) return false;
	if (obj->type() == py::types::type()) return false;
	return true;
}

py::PyObject *
	resolve_default_getattr(py::PyObject *obj, py::PyString *name, py::cache::AttrCache *cache)
{
	auto *type = obj->type();
	const uint64_t type_version = py::PyType::global_version();
	auto *shape = obj->shape();

	auto cached_lookup = py::AttrCache::lookup(type, name->value().c_str(), type_version);
	if (!cached_lookup) {
		auto descriptor_ = type->lookup(name);
		if (descriptor_.has_value()) {
			auto descriptor_result = descriptor_.value();
			if (descriptor_result.is_err()) { rt_raise(descriptor_result.unwrap_err()); }
			cached_lookup = descriptor_result.unwrap();
			py::AttrCache::update(type, name->value().c_str(), cached_lookup, type_version);
		} else {
			py::AttrCache::update(type, name->value().c_str(), PYLANG_ATTR_ABSENT, type_version);
			cached_lookup = PYLANG_ATTR_ABSENT;
		}
	}

	if (cached_lookup != PYLANG_ATTR_ABSENT) {
		auto *descriptor = cached_lookup;
		const bool has_get = descriptor->type()->underlying_type().__get__.has_value();
		if (has_get && py::descriptor_is_data(descriptor->type())) {
			return rt_unwrap(descriptor->get(obj, type));
		}
	}

	if (shape) {
		if (auto offset = shape->lookup(name)) {
			if (cache) {
				cache->expected_type.store(type, std::memory_order_release);
				cache->type_version.store(type_version, std::memory_order_release);
				cache->expected_shape.store(shape, std::memory_order_release);
				cache->slot_offset.store(static_cast<uint32_t>(*offset), std::memory_order_release);
				cache->kind.store(1, std::memory_order_release);
			}
			return obj->slots()[*offset];
		}
	}

	if (cached_lookup != PYLANG_ATTR_ABSENT) {
		auto *descriptor = cached_lookup;
		const bool has_get = descriptor->type()->underlying_type().__get__.has_value();
		if (has_get) { return rt_unwrap(descriptor->get(obj, type)); }
		return descriptor;
	}

	return rt_unwrap(obj->getattribute(name));
}

}// namespace

extern "C" py::cache::AttrCache pylang_attr_cache_template;
py::cache::AttrCache pylang_attr_cache_template{};

PYLANG_EXPORT_ATTR("getattr_ic", "obj", "ptr,obj,obj")
py::PyObject *rt_getattr_ic(py::cache::AttrCache *cache, py::PyObject *obj, py::PyObject *name_obj)
{
	auto *b_obj = py::ensure_box(obj);
	auto *name = static_cast<py::PyString *>(name_obj);
	auto *actual_type = b_obj->type();
	auto *actual_shape = b_obj->shape();
	const auto type_version = py::PyType::global_version();

	// Hot path: this cache state is only installed after resolving through the default
	// getattribute path. A later class mutation must bump the global type version, so a
	// matching version/shape/type guard is enough to reuse the slot directly.
	if (cache) {
		if (cache->kind.load(std::memory_order_acquire) == 1) {
			auto *expected_type = cache->expected_type.load(std::memory_order_relaxed);
			auto *expected_shape = cache->expected_shape.load(std::memory_order_relaxed);
			auto cached_type_version = cache->type_version.load(std::memory_order_relaxed);
			if (expected_type == actual_type && expected_shape == actual_shape
				&& cached_type_version == type_version) {
				auto offset = cache->slot_offset.load(std::memory_order_relaxed);
				return b_obj->slots()[offset];
			}
		}
		cache->kind.store(0, std::memory_order_release);
	}

	if (!uses_default_getattribute(b_obj)) { return rt_unwrap(b_obj->getattribute(name)); }

	return resolve_default_getattr(b_obj, name, cache);
}

PYLANG_EXPORT_ATTR("setattr_ic", "void", "ptr,obj,obj,obj")
void rt_setattr_ic(py::cache::AttrCache *cache,
	py::PyObject *obj,
	py::PyObject *name_obj,
	py::PyObject *value)
{
	auto *b_obj = py::ensure_box(obj);
	auto *name = static_cast<py::PyString *>(name_obj);

	if (!uses_default_setattribute(b_obj)) {
		rt_unwrap_void(b_obj->setattribute(name, py::ensure_box(value)));
		return;
	}

	// 类对象（PyType）的属性存储在 __dict__ 中，不能使用 shape/slots IC
	// 必须走 setattribute 路径以确保 __dict__ 被正确更新
	if (py::as<py::PyType>(b_obj)) {
		rt_unwrap_void(b_obj->setattribute(name, py::ensure_box(value)));
		return;
	}

	if (cache && cache->kind.load(std::memory_order_acquire) == 2) {
		auto *expected_type = cache->expected_type.load(std::memory_order_acquire);
		auto *expected_shape = cache->expected_shape.load(std::memory_order_acquire);
		auto *next_shape = cache->next_shape.load(std::memory_order_acquire);
		auto type_version = cache->type_version.load(std::memory_order_acquire);
		if (expected_type == b_obj->type() && expected_shape == b_obj->shape()
			&& type_version == py::PyType::global_version()) {
			auto offset = static_cast<size_t>(cache->slot_offset.load(std::memory_order_acquire));
			if (next_shape != expected_shape) {
				b_obj->set_shape(next_shape);
				if (b_obj->slots().size() == offset) {
					b_obj->slots().push_back(value);
				} else {
					if (b_obj->slots().size() < offset + 1) { b_obj->slots().resize(offset + 1); }
					b_obj->slots()[offset] = value;
				}
			} else {
				b_obj->slots()[offset] = value;
			}
			return;
		}
		cache->kind.store(0, std::memory_order_release);
	}

	auto descriptor_ = b_obj->type()->lookup(name);
	if (descriptor_.has_value()) {
		auto descriptor_result = descriptor_.value();
		if (descriptor_result.is_err()) {
			rt_raise(descriptor_result.unwrap_err());
			return;
		}
		auto *descriptor = descriptor_result.unwrap();
		if (py::descriptor_is_data(descriptor->type())) {
			rt_unwrap_void(b_obj->setattribute(name, py::ensure_box(value)));
			return;
		}
	}

	auto *current_shape = b_obj->shape();
	if (current_shape) {
		if (auto offset = current_shape->lookup(name)) {
			if (cache) {
				cache->expected_type.store(b_obj->type(), std::memory_order_release);
				cache->type_version.store(py::PyType::global_version(), std::memory_order_release);
				cache->expected_shape.store(current_shape, std::memory_order_release);
				cache->next_shape.store(current_shape, std::memory_order_release);
				cache->slot_offset.store(static_cast<uint32_t>(*offset), std::memory_order_release);
				cache->kind.store(2, std::memory_order_release);
			}
			b_obj->slots()[*offset] = value;
			return;
		}
	}

	auto *next_shape = current_shape ? current_shape->transition(name)
									 : py::Shape::root(b_obj->type())->transition(name);
	auto next_offset = next_shape->lookup(name);
	ASSERT(next_offset.has_value());
	if (cache) {
		cache->expected_type.store(b_obj->type(), std::memory_order_release);
		cache->type_version.store(py::PyType::global_version(), std::memory_order_release);
		cache->expected_shape.store(current_shape, std::memory_order_release);
		cache->next_shape.store(next_shape, std::memory_order_release);
		cache->slot_offset.store(static_cast<uint32_t>(*next_offset), std::memory_order_release);
		cache->kind.store(2, std::memory_order_release);
	}
	b_obj->set_shape(next_shape);
	if (b_obj->slots().size() == *next_offset) {
		b_obj->slots().push_back(value);
	} else {
		if (b_obj->slots().size() < *next_offset + 1) { b_obj->slots().resize(*next_offset + 1); }
		b_obj->slots()[*next_offset] = value;
	}
}
