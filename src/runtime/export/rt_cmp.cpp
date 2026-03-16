#include "rt_common.hpp"

#include "runtime/PyBool.hpp"

// =============================================================================
// Tier 2: 比较操作
// =============================================================================

PYLANG_EXPORT_CMP("compare_eq", "obj", "obj,obj")
py::PyObject *rt_compare_eq(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->richcompare(rhs, py::RichCompare::Py_EQ));
}

PYLANG_EXPORT_CMP("compare_ne", "obj", "obj,obj")
py::PyObject *rt_compare_ne(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->richcompare(rhs, py::RichCompare::Py_NE));
}

PYLANG_EXPORT_CMP("compare_lt", "obj", "obj,obj")
py::PyObject *rt_compare_lt(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->richcompare(rhs, py::RichCompare::Py_LT));
}

PYLANG_EXPORT_CMP("compare_le", "obj", "obj,obj")
py::PyObject *rt_compare_le(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->richcompare(rhs, py::RichCompare::Py_LE));
}

PYLANG_EXPORT_CMP("compare_gt", "obj", "obj,obj")
py::PyObject *rt_compare_gt(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->richcompare(rhs, py::RichCompare::Py_GT));
}

PYLANG_EXPORT_CMP("compare_ge", "obj", "obj,obj")
py::PyObject *rt_compare_ge(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->richcompare(rhs, py::RichCompare::Py_GE));
}

PYLANG_EXPORT_CMP("compare_is", "obj", "obj,obj")
py::PyObject *rt_compare_is(py::PyObject *lhs, py::PyObject *rhs)
{
	return lhs == rhs ? py::py_true() : py::py_false();
}

PYLANG_EXPORT_CMP("compare_is_not", "obj", "obj,obj")
py::PyObject *rt_compare_is_not(py::PyObject *lhs, py::PyObject *rhs)
{
	return lhs != rhs ? py::py_true() : py::py_false();
}

PYLANG_EXPORT_CMP("compare_in", "obj", "obj,obj")
py::PyObject *rt_compare_in(py::PyObject *value, py::PyObject *container)
{
	// spdlog::error("rt_compare_in: value type = {}, container type = {}",
	// 	value->type()->name(),
	// 	container->type()->name());
	bool result = rt_unwrap(container->contains(value));
	return result ? py::py_true() : py::py_false();
}

PYLANG_EXPORT_CMP("compare_not_in", "obj", "obj,obj")
py::PyObject *rt_compare_not_in(py::PyObject *value, py::PyObject *container)
{
	bool result = rt_unwrap(container->contains(value));
	return result ? py::py_false() : py::py_true();
}