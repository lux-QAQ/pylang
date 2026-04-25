#include "../rt_common.hpp"
#include "rt_cache.hpp"
#include "runtime/export/export.hpp"
#include "runtime/shape/Shape.hpp"

#include "runtime/AttributeCache.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"
#include "runtime/types/api.hpp"
#include "runtime/types/builtin.hpp"

#include <alloca.h>
#include <span>

// 声明旧路径以供退回保护使用，如果 rt_func.cpp 未公开则可以直接使用 extern
py::PyObject *rt_call_method_raw_ptrs(py::PyObject *owner,
	const char *method_name,
	py::PyObject **args,
	int32_t argc,
	py::PyObject *kwargs_dict);

void rt_list_append(py::PyObject *list, py::PyObject *value);
void rt_set_add(py::PyObject *set, py::PyObject *value);

namespace py {
namespace cache {

	// 内部缓更新逻辑，支持填充 PIC 槽位，并涵盖可调用的类属性
	static PyObject *
		update_method_cache(MethodCache *cache, PyObject *owner, const char *method_name_cstr)
	{
		auto *b_owner = py::ensure_box(owner);
		auto *attr_name = py::PyString::intern(method_name_cstr);
		auto *type = b_owner->type();

		auto descriptor_ = type->lookup(attr_name);

		if (descriptor_.has_value() && descriptor_->is_ok()) {
			auto *desc = descriptor_->unwrap();

			// 支持的直调类型：function, native_function, method_wrapper, slot_wrapper
			bool is_function = (desc->type() == py::types::function()
								|| desc->type() == py::types::native_function());

			// [性能优化] 新增：支持 method_wrapper (PyMethodDescriptor) 和 slot_wrapper
			// 的直接缓存。这些描述符虽然实现了 __get__，但我们在调用时直接用
			// call_fast_ptrs(args_with_self) 绕过 __get__，避免大量
			// PyNativeFunction 和 PyTuple 分配。
			bool is_descriptor_direct_callable = (desc->type() == py::types::method_wrapper()
												  || desc->type() == py::types::slot_wrapper());

			// 对于 non_data_descriptor_callable 检测：对象是否实现了 __get__
			// （没实现的话就是普通属性调用）
			bool is_non_data_descriptor_callable = !py::descriptor_is_data(desc);
			if (is_non_data_descriptor_callable && !is_function && !is_descriptor_direct_callable) {
				auto type_get = desc->type()->lookup(py::PyString::intern("__get__"));
				if (type_get.has_value() && type_get->is_ok()) {
					is_non_data_descriptor_callable = false;
				}
			}

			if (is_function || is_descriptor_direct_callable || is_non_data_descriptor_callable) {
				// tag=1: needs self prepended
				// (function/native_function/method_wrapper/slot_wrapper) tag=2: no self (普通
				// callable 属性)
				uint8_t tag = (is_function || is_descriptor_direct_callable) ? 1 : 2;

				// 循环淘汰算法选择槽位
				uint8_t slot_idx = cache->next_evict_slot.fetch_add(1, std::memory_order_relaxed)
								   % PYLANG_METHOD_CACHE_SLOTS;
				auto &slot = cache->slots[slot_idx];

				// 更新缓存槽位记录
				slot.expected_type.store(type, std::memory_order_release);
				slot.type_version.store(py::PyType::global_version(), std::memory_order_release);
				slot.expected_shape.store(b_owner->shape(), std::memory_order_release);
				slot.attr_name.store(attr_name, std::memory_order_release);
				slot.resolved_func.store(desc, std::memory_order_release);
				slot.tag.store(tag, std::memory_order_release);

				return desc;
			}
		}

		return nullptr;
	}

}// namespace cache
}// namespace py

// =============================================================================
// Export Tier: Polymorphic Inline Cache for Method Calls
// =============================================================================

// 为了不在此处与 IR 硬编码写死 MethodCache 的内存大小
// 我们在 runtime 里导出一个模板实例，并在 LLVM IR 层读取出其真实 StructType 尺寸和布局
extern "C" py::cache::MethodCache pylang_method_cache_template;
py::cache::MethodCache pylang_method_cache_template{};

