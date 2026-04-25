#include "rt_common.hpp"

#include "rt_tagged_ops.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PySet.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"
#include "runtime/types/builtin.hpp"

// =============================================================================
// 融合运算 (Fused Operations)
//
// 这些函数将多个热路径操作合并为单个调用，消除中间对象分配、
// 重复 flatten/box 转换和类型检查。
//
// 设计原则：
//   - 每个融合函数覆盖一个在 VTune 热路径中反复出现的 pattern
//   - 快速路径处理 tagged integer 和已知类型，慢路径安全回退
//   - 严格遵循 Python 3.9 语义
// =============================================================================

// =============================================================================
// 1. rt_compare_lt_bool / rt_compare_le_bool / rt_compare_gt_bool
//
// 融合: compare + is_true → 直接返回 i1 (bool)
// 热路径: while y*y < self.limit; while x*x < self.limit
//
// 原始链: rt_compare_lt → RtValue::flatten × 2 → RtValue::compare_lt
//         → as_pyobject_raw → rt_is_true → flatten → is_truthy
// 融合后: 直接比较返回 C bool，无中间 PyObject* 分配
// =============================================================================

PYLANG_EXPORT_CMP("compare_lt_bool", "i1", "obj,obj")
bool rt_compare_lt_bool(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::rt::compare_int_bool_or_slow(
		lhs,
		rhs,
		[](int64_t l, int64_t r) { return l < r; },
		[](py::RtValue l, py::RtValue r) { return py::RtValue::compare_lt(l, r); });
}

PYLANG_EXPORT_CMP("compare_le_bool", "i1", "obj,obj")
bool rt_compare_le_bool(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::rt::compare_int_bool_or_slow(
		lhs,
		rhs,
		[](int64_t l, int64_t r) { return l <= r; },
		[](py::RtValue l, py::RtValue r) { return py::RtValue::compare_le(l, r); });
}

PYLANG_EXPORT_CMP("compare_gt_bool", "i1", "obj,obj")
bool rt_compare_gt_bool(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::rt::compare_int_bool_or_slow(
		lhs,
		rhs,
		[](int64_t l, int64_t r) { return l > r; },
		[](py::RtValue l, py::RtValue r) { return py::RtValue::compare_gt(l, r); });
}

PYLANG_EXPORT_CMP("compare_eq_bool", "i1", "obj,obj")
bool rt_compare_eq_bool(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::rt::compare_int_bool_or_slow(
		lhs,
		rhs,
		[](int64_t l, int64_t r) { return l == r; },
		[](py::RtValue l, py::RtValue r) { return py::RtValue::compare_eq(l, r); });
}

// =============================================================================
// 2. rt_binary_mul_int / rt_binary_add_int
//
// 当两个操作数都是整数时的乘法/加法: 返回 PyObject* (tagged or boxed)
// 与 rt_binary_mul 相同实现但用于编译器识别场景提示
// (实际加速在于编译器端将连续的 mul+compare 融合)
// =============================================================================

// =============================================================================
// 3. rt_is_true_fast
//
// 融合版 is_true: 直接返回 C bool
// 对于 tagged int 和 bool 有零开销路径
// =============================================================================

PYLANG_EXPORT_CONVERT("is_true_fast", "i1", "obj")
bool rt_is_true_fast(py::PyObject *obj) { return py::rt::truthy(obj); }

// =============================================================================
// 4. rt_list_getitem_i64 / rt_list_setitem_i64
//
// 融合: list[tagged_int_index] 的 get/set
// 跳过 flatten + type dispatch chain
// 热路径: self.prime[n] 的读写 (Sieve 中最热操作)
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_getitem_i64", "obj", "obj,obj")
py::PyObject *rt_list_getitem_i64(py::PyObject *list, py::PyObject *index)
{
	py::PyObject *result = nullptr;

	if (py::rt::exact_list_index(list, index, [&result](py::PyList *py_list, int64_t idx) {
			result = py_list->unchecked_at(static_cast<size_t>(idx)).as_pyobject_raw();
			return true;
		})) {
		return result;
	}
	// 回退到通用路径
	return rt_unwrap(py::ensure_box(list)->getitem(py::ensure_box(index)));
}

