#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PySet.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/StopIteration.hpp"
#include "runtime/ValueError.hpp"

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
	if (result.unwrap_err()->type() == py::stop_iteration()->type()) {
		*has_value = false;
		return nullptr;
	}
	rt_raise(result.unwrap_err());
	return nullptr;// unreachable
}

// =============================================================================
// Tier 2: 解包操作
// =============================================================================

PYLANG_EXPORT_SUBSCR("unpack_sequence", "void", "obj,i32,ptr")
void rt_unpack_sequence(py::PyObject *iterable, int32_t count, py::PyObject **out)
{
	// 纯委托给 runtime 的 unpack_sequence
	rt_unwrap_void(py::unpack_sequence(iterable, count, out));
}

/// a, *b, c = iterable
/// before_count: 星号前的变量数量
/// after_count:  星号后的变量数量
/// out:          输出数组，大小 = before_count + 1(list) + after_count
/// a, *b, c = iterable
/// 纯委托给 runtime — export 层不实现语义
PYLANG_EXPORT_SUBSCR("unpack_ex", "void", "obj,i32,i32,ptr")
void rt_unpack_ex(py::PyObject *iterable,
    int32_t before_count,
    int32_t after_count,
    py::PyObject **out)
{
    rt_unwrap_void(py::unpack_ex(iterable, before_count, after_count, out));
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

PYLANG_EXPORT_SUBSCR("list_extend", "void", "obj,obj")
void rt_list_extend(py::PyObject *list, py::PyObject *iterable)
{
	rt_unwrap_void(static_cast<py::PyList *>(list)->extend(iterable));
}

PYLANG_EXPORT_SUBSCR("dict_merge", "void", "obj,obj")
void rt_dict_merge(py::PyObject *dict, py::PyObject *other)
{
	// {**a, **b} 语法 — 两者都已知是 dict，委托给 update
	rt_unwrap_void(static_cast<py::PyDict *>(dict)->update(other));
}

PYLANG_EXPORT_SUBSCR("dict_update", "void", "obj,obj")
void rt_dict_update(py::PyObject *dict, py::PyObject *other)
{
	auto *dict_obj = static_cast<py::PyDict *>(dict);

	// ✅ 委托给 PyDict::update（公有方法）
	rt_unwrap_void(dict_obj->update(other));
}

PYLANG_EXPORT_SUBSCR("set_update", "void", "obj,obj")
void rt_set_update(py::PyObject *set, py::PyObject *iterable)
{
	rt_unwrap_void(static_cast<py::PySet *>(set)->update(iterable));
}

// =============================================================================
// Tier 3: 容器快速访问
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_getitem_fast", "obj", "obj,i32")
py::PyObject *rt_list_getitem_fast(py::PyObject *list, int32_t index)
{
	auto *list_obj = static_cast<py::PyList *>(list);
	return rt_unwrap(list_obj->__getitem__(static_cast<int64_t>(index)));
}

PYLANG_EXPORT_SUBSCR("list_setitem_fast", "void", "obj,i32,obj")
void rt_list_setitem_fast(py::PyObject *list, int32_t index, py::PyObject *value)
{
	auto *list_obj = static_cast<py::PyList *>(list);
	rt_unwrap_void(list_obj->__setitem__(static_cast<int64_t>(index), value));
}

PYLANG_EXPORT_SUBSCR("dict_getitem_str", "obj", "obj,str")
py::PyObject *rt_dict_getitem_str(py::PyObject *dict, const char *key)
{
	auto *key_str = rt_unwrap(py::PyString::create(std::string(key)));
	return rt_unwrap(dict->getitem(key_str));
}

PYLANG_EXPORT_SUBSCR("dict_setitem_str", "void", "obj,str,obj")
void rt_dict_setitem_str(py::PyObject *dict, const char *key, py::PyObject *value)
{
	auto *key_str = rt_unwrap(py::PyString::create(std::string(key)));
	rt_unwrap_void(dict->setitem(key_str, value));
}

PYLANG_EXPORT_SUBSCR("dict_keys", "obj", "obj")
py::PyObject *rt_dict_keys(py::PyObject *dict)
{
	auto *dict_obj = static_cast<py::PyDict *>(dict);

	// ✅ 调用已有的 keys() 方法
	return rt_unwrap(dict_obj->keys());
}

PYLANG_EXPORT_SUBSCR("dict_values", "obj", "obj")
py::PyObject *rt_dict_values(py::PyObject *dict)
{
	auto *dict_obj = static_cast<py::PyDict *>(dict);

	// ✅ 调用已有的 values() 方法
	return rt_unwrap(dict_obj->values());
}

PYLANG_EXPORT_SUBSCR("dict_items", "obj", "obj")
py::PyObject *rt_dict_items(py::PyObject *dict)
{
	auto *dict_obj = static_cast<py::PyDict *>(dict);

	// ✅ 调用已有的 items() 方法（返回 PyDictItems*）
	auto *items = dict_obj->items();
	if (!items) { rt_raise(py::memory_error(sizeof(py::PyDictItems))); }
	return items;
}