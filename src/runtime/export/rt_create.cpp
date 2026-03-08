#include "rt_common.hpp"

#include "runtime/PyFloat.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"

// =============================================================================
// Tier 0: 对象创建
// =============================================================================

PYLANG_EXPORT_CREATE("string_from_cstr", "obj", "str,i64")
py::PyObject *rt_string_from_cstr(const char *data, int64_t length)
{
	std::string str(data, static_cast<size_t>(length));
	return rt_unwrap(py::PyString::create(str));
}

PYLANG_EXPORT_CREATE("build_tuple", "obj", "i32,ptr")
py::PyObject *rt_build_tuple(int32_t count, py::PyObject **items)
{
	std::vector<py::Value> elements;
	elements.reserve(static_cast<size_t>(count));
	for (int32_t i = 0; i < count; ++i) { elements.push_back(items[i]); }
	return rt_unwrap(py::PyTuple::create(elements));
}

// =============================================================================
// Tier 1: 更多对象创建
// =============================================================================

PYLANG_EXPORT_CREATE("integer_from_i64", "obj", "i64")
py::PyObject *rt_integer_from_i64(int64_t value) { return rt_unwrap(py::PyInteger::create(value)); }

PYLANG_EXPORT_CREATE("float_from_f64", "obj", "f64")
py::PyObject *rt_float_from_f64(double value) { return rt_unwrap(py::PyFloat::create(value)); }

PYLANG_EXPORT_CREATE("build_list", "obj", "i32,ptr")
py::PyObject *rt_build_list(int32_t count, py::PyObject **items)
{
	std::vector<py::Value> elements;
	elements.reserve(static_cast<size_t>(count));
	for (int32_t i = 0; i < count; ++i) { elements.push_back(items[i]); }
	return rt_unwrap(py::PyList::create(elements));
}