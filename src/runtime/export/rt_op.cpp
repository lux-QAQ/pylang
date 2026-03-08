#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyNone.hpp"

// =============================================================================
// Tier 1: 二元运算
// =============================================================================

PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
py::PyObject *rt_binary_add(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->add(rhs));
}

PYLANG_EXPORT_OP("binary_sub", "obj", "obj,obj")
py::PyObject *rt_binary_sub(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->subtract(rhs));
}

PYLANG_EXPORT_OP("binary_mul", "obj", "obj,obj")
py::PyObject *rt_binary_mul(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->multiply(rhs));
}

PYLANG_EXPORT_OP("binary_truediv", "obj", "obj,obj")
py::PyObject *rt_binary_truediv(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->truediv(rhs));
}

PYLANG_EXPORT_OP("binary_floordiv", "obj", "obj,obj")
py::PyObject *rt_binary_floordiv(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->floordiv(rhs));
}

PYLANG_EXPORT_OP("binary_mod", "obj", "obj,obj")
py::PyObject *rt_binary_mod(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->modulo(rhs));
}

PYLANG_EXPORT_OP("binary_pow", "obj", "obj,obj")
py::PyObject *rt_binary_pow(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->pow(rhs, py::py_none()));
}

PYLANG_EXPORT_OP("binary_lshift", "obj", "obj,obj")
py::PyObject *rt_binary_lshift(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->lshift(rhs));
}

PYLANG_EXPORT_OP("binary_rshift", "obj", "obj,obj")
py::PyObject *rt_binary_rshift(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->rshift(rhs));
}

PYLANG_EXPORT_OP("binary_and", "obj", "obj,obj")
py::PyObject *rt_binary_and(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->and_(rhs));
}

PYLANG_EXPORT_OP("binary_or", "obj", "obj,obj")
py::PyObject *rt_binary_or(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->or_(rhs));
}

PYLANG_EXPORT_OP("binary_xor", "obj", "obj,obj")
py::PyObject *rt_binary_xor(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(lhs->xor_(rhs));
}

// =============================================================================
// Tier 1: 一元运算
// =============================================================================

PYLANG_EXPORT_OP("unary_neg", "obj", "obj")
py::PyObject *rt_unary_neg(py::PyObject *obj) { return rt_unwrap(obj->neg()); }

PYLANG_EXPORT_OP("unary_pos", "obj", "obj")
py::PyObject *rt_unary_pos(py::PyObject *obj) { return rt_unwrap(obj->pos()); }

PYLANG_EXPORT_OP("unary_invert", "obj", "obj")
py::PyObject *rt_unary_invert(py::PyObject *obj) { return rt_unwrap(obj->invert()); }

PYLANG_EXPORT_OP("unary_not", "obj", "obj")
py::PyObject *rt_unary_not(py::PyObject *obj)
{
	bool is_true = rt_unwrap(obj->true_());
	return is_true ? py::py_false() : py::py_true();
}