#pragma once

#include "runtime/PyBool.hpp"
#include "runtime/PyEllipsis.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/Value.hpp"
#include "runtime/types/api.hpp"
#include "utilities.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/PyBool.hpp"
#include "runtime/PyFloat.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"

namespace py {

template<typename T> struct is_vector : std::false_type
{
	static constexpr bool value = false;
};

template<typename T> struct is_vector<std::vector<T>> : std::true_type
{
	using ElementType = T;
	static constexpr bool value = true;
};

template<typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

template<typename T>
concept Vector = is_vector_v<T>;

enum class ValueType {
	INT64 = 0,
	F64 = 1,
	STRING = 2,
	BYTES = 3,
	ELLIPSIS = 4,
	NONE = 5,
	BOOL = 6,
	OBJECT = 7,
	TUPLE = 8,
};

template<typename T> inline void serialize(const T &value, std::vector<uint8_t> &result)
{
	auto *data = bit_cast<const uint8_t *>(&value);
	result.insert(result.end(), data, data + sizeof(T));
}

template<> inline void serialize<Value>(const Value &value, std::vector<uint8_t> &result);

template<> inline void serialize<bool>(const bool &value, std::vector<uint8_t> &result)
{
	result.push_back(value ? 1 : 0);
}

template<> inline void serialize<std::byte>(const std::byte &value, std::vector<uint8_t> &result)
{
	result.push_back(::bit_cast<uint8_t>(value));
}

template<>
inline void serialize<std::string>(const std::string &value, std::vector<uint8_t> &result)
{
	serialize(value.size(), result);
	for (const auto &el : value) { serialize(el, result); }
}

template<Vector VectorType>
inline void serialize(const VectorType &value, std::vector<uint8_t> &result)
{
	serialize(value.size(), result);
	for (const auto &el : value) { serialize(el, result); }
}

template<> inline void serialize<Tuple>(const Tuple &value, std::vector<uint8_t> &result)
{
	serialize(value.elements.size(), result);
	for (const auto &el : value.elements) { serialize(el, result); }
}

template<> inline void serialize<PyTuple *>(PyTuple *const &value, std::vector<uint8_t> &result)
{
	serialize(value->size(), result);
	for (const auto &el : value->elements()) { serialize(el, result); }
}

template<> inline void serialize<Value>(const Value &value, std::vector<uint8_t> &result)
{
	if (value.is_tagged_int()) {
		serialize(static_cast<uint8_t>(ValueType::INT64), result);
		serialize(static_cast<uint64_t>(value.as_int()), result);
		return;
	}

	if (value.is_null()) { TODO(); }

	auto *obj = value.as_ptr();
	if (obj->type() == py::types::float_()) {
		serialize(static_cast<uint8_t>(ValueType::F64), result);
		serialize(as<PyFloat>(obj)->as_f64(), result);
	} else if (obj->type() == py::types::str()) {
		serialize(static_cast<uint8_t>(ValueType::STRING), result);
		serialize(as<PyString>(obj)->value(), result);
	} else if (obj == py_none()) {
		serialize(static_cast<uint8_t>(ValueType::NONE), result);
	} else if (obj->type() == py::types::bool_()) {
		serialize(static_cast<uint8_t>(ValueType::BOOL), result);
		serialize(as<PyBool>(obj)->value(), result);
	} else if (obj == py_ellipsis()) {
		serialize(static_cast<uint8_t>(ValueType::ELLIPSIS), result);
	} else {
		TODO();
	}
}

}// namespace py
