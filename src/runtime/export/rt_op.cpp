#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/NotImplemented.hpp"

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

// =============================================================================
// Tier 1: 增量赋值（inplace）运算
//
// Python 3.9 语义: x += y
//   1. 查找 type(x).__iadd__
//   2. 若找到，调用 x.__iadd__(y)
//   3. 若返回 NotImplemented 或未找到，退回 x.__add__(y) / y.__radd__(x)
//
// 对不可变类型（int, str, tuple）: __iadd__ 不存在，直接退回 __add__
// 对可变类型（list, dict, set）: __iadd__ 原地修改并返回 self
// 
// 当前inplace在runtime中并未实现,需要TODO =============================================================================

namespace {

/// 尝试调用 __ixxx__ 方法，成功返回结果，失败返回 nullptr
py::PyObject *try_inplace_method(py::PyObject *lhs, py::PyObject *rhs, const char *method_name)
{
	auto name = py::PyString::create(method_name);
	if (name.is_err()) { return nullptr; }

	auto [result, found] = lhs->lookup_attribute(name.unwrap());
	if (found != py::LookupAttrResult::FOUND || result.is_err()) { return nullptr; }

	auto args = py::PyTuple::create(rhs);
	if (args.is_err()) { return nullptr; }

	auto call_result = result.unwrap()->call(args.unwrap(), nullptr);
	if (call_result.is_err()) { return nullptr; }

	auto *ret = call_result.unwrap();
	if (ret == py::not_implemented()) { return nullptr; }

	return ret;
}

}// namespace

PYLANG_EXPORT_OP("inplace_add", "obj", "obj,obj")
py::PyObject *rt_inplace_add(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__iadd__")) { return r; }
	return rt_unwrap(lhs->add(rhs));
}

PYLANG_EXPORT_OP("inplace_sub", "obj", "obj,obj")
py::PyObject *rt_inplace_sub(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__isub__")) { return r; }
	return rt_unwrap(lhs->subtract(rhs));
}

PYLANG_EXPORT_OP("inplace_mul", "obj", "obj,obj")
py::PyObject *rt_inplace_mul(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__imul__")) { return r; }
	return rt_unwrap(lhs->multiply(rhs));
}

PYLANG_EXPORT_OP("inplace_truediv", "obj", "obj,obj")
py::PyObject *rt_inplace_truediv(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__itruediv__")) { return r; }
	return rt_unwrap(lhs->truediv(rhs));
}

PYLANG_EXPORT_OP("inplace_floordiv", "obj", "obj,obj")
py::PyObject *rt_inplace_floordiv(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__ifloordiv__")) { return r; }
	return rt_unwrap(lhs->floordiv(rhs));
}

PYLANG_EXPORT_OP("inplace_mod", "obj", "obj,obj")
py::PyObject *rt_inplace_mod(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__imod__")) { return r; }
	return rt_unwrap(lhs->modulo(rhs));
}

PYLANG_EXPORT_OP("inplace_pow", "obj", "obj,obj")
py::PyObject *rt_inplace_pow(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__ipow__")) { return r; }
	return rt_unwrap(lhs->pow(rhs, py::py_none()));
}

PYLANG_EXPORT_OP("inplace_lshift", "obj", "obj,obj")
py::PyObject *rt_inplace_lshift(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__ilshift__")) { return r; }
	return rt_unwrap(lhs->lshift(rhs));
}

PYLANG_EXPORT_OP("inplace_rshift", "obj", "obj,obj")
py::PyObject *rt_inplace_rshift(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__irshift__")) { return r; }
	return rt_unwrap(lhs->rshift(rhs));
}

PYLANG_EXPORT_OP("inplace_and", "obj", "obj,obj")
py::PyObject *rt_inplace_and(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__iand__")) { return r; }
	return rt_unwrap(lhs->and_(rhs));
}