PYLANG_EXPORT_SUBSCR("list_setitem_i64", "void", "obj,obj,obj")
void rt_list_setitem_i64(py::PyObject *list, py::PyObject *index, py::PyObject *value)
{
	if (py::rt::exact_list_index(list, index, [value](py::PyList *py_list, int64_t idx) {
			rt_unwrap_void(py_list->__setitem__(idx, py::ensure_box(value)));
			return true;
		})) {
		return;
	}
	rt_unwrap_void(py::ensure_box(list)->setitem(py::ensure_box(index), py::ensure_box(value)));
}

// =============================================================================
// 5. rt_dict_getitem_str / rt_dict_setitem_str_obj
//
// 融合: dict[string_key] 的直接访问
// 跳过 type dispatch, 直接查 hash map
// 热路径: head.children[ch] 访问
// =============================================================================

PYLANG_EXPORT_SUBSCR("dict_getitem", "obj", "obj,obj")
py::PyObject *rt_dict_getitem(py::PyObject *dict, py::PyObject *key)
{
	auto *b_dict = py::ensure_box(dict);
	if (__builtin_expect(b_dict->type() == py::types::dict(), 1)) {
		auto *py_dict = static_cast<py::PyDict *>(b_dict);
		auto *b_key = py::ensure_box(key);
		// String key fast path
		if (b_key->type() == py::types::str()) {
			auto it = py_dict->map().find(py::Value(static_cast<py::PyString *>(b_key)));
			if (it != py_dict->map().end()) { return it->second.as_pyobject_raw(); }
		}
		return rt_unwrap(py_dict->getitem(b_key));
	}
	return rt_unwrap(b_dict->getitem(py::ensure_box(key)));
}

PYLANG_EXPORT_SUBSCR("dict_setitem", "void", "obj,obj,obj")
void rt_dict_setitem(py::PyObject *dict, py::PyObject *key, py::PyObject *value)
{
	auto *b_dict = py::ensure_box(dict);
	if (__builtin_expect(b_dict->type() == py::types::dict(), 1)) {
		auto *py_dict = static_cast<py::PyDict *>(b_dict);
		py_dict->insert(py::ensure_box(key), py::ensure_box(value));
		return;
	}
	rt_unwrap_void(b_dict->setitem(py::ensure_box(key), py::ensure_box(value)));
}

// =============================================================================
// 6. rt_dict_contains_str_bool
//
// 融合: `ch not in head.children` / `ch in head.children`
// 直接返回 C bool, 避免 PyBool 打包
// =============================================================================

PYLANG_EXPORT_CMP("dict_contains_bool", "i1", "obj,obj")
bool rt_dict_contains_bool(py::PyObject *key, py::PyObject *container)
{
	auto *b_container = py::ensure_box(container);
	if (__builtin_expect(b_container->type() == py::types::dict(), 1)) {
		auto *dict = static_cast<py::PyDict *>(b_container);
		auto *b_key = py::ensure_box(key);
		if (b_key->type() == py::types::str()) {
			return dict->map().find(py::Value(static_cast<py::PyString *>(b_key)))
				   != dict->map().end();
		}
		// Non-string key
		py::RtValue r_key = py::RtValue::flatten(key);
		return dict->map().find(r_key) != dict->map().end();
	}
	return rt_unwrap(b_container->contains(py::ensure_box(key)));
}

// =============================================================================
// 7. rt_list_insert_0_tuple2
//
// 融合: queue.insert(0, (v, new_prefix))
// 避免: rt_build_tuple + rt_call_method_ic_ptrs (insert) 的开销
// 直接构建 tuple 并 insert 到 list[0]
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_insert_0_tuple2", "void", "obj,obj,obj")
void rt_list_insert_0_tuple2(py::PyObject *list, py::PyObject *a, py::PyObject *b)
{
	auto *py_list = static_cast<py::PyList *>(py::ensure_box(list));

	// 直接创建 PyTuple 并 insert 到 index 0
	auto tuple = py::PyTuple::create(py::ensure_box(a), py::ensure_box(b));
	if (tuple.is_err()) { rt_raise(tuple.unwrap_err()); }

	py_list->push_front_raw(py::Value(tuple.unwrap()));
}

