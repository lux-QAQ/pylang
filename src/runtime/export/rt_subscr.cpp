#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PySet.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/StopIteration.hpp"
#include "runtime/ValueError.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"


#include "runtime/types/builtin.hpp"
// =============================================================================
// Tier 2: 迭代器
// =============================================================================

PYLANG_EXPORT_SUBSCR("get_iter", "obj", "obj")
py::PyObject *rt_get_iter(py::PyObject *obj) { return rt_unwrap(py::ensure_box(obj)->iter()); }

PYLANG_EXPORT_SUBSCR("iter_next", "obj", "obj,ptr")
py::PyObject *rt_iter_next(py::PyObject *iter, bool *has_value)
{
	// iter 一定不是 tagged_int，但防卫编程总没错
	auto result = py::ensure_box(iter)->next();
	if (result.is_ok()) {
		*has_value = true;
		return result.unwrap();
	}
	if (result.unwrap_err()->type() == py::stop_iteration()->type()) {
		*has_value = false;
		return nullptr;
	}
	rt_raise(result.unwrap_err());
	return nullptr;
}

// =============================================================================
// Tier 2: 解包操作
// =============================================================================

PYLANG_EXPORT_SUBSCR("unpack_sequence", "void", "obj,i32,ptr")
void rt_unpack_sequence(py::PyObject *iterable, int32_t count, py::PyObject **out)
{
	rt_unwrap_void(py::unpack_sequence(py::ensure_box(iterable), count, out));
}

PYLANG_EXPORT_SUBSCR("unpack_ex", "void", "obj,i32,i32,ptr")
void rt_unpack_ex(py::PyObject *iterable,
	int32_t before_count,
	int32_t after_count,
	py::PyObject **out)
{
	rt_unwrap_void(py::unpack_ex(py::ensure_box(iterable), before_count, after_count, out));
}

// =============================================================================
// Tier 3: 下标操作 (核心特权区：绝对不能阻断快速路径)
// =============================================================================

PYLANG_EXPORT_SUBSCR("getitem", "obj", "obj,obj")
py::PyObject *rt_getitem(py::PyObject *obj, py::PyObject *key)
{
    auto *b_obj = py::ensure_box(obj);
    py::RtValue r_key = py::RtValue::flatten(key);

    // 快速通道：精确匹配保证子类多态不丢失；负索引补偿补全 Python 语义
    if (r_key.is_tagged_int()) {
        int64_t index = r_key.as_int();

        if (b_obj->type() == py::types::list()) {
            auto *list = static_cast<py::PyList *>(b_obj);
            int64_t sz = static_cast<int64_t>(list->elements().size());
            if (index < 0) { index += sz; }
            if (index >= 0 && index < sz) {
                return rt_unwrap(list->__getitem__(index));
            }
            // 越界则下降交由底层标准方法报错
        } 
        else if (b_obj->type() == py::types::tuple()) {
            auto *tuple = static_cast<py::PyTuple *>(b_obj);
            int64_t sz = static_cast<int64_t>(tuple->size());
            if (index < 0) { index += sz; }
            if (index >= 0 && index < sz) {
                return rt_unwrap(tuple->__getitem__(index));
            }
        }
    }
    return rt_unwrap(b_obj->getitem(r_key.box()));
}

PYLANG_EXPORT_SUBSCR("setitem", "void", "obj,obj,obj")
void rt_setitem(py::PyObject *obj, py::PyObject *key, py::PyObject *value)
{
    auto *b_obj = py::ensure_box(obj);
    py::RtValue r_key = py::RtValue::flatten(key);

    if (r_key.is_tagged_int()) {
        int64_t index = r_key.as_int();

        if (b_obj->type() == py::types::list()) {
            auto *list = static_cast<py::PyList *>(b_obj);
            int64_t sz = static_cast<int64_t>(list->elements().size());
            if (index < 0) { index += sz; }
            if (index >= 0 && index < sz) {
                rt_unwrap_void(list->__setitem__(index, py::ensure_box(value)));
                return;
            }
        }
    }
    rt_unwrap_void(b_obj->setitem(r_key.box(), py::ensure_box(value)));
}

PYLANG_EXPORT_SUBSCR("delitem", "void", "obj,obj")
void rt_delitem(py::PyObject *obj, py::PyObject *key)
{
	py::RtValue r_key = py::RtValue::flatten(key);
	rt_unwrap_void(py::ensure_box(obj)->delitem(r_key.box()));
}

