#include "rt_common.hpp"

#include "runtime/NotImplemented.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

// =============================================================================
// Tier 1: 二元运算
//
// 每个函数保留 tagged-int 快速路径，堆路径通过 rt_unwrap 正确传播 Python 异常
// =============================================================================

PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
py::PyObject *rt_binary_add(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t result;
		if (!__builtin_add_overflow(
				py::RtValue::raw_as_int(lhs), py::RtValue::raw_as_int(rhs), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t result;
		if (!__builtin_add_overflow(l.as_int(), r.as_int(), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	return rt_unwrap(l.box()->add(r.box()));
}

PYLANG_EXPORT_OP("binary_sub", "obj", "obj,obj")
py::PyObject *rt_binary_sub(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t result;
		if (!__builtin_sub_overflow(
				py::RtValue::raw_as_int(lhs), py::RtValue::raw_as_int(rhs), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t result;
		if (!__builtin_sub_overflow(l.as_int(), r.as_int(), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	return rt_unwrap(l.box()->subtract(r.box()));
}

PYLANG_EXPORT_OP("binary_mul", "obj", "obj,obj")
py::PyObject *rt_binary_mul(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t result;
		if (!__builtin_mul_overflow(
				py::RtValue::raw_as_int(lhs), py::RtValue::raw_as_int(rhs), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t result;
		if (!__builtin_mul_overflow(l.as_int(), r.as_int(), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	return rt_unwrap(l.box()->multiply(r.box()));
}

PYLANG_EXPORT_OP("binary_truediv", "obj", "obj,obj")
py::PyObject *rt_binary_truediv(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(py::ensure_box(lhs)->truediv(py::ensure_box(rhs)));
}

PYLANG_EXPORT_OP("binary_floordiv", "obj", "obj,obj")
py::PyObject *rt_binary_floordiv(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t lv = py::RtValue::raw_as_int(lhs), rv = py::RtValue::raw_as_int(rhs);
		if (rv != 0) {
			int64_t q = lv / rv;
			int64_t rem = lv % rv;
			if ((rem != 0) && ((rem ^ rv) < 0)) { q--; }
			if (py::RtValue::fits_tagged_int(q)) {
				return py::RtValue::from_int(q).as_pyobject_raw();
			}
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t lv = l.as_int(), rv = r.as_int();
		if (rv != 0) {
			int64_t q = lv / rv;
			int64_t rem = lv % rv;
			if ((rem != 0) && ((rem ^ rv) < 0)) { q--; }
			if (py::RtValue::fits_tagged_int(q)) {
				return py::RtValue::from_int(q).as_pyobject_raw();
			}
		}
	}
	return rt_unwrap(l.box()->floordiv(r.box()));
}

PYLANG_EXPORT_OP("binary_mod", "obj", "obj,obj")
py::PyObject *rt_binary_mod(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t lv = py::RtValue::raw_as_int(lhs), rv = py::RtValue::raw_as_int(rhs);
		if (rv != 0) {
			int64_t rem = lv % rv;
			if ((rem != 0) && ((rem ^ rv) < 0)) { rem += rv; }
			return py::RtValue::from_int(rem).as_pyobject_raw();
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t lv = l.as_int(), rv = r.as_int();
		if (rv != 0) {
			int64_t rem = lv % rv;
			if ((rem != 0) && ((rem ^ rv) < 0)) { rem += rv; }
			return py::RtValue::from_int(rem).as_pyobject_raw();
		}
	}
	return rt_unwrap(l.box()->modulo(r.box()));
}

PYLANG_EXPORT_OP("binary_pow", "obj", "obj,obj")
py::PyObject *rt_binary_pow(py::PyObject *lhs, py::PyObject *rhs)
{
	return rt_unwrap(py::ensure_box(lhs)->pow(py::ensure_box(rhs), py::py_none()));
}

PYLANG_EXPORT_OP("binary_lshift", "obj", "obj,obj")
py::PyObject *rt_binary_lshift(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t lv = py::RtValue::raw_as_int(lhs), rv = py::RtValue::raw_as_int(rhs);
		if (rv >= 0 && rv < 63) {
			__int128 result = static_cast<__int128>(lv) * (static_cast<__int128>(1) << rv);
			if (result >= py::RtValue::kTaggedIntMin && result <= py::RtValue::kTaggedIntMax) {
				return py::RtValue::from_int(static_cast<int64_t>(result)).as_pyobject_raw();
			}
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t lv = l.as_int(), rv = r.as_int();
		if (rv >= 0 && rv < 63) {
			__int128 result = static_cast<__int128>(lv) * (static_cast<__int128>(1) << rv);
			if (result >= py::RtValue::kTaggedIntMin && result <= py::RtValue::kTaggedIntMax) {
				return py::RtValue::from_int(static_cast<int64_t>(result)).as_pyobject_raw();
			}
		}
	}
	return rt_unwrap(l.box()->lshift(r.box()));
}

PYLANG_EXPORT_OP("binary_rshift", "obj", "obj,obj")
py::PyObject *rt_binary_rshift(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t rv = py::RtValue::raw_as_int(rhs);
		if (rv >= 0 && rv < 63) {
			return py::RtValue::from_int(py::RtValue::raw_as_int(lhs) >> rv).as_pyobject_raw();
		}
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		int64_t lv = l.as_int(), rv = r.as_int();
		if (rv >= 0 && rv < 63) { return py::RtValue::from_int(lv >> rv).as_pyobject_raw(); }
	}
	return rt_unwrap(l.box()->rshift(r.box()));
}

PYLANG_EXPORT_OP("binary_and", "obj", "obj,obj")
py::PyObject *rt_binary_and(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		return py::RtValue::from_int(py::RtValue::raw_as_int(lhs) & py::RtValue::raw_as_int(rhs))
			.as_pyobject_raw();
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		return py::RtValue::from_int(l.as_int() & r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->and_(r.box()));
}

PYLANG_EXPORT_OP("binary_or", "obj", "obj,obj")
py::PyObject *rt_binary_or(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		return py::RtValue::from_int(py::RtValue::raw_as_int(lhs) | py::RtValue::raw_as_int(rhs))
			.as_pyobject_raw();
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		return py::RtValue::from_int(l.as_int() | r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->or_(r.box()));
}

PYLANG_EXPORT_OP("binary_xor", "obj", "obj,obj")
py::PyObject *rt_binary_xor(py::PyObject *lhs, py::PyObject *rhs)
{
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		return py::RtValue::from_int(py::RtValue::raw_as_int(lhs) ^ py::RtValue::raw_as_int(rhs))
			.as_pyobject_raw();
	}
	auto l = py::RtValue::flatten(lhs);
	auto r = py::RtValue::flatten(rhs);
	if (py::RtValue::are_both_tagged_int(l, r)) {
		return py::RtValue::from_int(l.as_int() ^ r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->xor_(r.box()));
}

// =============================================================================
// Tier 1: 一元运算
// =============================================================================

PYLANG_EXPORT_OP("unary_neg", "obj", "obj")
py::PyObject *rt_unary_neg(py::PyObject *obj)
{
	if (py::RtValue::raw_is_tagged_int(obj)) {
		auto v = py::RtValue::raw_as_int(obj);
		if (py::RtValue::fits_tagged_int(-v)) {
			return py::RtValue::from_int(-v).as_pyobject_raw();
		}
	}
	auto v = py::RtValue::flatten(obj);
	if (v.is_tagged_int()) {
		if (py::RtValue::fits_tagged_int(-v.as_int())) {
			return py::RtValue::from_int(-v.as_int()).as_pyobject_raw();
		}
	}
	return rt_unwrap(py::ensure_box(obj)->neg());
}

PYLANG_EXPORT_OP("unary_pos", "obj", "obj")
py::PyObject *rt_unary_pos(py::PyObject *obj)
{
	if (py::RtValue::raw_is_tagged_int(obj)) return obj;
	if (py::RtValue::flatten(obj).is_tagged_int()) return obj;
	return rt_unwrap(py::ensure_box(obj)->pos());
}

PYLANG_EXPORT_OP("unary_invert", "obj", "obj")
py::PyObject *rt_unary_invert(py::PyObject *obj)
{
	if (py::RtValue::raw_is_tagged_int(obj)) {
		return py::RtValue::from_int(~py::RtValue::raw_as_int(obj)).as_pyobject_raw();
	}
	auto v = py::RtValue::flatten(obj);
	if (v.is_tagged_int()) { return py::RtValue::from_int(~v.as_int()).as_pyobject_raw(); }
	return rt_unwrap(py::ensure_box(obj)->invert());
}

PYLANG_EXPORT_OP("unary_not", "obj", "obj")
py::PyObject *rt_unary_not(py::PyObject *obj)
{
	if (py::RtValue::raw_is_tagged_int(obj)) {
		return py::RtValue::raw_as_int(obj) != 0 ? py::py_false() : py::py_true();
	}
	return py::RtValue::flatten(obj).is_truthy() ? py::py_false() : py::py_true();
}

// =============================================================================
// Tier 1: 增量赋值（inplace）运算
// =============================================================================

namespace {
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
	if (py::RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t result;
		if (!__builtin_add_overflow(
				py::RtValue::raw_as_int(lhs), py::RtValue::raw_as_int(rhs), &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	auto f_lhs = py::RtValue::flatten(lhs);
	auto f_rhs = py::RtValue::flatten(rhs);
	if (f_lhs.is_tagged_int() && f_rhs.is_tagged_int()) {
		return py::RtValue::add(f_lhs, f_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__iadd__")) { return r; }
	return rt_unwrap(b_lhs->add(b_rhs));
}

PYLANG_EXPORT_OP("inplace_sub", "obj", "obj,obj")
py::PyObject *rt_inplace_sub(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::sub(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__isub__")) { return r; }
	return rt_unwrap(b_lhs->subtract(b_rhs));
}

PYLANG_EXPORT_OP("inplace_mul", "obj", "obj,obj")
py::PyObject *rt_inplace_mul(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::mul(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__imul__")) { return r; }
	return rt_unwrap(b_lhs->multiply(b_rhs));
}

PYLANG_EXPORT_OP("inplace_truediv", "obj", "obj,obj")
py::PyObject *rt_inplace_truediv(py::PyObject *lhs, py::PyObject *rhs)
{
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__itruediv__")) { return r; }
	return rt_unwrap(b_lhs->truediv(b_rhs));
}

PYLANG_EXPORT_OP("inplace_floordiv", "obj", "obj,obj")
py::PyObject *rt_inplace_floordiv(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::floordiv(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__ifloordiv__")) { return r; }
	return rt_unwrap(b_lhs->floordiv(b_rhs));
}

PYLANG_EXPORT_OP("inplace_mod", "obj", "obj,obj")
py::PyObject *rt_inplace_mod(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::mod(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__imod__")) { return r; }
	return rt_unwrap(b_lhs->modulo(b_rhs));
}

PYLANG_EXPORT_OP("inplace_pow", "obj", "obj,obj")
py::PyObject *rt_inplace_pow(py::PyObject *lhs, py::PyObject *rhs)
{
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__ipow__")) { return r; }
	return rt_unwrap(b_lhs->pow(b_rhs, py::py_none()));
}

PYLANG_EXPORT_OP("inplace_lshift", "obj", "obj,obj")
py::PyObject *rt_inplace_lshift(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::lshift(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__ilshift__")) { return r; }
	return rt_unwrap(b_lhs->lshift(b_rhs));
}

PYLANG_EXPORT_OP("inplace_rshift", "obj", "obj,obj")
py::PyObject *rt_inplace_rshift(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::rshift(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__irshift__")) { return r; }
	return rt_unwrap(b_lhs->rshift(b_rhs));
}

PYLANG_EXPORT_OP("inplace_and", "obj", "obj,obj")
py::PyObject *rt_inplace_and(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::bit_and(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__iand__")) { return r; }
	return rt_unwrap(b_lhs->and_(b_rhs));
}

PYLANG_EXPORT_OP("inplace_or", "obj", "obj,obj")
py::PyObject *rt_inplace_or(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::bit_or(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__ior__")) { return r; }
	return rt_unwrap(b_lhs->or_(b_rhs));
}

PYLANG_EXPORT_OP("inplace_xor", "obj", "obj,obj")
py::PyObject *rt_inplace_xor(py::PyObject *lhs, py::PyObject *rhs)
{
	auto th_lhs = py::RtValue::flatten(lhs);
	auto th_rhs = py::RtValue::flatten(rhs);
	if (th_lhs.is_tagged_int() && th_rhs.is_tagged_int()) {
		return py::RtValue::bit_xor(th_lhs, th_rhs).as_pyobject_raw();
	}
	auto *b_lhs = py::ensure_box(lhs);
	auto *b_rhs = py::ensure_box(rhs);
	if (auto *r = try_inplace_method(b_lhs, b_rhs, "__ixor__")) { return r; }
	return rt_unwrap(b_lhs->xor_(b_rhs));
}