// =============================================================================
// 8. rt_dict_get_or_null
//
// 融合: head.children.get(ch) 但返回 raw ptr, null 表示不存在
// 用于: head = head.children.get(ch); if head is None: return None
// =============================================================================

PYLANG_EXPORT_SUBSCR("dict_get_or_null", "obj", "obj,obj")
py::PyObject *rt_dict_get_or_null(py::PyObject *dict, py::PyObject *key)
{
	auto *b_dict = py::ensure_box(dict);
	if (__builtin_expect(b_dict->type() == py::types::dict(), 1)) {
		auto *py_dict = static_cast<py::PyDict *>(b_dict);
		auto *b_key = py::ensure_box(key);
		if (b_key->type() == py::types::str()) {
			auto it = py_dict->map().find(py::Value(static_cast<py::PyString *>(b_key)));
			if (it != py_dict->map().end()) { return it->second.as_pyobject_raw(); }
			return py::py_none();
		}
		auto result = py_dict->get(b_key, nullptr);
		if (result.is_ok()) {
			auto *val = result.unwrap();
			return val ? val : py::py_none();
		}
	}
	return py::py_none();
}

// =============================================================================
// 9. rt_list_pop_unpack2
//
// 融合: top, current_prefix = queue.pop()
// 避免中间 PyTuple 解包: list.pop() → unpack_sequence → store
// 直接 pop + 解包到两个输出指针
// =============================================================================

PYLANG_EXPORT_SUBSCR("list_pop_unpack2", "i1", "obj,ptr,ptr")
bool rt_list_pop_unpack2(py::PyObject *list, py::PyObject **out_a, py::PyObject **out_b)
{
	auto *py_list = static_cast<py::PyList *>(py::ensure_box(list));
	if (py_list->empty()) { return false; }

	// Pop the last element
	auto last = py_list->pop_back_raw();

	// Unpack as 2-tuple
	auto *tuple = py::as<py::PyTuple>(last.as_pyobject_raw());
	if (__builtin_expect(tuple != nullptr && tuple->size() == 2, 1)) {
		*out_a = tuple->elements()[0].as_pyobject_raw();
		*out_b = tuple->elements()[1].as_pyobject_raw();
		return true;
	}

	// Fallback: use generic unpack
	auto *item = last.as_pyobject_raw();
	py::PyObject *out_arr[2];
	rt_unwrap_void(py::unpack_sequence(item, 2, out_arr));
	*out_a = out_arr[0];
	*out_b = out_arr[1];
	return true;
}

// =============================================================================
// 10. rt_getattr_self_slot
//
// 融合: self.attr 的极速访问 (跳过 IC 验证开销)
// 当编译器已知 obj 是 self 且 attr 是一个 instance slot 时使用
// 通过 inline cache 检查 shape 后直接访问 slot
// =============================================================================

// (此函数复用已有的 rt_getattr_ic, 但在编译器端做更积极的内联提示)

// =============================================================================
// 11. rt_compare_ne_bool / rt_compare_ge_bool
//
// 融合: compare + is_true → 直接返回 i1
// 补全 lt/le/gt/eq 的对称操作
// =============================================================================

PYLANG_EXPORT_CMP("compare_ne_bool", "i1", "obj,obj")
bool rt_compare_ne_bool(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::rt::compare_int_bool_or_slow(
		lhs,
		rhs,
		[](int64_t l, int64_t r) { return l != r; },
		[](py::RtValue l, py::RtValue r) { return py::RtValue::compare_ne(l, r); });
}

PYLANG_EXPORT_CMP("compare_ge_bool", "i1", "obj,obj")
bool rt_compare_ge_bool(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::rt::compare_int_bool_or_slow(
		lhs,
		rhs,
		[](int64_t l, int64_t r) { return l >= r; },
		[](py::RtValue l, py::RtValue r) { return py::RtValue::compare_ge(l, r); });
}

// =============================================================================
// 12. rt_compare_not_in_bool / rt_compare_in_bool
//
// 融合: `ch not in head.children` / `ch in dict` → 直接返回 i1
// 不分配 PyBool 中间对象
// 热路径: generate_trie 中 `if ch not in head.children:`
// =============================================================================