// =============================================================================
// Tier 3: 容器方法
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_append", "void", "obj,obj")
void rt_list_append(py::PyObject *list, py::PyObject *value)
{
	// list->elements() 提取引用后 push_back 返回 void, 不用 rt_unwrap
	static_cast<py::PyList *>(py::ensure_box(list))->elements().push_back(py::ensure_box(value));
}

PYLANG_EXPORT_SUBSCR("set_add", "void", "obj,obj")
void rt_set_add(py::PyObject *set, py::PyObject *value)
{
	rt_unwrap_void(static_cast<py::PySet *>(py::ensure_box(set))->add(py::ensure_box(value)));
}

PYLANG_EXPORT_SUBSCR("list_extend", "void", "obj,obj")
void rt_list_extend(py::PyObject *list, py::PyObject *iterable)
{
	rt_unwrap_void(
		static_cast<py::PyList *>(py::ensure_box(list))->extend(py::ensure_box(iterable)));
}

PYLANG_EXPORT_SUBSCR("dict_merge", "void", "obj,obj")
void rt_dict_merge(py::PyObject *dict, py::PyObject *other)
{
	rt_unwrap_void(static_cast<py::PyDict *>(py::ensure_box(dict))->update(py::ensure_box(other)));
}

PYLANG_EXPORT_SUBSCR("dict_update", "void", "obj,obj")
void rt_dict_update(py::PyObject *dict, py::PyObject *other)
{
	rt_unwrap_void(static_cast<py::PyDict *>(py::ensure_box(dict))->update(py::ensure_box(other)));
}

PYLANG_EXPORT_SUBSCR("set_update", "void", "obj,obj")
void rt_set_update(py::PyObject *set, py::PyObject *iterable)
{
	rt_unwrap_void(static_cast<py::PySet *>(py::ensure_box(set))->update(py::ensure_box(iterable)));
}

// =============================================================================
// Tier 3: 容器快速访问
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_getitem_fast", "obj", "obj,i32")
py::PyObject *rt_list_getitem_fast(py::PyObject *list, int32_t index)
{
	auto *list_obj = static_cast<py::PyList *>(py::ensure_box(list));
	return rt_unwrap(list_obj->__getitem__(static_cast<int64_t>(index)));
}

PYLANG_EXPORT_SUBSCR("list_setitem_fast", "void", "obj,i32,obj")
void rt_list_setitem_fast(py::PyObject *list, int32_t index, py::PyObject *value)
{
	auto *list_obj = static_cast<py::PyList *>(py::ensure_box(list));
	rt_unwrap_void(list_obj->__setitem__(static_cast<int64_t>(index), py::ensure_box(value)));
}

PYLANG_EXPORT_SUBSCR("dict_getitem_str", "obj", "obj,str")
py::PyObject *rt_dict_getitem_str(py::PyObject *dict, const char *key)
{
	auto *key_str = rt_unwrap(py::PyString::create(std::string(key)));
	return rt_unwrap(py::ensure_box(dict)->getitem(key_str));
}

PYLANG_EXPORT_SUBSCR("dict_setitem_str", "void", "obj,str,obj")
void rt_dict_setitem_str(py::PyObject *dict, const char *key, py::PyObject *value)
{
	auto *key_str = rt_unwrap(py::PyString::create(std::string(key)));
	rt_unwrap_void(py::ensure_box(dict)->setitem(key_str, py::ensure_box(value)));
}

PYLANG_EXPORT_SUBSCR("dict_keys", "obj", "obj")
py::PyObject *rt_dict_keys(py::PyObject *dict)
{
	return rt_unwrap(static_cast<py::PyDict *>(py::ensure_box(dict))->keys());
}

PYLANG_EXPORT_SUBSCR("dict_values", "obj", "obj")
py::PyObject *rt_dict_values(py::PyObject *dict)
{
	return rt_unwrap(static_cast<py::PyDict *>(py::ensure_box(dict))->values());
}

PYLANG_EXPORT_SUBSCR("dict_items", "obj", "obj")
py::PyObject *rt_dict_items(py::PyObject *dict)
{
	auto *items = static_cast<py::PyDict *>(py::ensure_box(dict))->items();
	if (!items) { rt_raise(py::memory_error(sizeof(py::PyDictItems))); }
	return items;
}