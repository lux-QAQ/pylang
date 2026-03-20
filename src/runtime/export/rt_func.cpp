#include "rt_common.hpp"

#include "runtime/IndexError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyCell.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

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
	std::vector<py::Value> elements;
	elements.reserve(argc);
	for (int32_t i = 0; i < argc; ++i) { elements.push_back(py::ensure_box(argv[i])); }
	auto *args = rt_unwrap(py::PyTuple::create(elements));
	return rt_unwrap(py::ensure_box(callable)->call(args, nullptr));
}

// =============================================================================
// Tier 4: make_function
// =============================================================================

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