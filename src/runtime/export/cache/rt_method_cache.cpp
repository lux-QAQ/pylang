#include "../rt_common.hpp"
#include "rt_cache.hpp"
#include "runtime/export/export.hpp"

#include "runtime/AttributeCache.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFunction.hpp"
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

			// 第三种支持：包含函数、native函数、以及没有 __get__ 的普通 Callable
			bool is_function = (desc->type() == py::types::function()
								|| desc->type() == py::types::native_function());

			// 对于 non_data_descriptor_callable 检测：对象是否实现了 __get__
			// （没实现的话就是普通属性调用）
			bool is_non_data_descriptor_callable = !py::descriptor_is_data(desc);
			if (is_non_data_descriptor_callable && !is_function) {
				auto type_get = desc->type()->lookup(py::PyString::intern("__get__"));
				if (type_get.has_value() && type_get->is_ok()) {
					is_non_data_descriptor_callable = false;
				}
			}

			if (is_function || is_non_data_descriptor_callable) {
				uint8_t tag = is_function ? 1 : 2;

				// 循环淘汰算法选择槽位
				uint8_t slot_idx = cache->next_evict_slot.fetch_add(1, std::memory_order_relaxed)
								   % PYLANG_METHOD_CACHE_SLOTS;
				auto &slot = cache->slots[slot_idx];

				// 更新缓存槽位记录
				slot.expected_type.store(type, std::memory_order_release);
				slot.type_version.store(py::PyType::global_version(), std::memory_order_release);
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
	// ==== 内置方法 Intrinsic fast path 原封不动复制保留 ====
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
	}

	auto *actual_type = b_owner->type();

	// ==== 1. 读取且比对缓存 (乐观锁加载, PIC) ====
	for (int i = 0; i < PYLANG_METHOD_CACHE_SLOTS; ++i) {
		auto &slot = cache->slots[i];
		uint8_t tag = slot.tag.load(std::memory_order_acquire);

		if (tag != 0) {
			auto *expected_type = slot.expected_type.load(std::memory_order_acquire);
			auto type_version = slot.type_version.load(std::memory_order_acquire);

			// 类型以及全局类成员没有变动
			if (expected_type == actual_type && type_version == py::PyType::global_version()) {
				// 获取 intern 后的缓存属性名
				auto *interned_name = slot.attr_name.load(std::memory_order_acquire);

				// O(1) 检测：如果用户对象包含 __dict__ 字典，确保字典里没有发生同名猴子补丁
				if (auto *attrs = b_owner->attributes()) {
					auto &map = attrs->map();
					if (map.find(py::Value(interned_name)) != map.end()) {
						goto NEXT_SLOT;// 发生遮蔽，直接放弃当前槽位，跳转下一个或者慢路径
					}
				}

				// 缓存命中! 提取已经解析的 callable
				auto *resolved_func = slot.resolved_func.load(std::memory_order_acquire);
				bool needs_self = (tag == 1);

				py::PyDict *kwargs = nullptr;
				if (kwargs_dict && kwargs_dict != py::py_none()) {
					kwargs = py::as<py::PyDict>(kwargs_dict);
				}

				size_t total_argc = needs_self ? argc + 1 : argc;
				auto *value_array =
					static_cast<py::Value *>(alloca(sizeof(py::Value) * total_argc));

				if (needs_self) {
					new (&value_array[0]) py::Value(b_owner);
					for (int i = 0; i < argc; ++i) {
						new (&value_array[i + 1]) py::Value(py::ensure_box(args[i]));
					}
				} else {
					for (int i = 0; i < argc; ++i) {
						new (&value_array[i]) py::Value(py::ensure_box(args[i]));
					}
				}

				auto result =
					resolved_func->call_raw(std::span<py::Value>(value_array, total_argc), kwargs);

				for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&value_array[i]);
				return rt_unwrap(result);
			}
		}
	NEXT_SLOT:;
	}

	// ==== 2. 未命中或缓存失效：调用维护逻辑 ====
	py::cache::update_method_cache(cache, b_owner, method_name);

	// Fallback: 第一次执行或失效时，使用完整开销原本的解释派法
	return rt_call_method_raw_ptrs(owner, method_name, args, argc, kwargs_dict);
}