PYLANG_EXPORT_CMP("compare_not_in_bool", "i1", "obj,obj")
bool rt_compare_not_in_bool(py::PyObject *value, py::PyObject *container)
{
	auto *b_container = py::ensure_box(container);

	// Dict fast path — 最热路径
	if (__builtin_expect(b_container->type() == py::types::dict(), 1)) {
		auto *dict = static_cast<py::PyDict *>(b_container);
		auto *b_val = py::ensure_box(value);
		if (b_val->type() == py::types::str()) {
			return dict->map().find(py::Value(static_cast<py::PyString *>(b_val)))
				   == dict->map().end();
		}
		py::RtValue r_val = py::RtValue::flatten(value);
		return dict->map().find(r_val) == dict->map().end();
	}

	// List path
	if (b_container->type() == py::types::list()) {
		auto *list = static_cast<py::PyList *>(b_container);
		py::RtValue r_val = py::RtValue::raw_is_tagged_int(value) ? py::RtValue::from_ptr(value)
																  : py::RtValue::flatten(value);
		for (const auto &item : list->elements()) {
			if (py::RtValue::compare_eq(item, r_val).is_truthy()) { return false; }
		}
		return true;
	}

	// Set/FrozenSet path
	if (b_container->type() == py::types::set() || b_container->type() == py::types::frozenset()) {
		return !rt_unwrap(b_container->contains(py::ensure_box(value)));
	}

	// Generic fallback
	return !rt_unwrap(b_container->contains(py::ensure_box(value)));
}

PYLANG_EXPORT_CMP("compare_in_bool", "i1", "obj,obj")
bool rt_compare_in_bool(py::PyObject *value, py::PyObject *container)
{
	auto *b_container = py::ensure_box(container);

	// Dict fast path
	if (__builtin_expect(b_container->type() == py::types::dict(), 1)) {
		auto *dict = static_cast<py::PyDict *>(b_container);
		auto *b_val = py::ensure_box(value);
		if (b_val->type() == py::types::str()) {
			return dict->map().find(py::Value(static_cast<py::PyString *>(b_val)))
				   != dict->map().end();
		}
		py::RtValue r_val = py::RtValue::flatten(value);
		return dict->map().find(r_val) != dict->map().end();
	}

	// List path
	if (b_container->type() == py::types::list()) {
		auto *list = static_cast<py::PyList *>(b_container);
		py::RtValue r_val = py::RtValue::raw_is_tagged_int(value) ? py::RtValue::from_ptr(value)
																  : py::RtValue::flatten(value);
		for (const auto &item : list->elements()) {
			if (py::RtValue::compare_eq(item, r_val).is_truthy()) { return true; }
		}
		return false;
	}

	// Generic fallback
	return rt_unwrap(b_container->contains(py::ensure_box(value)));
}

// =============================================================================
// 13. rt_setitem_dict_fast
//
// 融合: dict.__setitem__(key, value)
// 跳过 type dispatch，直接插入
// 热路径: head.children[ch] = Node()
// =============================================================================

PYLANG_EXPORT_SUBSCR("setitem_fast", "void", "obj,obj,obj")
void rt_setitem_fast(py::PyObject *obj, py::PyObject *key, py::PyObject *value)
{
	auto *b_obj = py::ensure_box(obj);

	// Dict fast path (最热)
	if (__builtin_expect(b_obj->type() == py::types::dict(), 1)) {
		auto *dict = static_cast<py::PyDict *>(b_obj);
		dict->insert(py::ensure_box(key), py::ensure_box(value));
		return;
	}

	// List fast path (tagged int key)
	if (py::RtValue::raw_is_tagged_int(key) && b_obj->type() == py::types::list()) {
		auto *list = static_cast<py::PyList *>(b_obj);
		int64_t idx = py::RtValue::raw_as_int(key);
		int64_t sz = static_cast<int64_t>(list->logical_size());
		if (idx < 0) idx += sz;
		if (idx >= 0 && idx < sz) {
			rt_unwrap_void(list->__setitem__(idx, py::ensure_box(value)));
			return;
		}
	}

	// Generic fallback
	py::RtValue r_key = py::RtValue::flatten(key);
	rt_unwrap_void(b_obj->setitem(r_key.box(), py::ensure_box(value)));
}
