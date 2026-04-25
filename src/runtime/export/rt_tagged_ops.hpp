#pragma once

#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"
#include "runtime/types/builtin.hpp"

#include <cstdint>
#include <type_traits>

namespace py::rt {

struct RawObject
{
	PyObject *ptr;
};

struct TaggedInt
{
	int64_t value;
};

[[nodiscard]] inline bool is_tagged_int(RawObject value)
{
	return RtValue::raw_is_tagged_int(value.ptr);
}

[[nodiscard]] inline TaggedInt as_tagged_int(RawObject value)
{
	return TaggedInt{ RtValue::raw_as_int(value.ptr) };
}

[[nodiscard]] inline PyObject *tagged_result(int64_t value)
{
	return RtValue::from_int(value).as_pyobject_raw();
}

[[nodiscard]] inline PyObject *py_bool(bool value) { return value ? py_true() : py_false(); }

template<typename Fast, typename Slow>
[[nodiscard]] inline PyObject *
	binary_int_or_slow(PyObject *lhs, PyObject *rhs, Fast &&fast, Slow &&slow)
{
	static_assert(std::is_invocable_r_v<PyObject *, Fast, int64_t, int64_t>);
	static_assert(std::is_invocable_r_v<PyObject *, Slow, RtValue, RtValue>);

	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		if (auto *result = fast(RtValue::raw_as_int(lhs), RtValue::raw_as_int(rhs))) {
			return result;
		}
	}

	auto l = RtValue::flatten(lhs);
	auto r = RtValue::flatten(rhs);
	if (RtValue::are_both_tagged_int(l, r)) {
		if (auto *result = fast(l.as_int(), r.as_int())) { return result; }
	}

	return slow(l, r);
}

template<typename Predicate, typename Slow>
[[nodiscard]] inline PyObject *
	compare_int_or_slow(PyObject *lhs, PyObject *rhs, Predicate &&predicate, Slow &&slow)
{
	static_assert(std::is_invocable_r_v<bool, Predicate, int64_t, int64_t>);
	static_assert(std::is_invocable_r_v<RtValue, Slow, RtValue, RtValue>);

	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		return py_bool(predicate(RtValue::raw_as_int(lhs), RtValue::raw_as_int(rhs)));
	}

	auto l = RtValue::flatten(lhs);
	auto r = RtValue::flatten(rhs);
	if (RtValue::are_both_tagged_int(l, r)) { return py_bool(predicate(l.as_int(), r.as_int())); }

	return slow(l, r).as_pyobject_raw();
}

template<typename Predicate, typename Slow>
[[nodiscard]] inline bool
	compare_int_bool_or_slow(PyObject *lhs, PyObject *rhs, Predicate &&predicate, Slow &&slow)
{
	static_assert(std::is_invocable_r_v<bool, Predicate, int64_t, int64_t>);
	static_assert(std::is_invocable_r_v<RtValue, Slow, RtValue, RtValue>);

	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		return predicate(RtValue::raw_as_int(lhs), RtValue::raw_as_int(rhs));
	}

	auto l = RtValue::flatten(lhs);
	auto r = RtValue::flatten(rhs);
	if (RtValue::are_both_tagged_int(l, r)) { return predicate(l.as_int(), r.as_int()); }

	return slow(l, r).is_truthy();
}

template<typename Fast, typename Slow>
[[nodiscard]] inline PyObject *unary_int_or_slow(PyObject *obj, Fast &&fast, Slow &&slow)
{
	static_assert(std::is_invocable_r_v<PyObject *, Fast, int64_t>);
	static_assert(std::is_invocable_r_v<PyObject *, Slow, RtValue>);

	if (RtValue::raw_is_tagged_int(obj)) {
		if (auto *result = fast(RtValue::raw_as_int(obj))) { return result; }
	}

	auto value = RtValue::flatten(obj);
	if (value.is_tagged_int()) {
		if (auto *result = fast(value.as_int())) { return result; }
	}

	return slow(value);
}

[[nodiscard]] inline bool truthy(PyObject *obj)
{
	if (RtValue::raw_is_tagged_int(obj)) { return RtValue::raw_as_int(obj) != 0; }

	auto value = RtValue::flatten(obj);
	if (value.is_tagged_int()) { return value.as_int() != 0; }

	auto *boxed = value.box();
	if (boxed->type() == types::bool_()) { return static_cast<PyBool *>(boxed)->value(); }
	if (boxed == py_none()) { return false; }
	return rt_unwrap(boxed->true_());
}

template<typename Hit>
[[nodiscard]] inline bool exact_list_index(PyObject *list, PyObject *index, Hit &&hit)
{
	static_assert(std::is_invocable_r_v<bool, Hit, PyList *, int64_t>);

	auto *boxed_list = ensure_box(list);
	auto try_hit = [&hit](PyList *py_list, int64_t idx) {
		auto sz = static_cast<int64_t>(py_list->logical_size());
		if (idx < 0) { idx += sz; }
		if (__builtin_expect(idx >= 0 && idx < sz, 1)) { return hit(py_list, idx); }
		return false;
	};

	if (__builtin_expect(
			boxed_list->type() == types::list() && RtValue::raw_is_tagged_int(index), 1)) {
		return try_hit(static_cast<PyList *>(boxed_list), RtValue::raw_as_int(index));
	}

	auto idx = RtValue::flatten(index);
	if (boxed_list->type() == types::list() && idx.is_tagged_int()) {
		return try_hit(static_cast<PyList *>(boxed_list), idx.as_int());
	}

	return false;
}

}// namespace py::rt
