#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PySet.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyType.hpp"
#include "runtime/StopIteration.hpp"
#include "runtime/TypeError.hpp"
#include "runtime/ValueError.hpp"
#include "runtime/types/builtin.hpp"

// =============================================================================
// Tier 1: 布尔判断
// =============================================================================

PYLANG_EXPORT_CONVERT("is_true", "bool", "obj")
bool rt_is_true(py::PyObject *obj) { return rt_unwrap(obj->true_()); }

// =============================================================================
// Tier 1: 类型转换（委托给 runtime）
// =============================================================================

PYLANG_EXPORT_CONVERT("len", "i64", "obj")
int64_t rt_len(py::PyObject *obj)
{
	// 尝试 sequence 协议
	auto seq = obj->as_sequence();
	if (seq.is_ok()) { return rt_unwrap(seq.unwrap().len()); }

	// 尝试 mapping 协议
	auto map = obj->as_mapping();
	if (map.is_ok()) { return rt_unwrap(map.unwrap().len()); }

	rt_raise(py::type_error("object of type '{}' has no len()", obj->type()->name()));
}

PYLANG_EXPORT_CONVERT("isinstance", "bool", "obj,obj")
bool rt_isinstance(py::PyObject *obj, py::PyObject *type)
{
	auto *type_obj = py::as<py::PyType>(type);
	if (!type_obj) { rt_raise(py::type_error("isinstance() arg 2 must be a type")); }
	return obj->type()->issubclass(type_obj);
}

PYLANG_EXPORT_CONVERT("type_of", "obj", "obj")
py::PyObject *rt_type_of(py::PyObject *obj) { return obj->type(); }

// 委托给 int() 的完整实现
PYLANG_EXPORT_CONVERT("to_int", "obj", "obj")
py::PyObject *rt_to_int(py::PyObject *obj)
{
	// 快速路径：如果已经是 int，直接返回
	if (obj->type() == py::types::integer()) { return obj; }

	// 标准路径：通过 int(obj) 调用协议
	auto *int_type = py::types::integer();
	auto *args = rt_unwrap(py::PyTuple::create(obj));
	return rt_unwrap(int_type->call(args, nullptr));
}

// 委托给 str() 的完整实现
PYLANG_EXPORT_CONVERT("to_str", "obj", "obj")
py::PyObject *rt_to_str(py::PyObject *obj)
{
	// 快速路径：如果已经是 str，直接返回
	if (obj->type() == py::types::str()) { return obj; }

	return rt_unwrap(obj->str());
}

// 委托给 bool() 的完整实现
PYLANG_EXPORT_CONVERT("to_bool", "obj", "obj")
py::PyObject *rt_to_bool(py::PyObject *obj)
{
	// 快速路径：如果已经是 bool，直接返回
	if (obj->type() == py::types::bool_()) { return obj; }

	auto result = obj->true_();
	if (result.is_err()) { rt_raise(result.unwrap_err()); }
	return result.unwrap() ? py::py_true() : py::py_false();
}

// ✅ 新增：委托给 float() 的完整实现
PYLANG_EXPORT_CONVERT("to_float", "obj", "obj")
py::PyObject *rt_to_float(py::PyObject *obj)
{
	// 快速路径：如果已经是 float，直接返回
	if (obj->type() == py::types::float_()) { return obj; }

	auto *float_type = py::types::float_();
	auto *args = rt_unwrap(py::PyTuple::create(obj));
	return rt_unwrap(float_type->call(args, nullptr));
}