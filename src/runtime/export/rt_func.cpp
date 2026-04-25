#include "rt_common.hpp"
#include "runtime/shape/Shape.hpp"

#include "runtime/IndexError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyCell.hpp"
#include "runtime/PyClassMethod.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyStaticMethod.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"
#include "runtime/types/api.hpp"
#include "runtime/types/builtin.hpp"
#include <alloca.h>
#include <span>

py::PyObject *rt_getattr(py::PyObject *obj, const char *name);
void rt_list_append(py::PyObject *list, py::PyObject *value);
// 声明外部专用的 fast path 函数
void rt_set_add(py::PyObject *set, py::PyObject *value);

// =============================================================================
// Tier 0: 函数调用
// =============================================================================

PYLANG_EXPORT_FUNC("call", "obj", "obj,obj,obj")
py::PyObject *rt_call(py::PyObject *callable, py::PyObject *args, py::PyObject *kwargs)
{
	auto *tuple_args = static_cast<py::PyTuple *>(py::ensure_box(args));
	auto *dict_kwargs = kwargs ? static_cast<py::PyDict *>(py::ensure_box(kwargs)) : nullptr;
	return rt_unwrap(py::ensure_box(callable)->call(tuple_args, dict_kwargs));
}

// =============================================================================
// Tier 4: 方法调用
// =============================================================================

PYLANG_EXPORT_FUNC("load_method", "obj", "obj,str")
py::PyObject *rt_load_method(py::PyObject *obj, const char *name)
{
	auto *b_obj = py::ensure_box(obj);
	auto *cached = py::MethodCache::load_method(b_obj, name);
	if (!cached) {
		// 回退抛出原本由于找不到属性导致的抛错
		rt_raise(b_obj->get_method(py::PyString::intern(name)).unwrap_err());
	}
	return cached;
}

// =============================================================================
// Tier 4: tuple 特化访问（闭包实现需要）
// =============================================================================

PYLANG_EXPORT_SUBSCR("tuple_getitem", "obj", "obj,i32")
py::PyObject *rt_tuple_getitem(py::PyObject *tuple, int32_t index)
{
	auto *t = static_cast<py::PyTuple *>(py::ensure_box(tuple));
	return rt_unwrap(t->__getitem__(static_cast<int64_t>(index)));
}

// =============================================================================
// Tier 4: 闭包操作
// =============================================================================

PYLANG_EXPORT_FUNC("create_cell", "obj", "obj")
py::PyObject *rt_create_cell(py::PyObject *value)
{
	if (value) { return rt_unwrap(py::PyCell::create(py::Value{ py::ensure_box(value) })); }
	return rt_unwrap(py::PyCell::create());
}

PYLANG_EXPORT_FUNC("cell_get", "obj", "obj")
py::PyObject *rt_cell_get(py::PyObject *cell)
{
	auto *cell_obj = static_cast<py::PyCell *>(py::ensure_box(cell));
	if (cell_obj->empty()) {
		rt_raise(py::name_error("free variable referenced before assignment in enclosing scope"));
	}
	return rt_unwrap(py::PyObject::from(cell_obj->content()));
}

PYLANG_EXPORT_FUNC("cell_set", "void", "obj,obj")
void rt_cell_set(py::PyObject *cell, py::PyObject *value)
{
	auto *cell_obj = static_cast<py::PyCell *>(py::ensure_box(cell));
	cell_obj->set_cell(py::Value{ py::ensure_box(value) });
}

// =============================================================================
// Phase 4+: 快速调用路径
// =============================================================================

PYLANG_EXPORT_FUNC("call_fast", "obj", "obj,i32,ptr")
py::PyObject *rt_call_fast(py::PyObject *callable, int32_t argc, py::PyObject **argv)
{
	// 抛弃旧版本的 PyTuple 生成与 variant 装箱包装
	// 全面采用 AOT C ABI 零分配指针穿透协议
	auto result = py::ensure_box(callable)->call_fast_ptrs(argv, argc, nullptr);
	return rt_unwrap(result);
}

// =============================================================================
// Tier 4: make_function
// =============================================================================
PYLANG_EXPORT_FUNC("value_array_get", "obj", "ptr,i32")
py::PyObject *rt_value_array_get(py::PyObject **args, int32_t index) { return args[index]; }