PYLANG_EXPORT_OP("inplace_or", "obj", "obj,obj")
py::PyObject *rt_inplace_or(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__ior__")) { return r; }
	return rt_unwrap(lhs->or_(rhs));
}

PYLANG_EXPORT_OP("inplace_xor", "obj", "obj,obj")
py::PyObject *rt_inplace_xor(py::PyObject *lhs, py::PyObject *rhs)
{
	if (auto *r = try_inplace_method(lhs, rhs, "__ixor__")) { return r; }
	return rt_unwrap(lhs->xor_(rhs));
}

// 临时可能需要 但是为了确保不在export实现runtime功能这些应该交给runtime模块实现,所以先注释掉
// PYLANG_EXPORT_OP("inplace_add", "obj", "obj,obj")
// py::PyObject *rt_inplace_add(py::PyObject *lhs, py::PyObject *rhs)
// {
//     // 快速路径：list += iterable → list.extend(iterable); return list
//     if (lhs->type() == py::types::list()) {
//         rt_unwrap_void(static_cast<py::PyList *>(lhs)->extend(rhs));
//         return lhs;
//     }

//     // 通用路径：尝试 __iadd__ 协议
//     if (auto *r = try_inplace_method(lhs, rhs, "__iadd__")) { return r; }

//     // 退回普通 __add__
//     return rt_unwrap(lhs->add(rhs));
// }

// PYLANG_EXPORT_OP("inplace_mul", "obj", "obj,obj")
// py::PyObject *rt_inplace_mul(py::PyObject *lhs, py::PyObject *rhs)
// {
//     // 快速路径：list *= n → list 原地重复
//     if (lhs->type() == py::types::list()) {
//         auto seq = lhs->as_sequence();
//         if (seq.is_ok()) {
//             auto result = seq.unwrap().repeat(rhs);
//             if (result.is_ok()) {
//                 // 将 repeat 结果的元素复制回原 list
//                 auto *list = static_cast<py::PyList *>(lhs);
//                 auto *new_list = static_cast<py::PyList *>(result.unwrap());
//                 list->elements() = std::move(new_list->elements());
//                 return lhs;
//             }
//         }
//     }

//     if (auto *r = try_inplace_method(lhs, rhs, "__imul__")) { return r; }
//     return rt_unwrap(lhs->multiply(rhs));
// }

// PYLANG_EXPORT_OP("inplace_or", "obj", "obj,obj")
// py::PyObject *rt_inplace_or(py::PyObject *lhs, py::PyObject *rhs)
// {
//     // 快速路径：dict |= other → dict.update(other); return dict  (PEP 584, Python 3.9+)
//     if (lhs->type() == py::types::dict()) {
//         rt_unwrap_void(static_cast<py::PyDict *>(lhs)->update(rhs));
//         return lhs;
//     }

//     // 快速路径：set |= other → set.update(other); return set
//     if (lhs->type() == py::types::set()) {
//         rt_unwrap_void(static_cast<py::PySet *>(lhs)->update(rhs));
//         return lhs;
//     }

//     if (auto *r = try_inplace_method(lhs, rhs, "__ior__")) { return r; }
//     return rt_unwrap(lhs->or_(rhs));
// }

// PYLANG_EXPORT_OP("inplace_and", "obj", "obj,obj")
// py::PyObject *rt_inplace_and(py::PyObject *lhs, py::PyObject *rhs)
// {
//     // 快速路径：set &= other
//     if (lhs->type() == py::types::set()) {
//         auto result = static_cast<py::PySet *>(lhs)->intersection(rhs);
//         if (result.is_ok()) {
//             static_cast<py::PySet *>(lhs)->elements() = 
//                 std::move(static_cast<py::PySet *>(result.unwrap())->elements());
//             return lhs;
//         }
//     }

//     if (auto *r = try_inplace_method(lhs, rhs, "__iand__")) { return r; }
//     return rt_unwrap(lhs->and_(rhs));
// }