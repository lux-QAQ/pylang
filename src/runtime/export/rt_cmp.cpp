#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"// <-- 新增
#include "runtime/PyList.hpp"// <-- 新增
#include "runtime/PyString.hpp"// <-- 新增
#include "runtime/taggered_pointer/RtValue.hpp"
#include "runtime/types/builtin.hpp"// <-- 新增

// =============================================================================
// Tier 2: 比较操作
// =============================================================================

PYLANG_EXPORT_CMP("compare_eq", "obj", "obj,obj")
py::PyObject *rt_compare_eq(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::compare_eq(py::RtValue::flatten(lhs), py::RtValue::flatten(rhs))
		.as_pyobject_raw();
}

PYLANG_EXPORT_CMP("compare_ne", "obj", "obj,obj")
py::PyObject *rt_compare_ne(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::compare_ne(py::RtValue::flatten(lhs), py::RtValue::flatten(rhs))
		.as_pyobject_raw();
}

PYLANG_EXPORT_CMP("compare_lt", "obj", "obj,obj")
py::PyObject *rt_compare_lt(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::compare_lt(py::RtValue::flatten(lhs), py::RtValue::flatten(rhs))
		.as_pyobject_raw();
}

PYLANG_EXPORT_CMP("compare_le", "obj", "obj,obj")
py::PyObject *rt_compare_le(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::compare_le(py::RtValue::flatten(lhs), py::RtValue::flatten(rhs))
		.as_pyobject_raw();
}

PYLANG_EXPORT_CMP("compare_gt", "obj", "obj,obj")
py::PyObject *rt_compare_gt(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::compare_gt(py::RtValue::flatten(lhs), py::RtValue::flatten(rhs))
		.as_pyobject_raw();
}

PYLANG_EXPORT_CMP("compare_ge", "obj", "obj,obj")
py::PyObject *rt_compare_ge(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::compare_ge(py::RtValue::flatten(lhs), py::RtValue::flatten(rhs))
		.as_pyobject_raw();
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
	auto *b_container = py::ensure_box(container);
	py::RtValue r_val = py::RtValue::flatten(value);

	// [极致优化]：Dict Fast Path (O(1))
	if (b_container->type() == py::types::dict()) {
		auto *dict = static_cast<py::PyDict *>(b_container);

		auto *unboxed_val = py::ensure_box(r_val.as_pyobject_raw());
		if (unboxed_val->type() == py::types::str()) {
			bool found =
				dict->map().find(py::RtValue::from_ptr(static_cast<py::PyString *>(unboxed_val)))
				!= dict->map().end();
			return found ? py::py_true() : py::py_false();
		}
	}

	// [极致优化]：List Fast Path (安全版)
	if (b_container->type() == py::types::list()) {
		auto *list = static_cast<py::PyList *>(b_container);
		bool found = false;
		for (const auto &item : list->elements()) {
			// [核心修复]：使用 RtValue::compare_eq 屏蔽底层 variant 的解引用风险
			if (py::RtValue::compare_eq(item, r_val).is_truthy()) {
				found = true;
				break;
			}
		}
		return found ? py::py_true() : py::py_false();
	}

	bool result = rt_unwrap(b_container->contains(py::ensure_box(value)));
	return result ? py::py_true() : py::py_false();
}

PYLANG_EXPORT_CMP("compare_not_in", "obj", "obj,obj")
py::PyObject *rt_compare_not_in(py::PyObject *value, py::PyObject *container)
{
	// 逻辑同上，仅返回值取反
	auto *b_container = py::ensure_box(container);
	py::RtValue r_val = py::RtValue::flatten(value);

	if (b_container->type() == py::types::dict()) {
		auto *dict = static_cast<py::PyDict *>(b_container);
		auto *unboxed_val = py::ensure_box(r_val.as_pyobject_raw());
		if (unboxed_val->type() == py::types::str()) {
			bool found =
				dict->map().find(py::RtValue::from_ptr(static_cast<py::PyString *>(unboxed_val)))
				!= dict->map().end();
			return found ? py::py_false() : py::py_true();
		}
	}

	if (b_container->type() == py::types::list()) {
		auto *list = static_cast<py::PyList *>(b_container);
		bool found = false;
		for (const auto &item : list->elements()) {
			if (py::RtValue::compare_eq(item, r_val).is_truthy()) {
				found = true;
				break;
			}
		}
		return found ? py::py_false() : py::py_true();
	}

	bool result = rt_unwrap(b_container->contains(py::ensure_box(value)));
	return result ? py::py_false() : py::py_true();
}