#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PySet.hpp"
#include "runtime/StopIteration.hpp"

// =============================================================================
// Tier 2: 迭代器
// =============================================================================

PYLANG_EXPORT_SUBSCR("get_iter", "obj", "obj")
py::PyObject *rt_get_iter(py::PyObject *obj) { return rt_unwrap(obj->iter()); }

PYLANG_EXPORT_SUBSCR("iter_next", "obj", "obj,ptr")
py::PyObject *rt_iter_next(py::PyObject *iter, bool *has_value)
{
	auto result = iter->next();
	if (result.is_ok()) {
		*has_value = true;
		return result.unwrap();
	}
	// StopIteration 不是错误，是正常的循环终止
	if (result.unwrap_err()->type() == py::stop_iteration()->type()) {
		*has_value = false;
		return nullptr;
	}
	// 其他异常才是真正的错误
	rt_raise(result.unwrap_err());
}

// =============================================================================
// Tier 3: 下标操作
// =============================================================================

PYLANG_EXPORT_SUBSCR("getitem", "obj", "obj,obj")
py::PyObject *rt_getitem(py::PyObject *obj, py::PyObject *key)
{
	return rt_unwrap(obj->getitem(key));
}

PYLANG_EXPORT_SUBSCR("setitem", "void", "obj,obj,obj")
void rt_setitem(py::PyObject *obj, py::PyObject *key, py::PyObject *value)
{
	rt_unwrap_void(obj->setitem(key, value));
}

PYLANG_EXPORT_SUBSCR("delitem", "void", "obj,obj")
void rt_delitem(py::PyObject *obj, py::PyObject *key) { rt_unwrap_void(obj->delitem(key)); }

// =============================================================================
// Tier 3: 容器方法
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_append", "void", "obj,obj")
void rt_list_append(py::PyObject *list, py::PyObject *value)
{
	static_cast<py::PyList *>(list)->elements().push_back(value);
}

PYLANG_EXPORT_SUBSCR("set_add", "void", "obj,obj")
void rt_set_add(py::PyObject *set, py::PyObject *value)
{
	rt_unwrap_void(static_cast<py::PySet *>(set)->add(value));
}