PYLANG_EXPORT_FUNC("make_function", "obj", "str,ptr,obj,obj,obj,obj")
py::PyObject *rt_make_function(const char *name,
	void *code_ptr,
	py::PyObject *module,
	py::PyObject *defaults,
	py::PyObject *kwdefaults,
	py::PyObject *closure)
{
	return rt_unwrap(py::PyNativeFunction::create_aot(std::string(name),
		code_ptr,
		py::ensure_box(module),
		py::ensure_box(defaults),
		py::ensure_box(kwdefaults),
		py::ensure_box(closure)));
}

// =============================================================================
// Tier 4: get_closure
// =============================================================================

PYLANG_EXPORT_FUNC("get_closure", "obj", "obj")
py::PyObject *rt_get_closure(py::PyObject *func)
{
	auto b_func = py::ensure_box(func);
	if (auto *pyfunc = py::as<py::PyFunction>(b_func)) {
		auto *closure = pyfunc->closure();
		return closure ? static_cast<py::PyObject *>(closure) : rt_unwrap(py::PyTuple::create());
	}

	if (auto *native = py::as<py::PyNativeFunction>(b_func)) {
		auto *closure = native->closure();
		return closure ? static_cast<py::PyObject *>(closure) : rt_unwrap(py::PyTuple::create());
	}

	return rt_unwrap(py::PyTuple::create());
}

// PYLANG_EXPORT_FUNC("rt_call_method_raw", "obj", "obj,str,i32,ptr")
// py::PyObject *
// 	rt_call_method_raw(py::PyObject *obj, const char *name, int32_t argc, py::PyObject **argv)
// {
// 	auto *b_obj = py::ensure_box(obj);
// 	auto *attr_name = py::PyString::intern(name);
// 	auto *type = b_obj->type();

// 	py::PyObject *self = nullptr;
// 	py::PyObject *method = nullptr;

// 	// 3.9 语义查找
// 	auto descr_lookup = type->lookup(attr_name);
// 	py::PyObject *descr =
// 		(descr_lookup.has_value() && descr_lookup->is_ok()) ? descr_lookup->unwrap() : nullptr;

// 	if (descr) {
// 		// 如果是数据描述符（如 property），必须走标准 getattr 流程（会创建 BoundMethod）
// 		if (py::descriptor_is_data(descr)) {
// 			method = rt_getattr(b_obj, name);
// 		} else {
// 			// 检查实例字典
// 			if (auto *inst_dict = b_obj->attributes()) {
// 				if (auto it = inst_dict->map().find(attr_name); it != inst_dict->map().end()) {
// 					method = py::PyObject::from(it->second).unwrap();
// 				}
// 			}
// 			// 如果实例没覆盖，且 descr 是函数，命中 Fast Path
// 			if (!method) {
// 				if (descr->type() == py::types::native_function()
// 					|| descr->type() == py::types::function()) {
// 					method = descr;
// 					self = b_obj;// 标记需要传入 self
// 				} else {
// 					method = rt_getattr(b_obj, name);
// 				}
// 			}
// 		}
// 	} else {
// 		method = rt_getattr(b_obj, name);
// 	}

// 	// 修复：将 PyObject** 转换为 Value 数组，并调用 call_raw
// 	size_t total_argc = argc + (self ? 1 : 0);
// 	auto *stack_args = static_cast<py::Value *>(alloca(sizeof(py::Value) * total_argc));

// 	if (self) {
// 		new (&stack_args[0]) py::Value(self);
// 		for (int i = 0; i < argc; ++i) new (&stack_args[i + 1]) py::Value(py::ensure_box(argv[i]));
// 	} else {
// 		for (int i = 0; i < argc; ++i) new (&stack_args[i]) py::Value(py::ensure_box(argv[i]));
// 	}

// 	// 通过虚函数多态性进入 PyNativeFunction 或 PyBoundMethod
// 	auto result = method->call_raw(std::span<py::Value>(stack_args, total_argc), nullptr);

// 	// 析构 Value（清理 tagged 指针不需要，但为了安全一致性）
// 	for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&stack_args[i]);

// 	return rt_unwrap(result);
// }


PYLANG_EXPORT_FUNC("call_raw_ptrs", "obj", "obj,ptr,i32,obj")
py::PyObject *rt_call_raw_ptrs(py::PyObject *callable,
	py::PyObject **args,
	int32_t argc,
	py::PyObject *kwargs_dict)
{
	py::PyDict *kwargs = nullptr;
	if (kwargs_dict && kwargs_dict != py::py_none()) { kwargs = py::as<py::PyDict>(kwargs_dict); }

	// 统一 boxing callable，通过极速无variant通道执行
	auto result = py::ensure_box(callable)->call_fast_ptrs(args, argc, kwargs);

	return rt_unwrap(result);
}


