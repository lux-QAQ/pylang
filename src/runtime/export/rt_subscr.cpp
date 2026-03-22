#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyByteArray.hpp"
#include "runtime/PyBytes.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyRange.hpp"
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
	auto *b_iter = py::ensure_box(iter);

	// Range 迭代器专属零分配短路通道
	// 确保 Tagged Pointer 只在这个专为 AOT for 循环服务的边界函数中产生，绝不泄漏到普通的 C++
	// Runtime 中！
	if (b_iter->type() == py::types::range_iterator()) {
		if (auto val = static_cast<py::PyRangeIterator *>(b_iter)->next_fast()) {
			*has_value = true;
			return py::RtValue::from_int_or_box(*val).as_pyobject_raw();
		} else {
			*has_value = false;
			return nullptr;
		}
	}


	// 2. 字符串迭代快速路径 (消灭 StopIteration 和 PyString 碎片)
	if (b_iter->type() == py::types::str_iterator()) {
		auto *s_iter = static_cast<py::PyStringIterator *>(b_iter);
		if (auto *val = s_iter->next_raw()) {
			*has_value = true;
			return val;
		}
		*has_value = false;
		return nullptr;
	}


	auto result = b_iter->next();
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
	auto *b_type = b_obj->type();// [修复]：定义 b_type
	py::RtValue r_key = py::RtValue::flatten(key);


    if (b_type == py::types::dict()) {
        auto *dict = static_cast<py::PyDict *>(b_obj);
        // 直接在底层 std::unordered_map 中查找，绕过 getitem 虚函数和 MRO
        auto it = dict->map().find(r_key.to_value());
        if (it != dict->map().end()) {
            return py::RtValue::from_value(it->second).as_pyobject_raw();
        }
    }


	if (r_key.is_tagged_int()) {
		int64_t index = r_key.as_int();
		auto type = b_obj->type();

		// bytes 索引访问 -> 返回 Tagged Integer
		if (type == py::types::bytes()) {
			auto *bytes = static_cast<py::PyBytes *>(b_obj);
			int64_t sz = static_cast<int64_t>(bytes->value().b.size());
			if (index < 0) index += sz;
			if (index >= 0 && index < sz) {
				uint8_t val = static_cast<uint8_t>(bytes->value().b[index]);
				return py::RtValue::from_int(val).as_pyobject_raw();
			}
		}
		// bytearray 索引访问 -> 返回 Tagged Integer
		else if (type == py::types::bytearray()) {
			auto *ba = static_cast<py::PyByteArray *>(b_obj);
			int64_t sz = static_cast<int64_t>(ba->value().b.size());
			if (index < 0) index += sz;
			if (index >= 0 && index < sz) {
				uint8_t val = static_cast<uint8_t>(ba->value().b[index]);
				return py::RtValue::from_int(val).as_pyobject_raw();
			}
		}
		// List/Tuple
		// else if (type == py::types::list()) {
		// 	auto *list = static_cast<py::PyList *>(b_obj);
		// 	int64_t sz = static_cast<int64_t>(list->elements().size());
		// 	if (index < 0) { index += sz; }
		// 	if (index >= 0 && index < sz) { return rt_unwrap(list->__getitem__(index)); }
		// 	// 越界则下降交由底层标准方法报错
		// } else if (b_obj->type() == py::types::tuple()) {
		// 	auto *tuple = static_cast<py::PyTuple *>(b_obj);
		// 	int64_t sz = static_cast<int64_t>(tuple->size());
		// 	if (index < 0) { index += sz; }
		// 	if (index >= 0 && index < sz) { return rt_unwrap(tuple->__getitem__(index)); }
		// }
		else if (type == py::types::list()) {
			auto *list = static_cast<py::PyList *>(b_obj);
			int64_t sz = static_cast<int64_t>(list->elements().size());
			if (index < 0) { index += sz; }
			if (index >= 0 && index < sz) {
				// [优化]：直接从 Value 恢复为 RtValue，避免 __getitem__ 的装箱
				return py::RtValue::from_value(list->elements()[index]).as_pyobject_raw();
			}
		} else if (b_obj->type() == py::types::tuple()) {
			auto *tuple = static_cast<py::PyTuple *>(b_obj);
			int64_t sz = static_cast<int64_t>(tuple->size());
			if (index < 0) { index += sz; }
			if (index >= 0 && index < sz) {
				return py::RtValue::from_value(tuple->elements()[index]).as_pyobject_raw();
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
	auto *b_list = static_cast<py::PyList *>(py::ensure_box(list));
	auto rt_val = py::RtValue::flatten(value);

	if (rt_val.is_tagged_int()) {
		// [核心修复]：如果是 Tagged Integer，转存为 Value 的 Number 分支
		// 这样既消灭了 PyInteger 对象分配，又保证了 ValueEq 的类型安全
		b_list->elements().push_back(py::Value(py::Number(rt_val.as_int())));
	} else {
		b_list->elements().push_back(py::Value(value));
	}
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
	// [极致优化]: 使用 intern 替代 create。
	// 这能保证在大型循环中访问 d["status"] 时，PyString 对象分配次数为 0。
	auto *key_str = py::PyString::intern(key);
	return rt_unwrap(py::ensure_box(dict)->getitem(key_str));
}

PYLANG_EXPORT_SUBSCR("dict_setitem_str", "void", "obj,str,obj")
void rt_dict_setitem_str(py::PyObject *dict, const char *key, py::PyObject *value)
{
	auto *key_str = py::PyString::intern(key);
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