PYLANG_EXPORT_FUNC("call_method_ic_ptrs", "obj", "ptr,obj,str,ptr,i32,obj")
py::PyObject *rt_call_method_ic_ptrs(py::cache::MethodCache *cache,
	py::PyObject *owner,
	const char *method_name,
	py::PyObject **args,
	int32_t argc,
	py::PyObject *kwargs_dict)
{
	auto *b_owner = py::ensure_box(owner);

	// ==== 内置方法 Intrinsic fast path ====
	if (!kwargs_dict || kwargs_dict == py::py_none()) {
		if (argc == 1) {
			if (b_owner->type() == py::types::list() && method_name[0] == 'a'
				&& std::strcmp(method_name, "append") == 0) {
				rt_list_append(owner, args[0]);
				return py::py_none();
			}
			if (b_owner->type() == py::types::set() && method_name[0] == 'a'
				&& std::strcmp(method_name, "add") == 0) {
				rt_set_add(owner, args[0]);
				return py::py_none();
			}
		}

		// [性能优化] dict 方法 Intrinsic Fast Path
		if (b_owner->type() == py::types::dict()) {
			auto *dict = static_cast<py::PyDict *>(b_owner);
			if (method_name[0] == 'g' && std::strcmp(method_name, "get") == 0) {
				if (argc == 1) {
					auto *key = py::ensure_box(args[0]);
					return rt_unwrap(dict->get(key, nullptr));
				}
				if (argc == 2) {
					auto *key = py::ensure_box(args[0]);
					auto *def = py::ensure_box(args[1]);
					return rt_unwrap(dict->get(key, def));
				}
			}
			if (argc == 0 && method_name[0] == 'i' && std::strcmp(method_name, "items") == 0) {
				return rt_unwrap(py::PyDictItems::create(*dict));
			}
			if (argc == 0 && method_name[0] == 'k' && std::strcmp(method_name, "keys") == 0) {
				return rt_unwrap(dict->keys());
			}
			if (argc == 0 && method_name[0] == 'v' && std::strcmp(method_name, "values") == 0) {
				return rt_unwrap(dict->values());
			}
		}

		// [性能优化] list 方法 Intrinsic Fast Path
		if (b_owner->type() == py::types::list()) {
			auto *list = static_cast<py::PyList *>(b_owner);
			if (argc == 0 && method_name[0] == 's' && std::strcmp(method_name, "sort") == 0) {
				rt_unwrap(list->sort(nullptr, nullptr));
				return py::py_none();
			}
			if (argc == 0 && method_name[0] == 'p' && std::strcmp(method_name, "pop") == 0) {
				return rt_unwrap(list->pop(nullptr));
			}
			if (argc == 2 && method_name[0] == 'i' && std::strcmp(method_name, "insert") == 0) {
				auto *value = py::ensure_box(args[1]);
				auto idx_rt = py::RtValue::from_ptr(args[0]);
				long index = 0;
				if (idx_rt.is_tagged_int()) {
					index = idx_rt.as_int();
				} else {
					auto *idx_int = py::as<py::PyInteger>(idx_rt.box());
					if (!idx_int) goto skip_list_insert_ic;
					index = idx_int->as_big_int().get_si();
				}
				{
					auto &elems = list->elements();
					long sz = static_cast<long>(elems.size());
					if (index < 0) {
						index += sz;
						if (index < 0) index = 0;
					}
					if (index > sz) { index = sz; }
					elems.insert(elems.begin() + index, value);
					return py::py_none();
				}
			skip_list_insert_ic:;
			}
		}
	}

	auto *actual_type = b_owner->type();
	auto *actual_shape = b_owner->shape();
	const auto actual_type_version = py::PyType::global_version();

	// ==== 1. 读取且比对缓存 (乐观锁加载, PIC) ====
	for (int i = 0; i < PYLANG_METHOD_CACHE_SLOTS; ++i) {
		auto &slot = cache->slots[i];
		uint8_t tag = slot.tag.load(std::memory_order_acquire);

		if (tag != 0) {
			auto *expected_type = slot.expected_type.load(std::memory_order_relaxed);
			auto *expected_shape = slot.expected_shape.load(std::memory_order_relaxed);
			auto type_version = slot.type_version.load(std::memory_order_relaxed);

			// 类型以及全局类成员没有变动
			if (expected_type == actual_type && expected_shape == actual_shape
				&& type_version == actual_type_version) {
				// 缓存命中! 提取已经解析的 callable

				py::PyDict *kwargs = nullptr;
				if (kwargs_dict && kwargs_dict != py::py_none()) {
					kwargs = py::as<py::PyDict>(kwargs_dict);
				}

				bool needs_self = (tag == 1);
				size_t total_argc = needs_self ? argc + 1 : argc;
				py::PyObject *raw_args_array[16];
				py::PyObject **final_args = args;

				if (needs_self) {
					if (total_argc <= 16) {
						final_args = raw_args_array;
					} else {
						final_args = static_cast<py::PyObject **>(
							alloca(sizeof(py::PyObject *) * total_argc));
					}
					final_args[0] = b_owner;
					for (int j = 0; j < argc; ++j) { final_args[j + 1] = args[j]; }
				}

				auto *resolved_func = slot.resolved_func.load(std::memory_order_relaxed);

				// [性能优化] AOT 直接分发: 从 resolved_func 提取 AOT 指针
				// 不改变结构体大小，避免 LLVM IR 内存布局不匹配
				if (needs_self && resolved_func->type() == py::types::native_function()) {
					auto *nf = static_cast<py::PyNativeFunction *>(resolved_func);
					auto *aot = nf->aot_raw_ptr();
					if (aot) {
						using AOTFnPtr = py::PyObject *(*)(py::PyObject *,
							py::PyObject *,
							py::PyObject **,
							int32_t,
							py::PyDict *);
						auto *res = reinterpret_cast<AOTFnPtr>(aot)(nf->module_ref(),
							reinterpret_cast<py::PyObject *>(nf->closure()),
							final_args,
							static_cast<int32_t>(total_argc),
							kwargs);
						if (__builtin_expect(res != nullptr, 1)) return res;
						// AOT 返回 null 表示异常，propagate null
						return nullptr;
					}
				}

				auto result = resolved_func->call_fast_ptrs(final_args, total_argc, kwargs);
				return rt_unwrap(result);
			}
		}
	}

	// [性能优化] 检测自定义 __getattribute__
	// 缓存命中已由 type_version/shape 守卫保护；只有 miss 时才需要检查完整语义边界。
	{
		const auto &ga = actual_type->underlying_type().__getattribute__;
		if (ga.has_value()) {
			if (std::holds_alternative<py::PyObject *>(*ga) || actual_type == py::types::type()) {
				return rt_call_method_raw_ptrs(owner, method_name, args, argc, kwargs_dict);
			}
		}
	}

	// ==== 2. 未命中或缓存失效：调用维护逻辑 ====
	py::cache::update_method_cache(cache, b_owner, method_name);

	// Fallback: 第一次执行或失效时，使用完整开销原本的解释派法
	return rt_call_method_raw_ptrs(owner, method_name, args, argc, kwargs_dict);
}