PYLANG_EXPORT_FUNC("call_method_raw_ptrs", "obj", "obj,str,ptr,i32,obj")
py::PyObject *rt_call_method_raw_ptrs(py::PyObject *owner,
	const char *method_name,
	py::PyObject **args,
	int32_t argc,
	py::PyObject *kwargs_dict)
{
	auto *b_owner = py::ensure_box(owner);

	// [性能优化] 检测自定义 __getattribute__
	// 旧方法用 get_address 比较跨 TU 的 std::function 指针地址，不可靠。
	// 新方法：只有 PyObject* variant (Python 层覆盖) 或 type 元类才触发完整 getattribute 路径。
	{
		const auto &ga = b_owner->type()->underlying_type().__getattribute__;
		if (ga.has_value()) {
			if (std::holds_alternative<py::PyObject *>(*ga)
				|| b_owner->type() == py::types::type()) {
				auto *bound = rt_unwrap(b_owner->getattribute(py::PyString::intern(method_name)));
				return rt_call_raw_ptrs(bound, args, argc, kwargs_dict);
			}
		}
	}

	// === 内置方法的 Intrinsic Fast Path ===
	if (!kwargs_dict || kwargs_dict == py::py_none()) {
		if (argc == 1) {
			if (b_owner->type() == py::types::list() && method_name[0] == 'a'
				&& std::strcmp(method_name, "append") == 0) {
				rt_list_append(owner, args[0]);
				return py::py_none();
			}
			// [极致优化]：拦截 set.add，消灭所有隐式 Tuple 和 BoundMethod 分配！
			if (b_owner->type() == py::types::set() && method_name[0] == 'a'
				&& std::strcmp(method_name, "add") == 0) {
				rt_set_add(owner, args[0]);
				return py::py_none();
			}
		}

		// [性能优化] dict 方法的 Intrinsic Fast Path
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

		// [性能优化] list 方法的 Intrinsic Fast Path
		if (b_owner->type() == py::types::list()) {
			auto *list = static_cast<py::PyList *>(b_owner);
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
					if (!idx_int) goto skip_list_insert;
					index = idx_int->as_big_int().get_si();
				}
				{
					list->insert_raw_clamped(index, value);
					return py::py_none();
				}
			skip_list_insert:;
			}
		}
	}

	auto *attr_name = py::PyString::intern(method_name);
	auto *type = b_owner->type();

	auto descriptor_ = type->lookup(attr_name);
	py::PyDict *kwargs = nullptr;
	if (kwargs_dict && kwargs_dict != py::py_none()) { kwargs = py::as<py::PyDict>(kwargs_dict); }

	if (descriptor_.has_value() && descriptor_->is_ok()) {
		auto *desc = descriptor_->unwrap();
		auto *desc_type = desc->type();

		// 0. 特例：函数和原生函数绝不是数据描述符，必须最先拦截，直接走 AOT 零分配极速路径
		if (desc_type == py::types::function() || desc_type == py::types::native_function()) {
			// 先检查实例字典是否覆盖了方法 (Python 语义: 实例属性优先于非数据描述符)
			if (auto *shape = b_owner->shape()) {
				if (auto offset = shape->lookup(attr_name)) {
					auto func = rt_unwrap(py::PyObject::from(b_owner->slots()[*offset]));
					return rt_call_raw_ptrs(func, args, argc, kwargs_dict);
				}
			}

			// AOT 极速直通绑定调用 (彻底剔除 variant)
			size_t total_argc = argc + 1;
			py::PyObject *raw_args_array[16];
			py::PyObject **final_args = args;
			if (total_argc <= 16) {
				final_args = raw_args_array;
			} else {
				final_args =
					static_cast<py::PyObject **>(alloca(sizeof(py::PyObject *) * total_argc));
			}
			final_args[0] = b_owner;
			for (int i = 0; i < argc; ++i) { final_args[i + 1] = args[i]; }

			auto result = desc->call_fast_ptrs(final_args, total_argc, kwargs);
			return rt_unwrap(result);
		}

		// 0.5 非 data descriptor 的 method/slot 直调路径：
		// 避免先 __get__ 生成绑定 wrapper，再掉回通用 call_fast_ptrs fallback。
		if (desc_type == py::types::method_wrapper() || desc_type == py::types::slot_wrapper()) {
			if (auto *shape = b_owner->shape()) {
				if (auto offset = shape->lookup(attr_name)) {
					auto func = rt_unwrap(py::PyObject::from(b_owner->slots()[*offset]));
					return rt_call_raw_ptrs(func, args, argc, kwargs_dict);
				}
			}

			size_t total_argc = argc + 1;
			py::PyObject *raw_args_array[16];
			py::PyObject **final_args = args;
			if (total_argc <= 16) {
				final_args = raw_args_array;
			} else {
				final_args =
					static_cast<py::PyObject **>(alloca(sizeof(py::PyObject *) * total_argc));
			}
			final_args[0] = b_owner;
			for (int i = 0; i < argc; ++i) { final_args[i + 1] = args[i]; }

			auto result = desc->call_fast_ptrs(final_args, total_argc, kwargs);
			return rt_unwrap(result);
		}

		if (desc_type == py::types::classmethod_descriptor()) {
			size_t total_argc = argc + 1;
			py::PyObject *raw_args_array[16];
			py::PyObject **final_args = args;
			if (total_argc <= 16) {
				final_args = raw_args_array;
			} else {
				final_args =
					static_cast<py::PyObject **>(alloca(sizeof(py::PyObject *) * total_argc));
			}
			final_args[0] = b_owner->type();
			for (int i = 0; i < argc; ++i) { final_args[i + 1] = args[i]; }

			auto result = desc->call_fast_ptrs(final_args, total_argc, kwargs);
			return rt_unwrap(result);
		}

		// 0.7 Python @staticmethod 描述符:
		//     staticmethod.__get__ 返回底层函数，调用时不传 self
		if (desc_type == py::types::static_method()) {
			auto *sm = static_cast<py::PyStaticMethod *>(desc);
			auto *func = sm->static_method();
			return rt_call_raw_ptrs(func, args, argc, kwargs_dict);
		}

		// 0.8 Python @classmethod 描述符:
		//     classmethod.__get__ 返回绑定到 cls 的方法
		if (desc_type == py::types::classmethod()) {
			auto *cm = static_cast<py::PyClassMethod *>(desc);
			// 通过 __get__ 获取绑定到 owner 类型的方法
			auto bound = cm->__get__(b_owner, b_owner->type());
			if (bound.is_err()) return rt_unwrap(bound);
			return rt_call_raw_ptrs(bound.unwrap(), args, argc, kwargs_dict);
		}

		// 1. 数据描述符 (例如 @property 或是错误的 hack 产物)
		// 修复：必须获取属性后，将其作为 callable 执行 rt_call_raw_ptrs！
		if (py::descriptor_is_data(desc)) {
			auto bound = rt_unwrap(b_owner->getattribute(attr_name));
			return rt_call_raw_ptrs(bound, args, argc, kwargs_dict);
		}

		// 2. 实例字典 (覆盖类属性)
		if (auto *shape = b_owner->shape()) {
			if (auto offset = shape->lookup(attr_name)) {
				auto func = rt_unwrap(py::PyObject::from(b_owner->slots()[*offset]));
				return rt_call_raw_ptrs(func, args, argc, kwargs_dict);
			}
		}

		// 3. [性能优化] 通用非数据描述符 catch-all: 直接 call_fast_ptrs 带 self
		// 避免走 getattribute → __get__ → PyNativeFunction 分配的昂贵路径
		{
			size_t total_argc = argc + 1;
			py::PyObject *raw_args_array[16];
			py::PyObject **final_args = args;
			if (total_argc <= 16) {
				final_args = raw_args_array;
			} else {
				final_args =
					static_cast<py::PyObject **>(alloca(sizeof(py::PyObject *) * total_argc));
			}
			final_args[0] = b_owner;
			for (int i = 0; i < argc; ++i) { final_args[i + 1] = args[i]; }

			auto result = desc->call_fast_ptrs(final_args, total_argc, kwargs);
			return rt_unwrap(result);
		}
	}

	// Fallback: 回退到完整的 getattribute 流程，然后发起调用
	auto bound = rt_unwrap(b_owner->getattribute(attr_name));
	return rt_call_raw_ptrs(bound, args, argc, kwargs_dict);
}
