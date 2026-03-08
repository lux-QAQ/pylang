#include "rt_common.hpp"

#include "runtime/PyDict.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"

// =============================================================================
// Tier 0: 函数调用
// =============================================================================

PYLANG_EXPORT_FUNC("call", "obj", "obj,obj,obj")
py::PyObject *rt_call(py::PyObject *callable, py::PyObject *args, py::PyObject *kwargs)
{
	auto *tuple_args = static_cast<py::PyTuple *>(args);
	auto *dict_kwargs = static_cast<py::PyDict *>(kwargs);// 可能是 nullptr
	return rt_unwrap(callable->call(tuple_args, dict_kwargs));
}

// =============================================================================
// Tier 4: 方法调用
// =============================================================================

PYLANG_EXPORT_FUNC("load_method", "obj", "obj,str")
py::PyObject *rt_load_method(py::PyObject *obj, const char *name)
{
	auto *method_name = rt_unwrap(py::PyString::create(std::string(name)));
	return rt_unwrap(obj->get_method(method_name));
}