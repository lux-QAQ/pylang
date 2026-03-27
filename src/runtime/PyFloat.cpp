#include "PyFloat.hpp"
#include "MemoryError.hpp"
#include "PyBytes.hpp"
#include "runtime/PyNumber.hpp"
#include "runtime/PyString.hpp"
#include "runtime/TypeError.hpp"
#include "runtime/Value.hpp"
#include "runtime/ValueError.hpp"
#include "runtime/forward.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


#include "runtime/compat.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sstream>

namespace py {

template<> PyFloat *as(PyObject *obj)
{
	if (obj->type() == types::float_()) { return static_cast<PyFloat *>(obj); }
	return nullptr;
}

template<> const PyFloat *as(const PyObject *obj)
{
	if (obj->type() == types::float_()) { return static_cast<const PyFloat *>(obj); }
	return nullptr;
}

namespace {

	constexpr int kPyHashBits = (sizeof(void *) >= 8) ? 61 : 31;
	constexpr uint64_t kPyHashModulus = (uint64_t{ 1 } << kPyHashBits) - 1;
	constexpr int64_t kPyHashInf = 314159;
	constexpr int64_t kPyHashNan = 0;
	constexpr double kLog10_2UpperBound = 0.30103;

	inline int64_t normalize_python_hash(int64_t hash) noexcept { return hash == -1 ? -2 : hash; }

	inline bool is_finite(double value) noexcept { return std::isfinite(value); }

	inline bool is_halfway(double original, double rounded) noexcept
	{
		return std::fabs(original - rounded) == 0.5;
	}

	std::string trim_ascii_whitespace(const std::string &value)
	{
		const auto begin = value.find_first_not_of(" \t\n\r\f\v");
		if (begin == std::string::npos) { return ""; }
		const auto end = value.find_last_not_of(" \t\n\r\f\v");
		return value.substr(begin, end - begin + 1);
	}

	PyResult<PyFloat *> create_float_for_type(PyType *type, double value)
	{
		return PyFloat::create(type, value);
	}

	std::string float_to_hex(double value)
	{
		if (std::isnan(value)) { return "nan"; }
		if (std::isinf(value)) { return value < 0 ? "-inf" : "inf"; }
		if (value == 0.0) { return std::signbit(value) ? "-0x0.0p+0" : "0x0.0p+0"; }

		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "%a", value);
		std::string result(buffer);
		const auto exponent_pos = result.find('p');
		if (exponent_pos != std::string::npos && exponent_pos + 1 < result.size()) {
			if (result[exponent_pos + 1] != '+' && result[exponent_pos + 1] != '-') {
				result.insert(exponent_pos + 1, "+");
			}
		}
		return result;
	}

	PyResult<PyString *> get_string_argument(PyObject *value, std::string_view function_name)
	{
		if (auto *str = as<PyString>(value)) { return Ok(str); }
		if (auto *bytes = as<PyBytes>(value)) {
			return PyString::decode(bytes->value().b, "ascii", "strict");
		}
		return Err(type_error("{}() argument must be a string or bytes-like object, not '{}'",
			function_name,
			value->type()->name()));
	}

}// namespace

PyFloat::PyFloat(PyType *type) : PyNumber(type) {}

PyFloat::PyFloat(double value) : PyNumber(Number{ value }, types::BuiltinTypes::the().float_()) {}

PyResult<PyObject *> PyFloat::__new__(const PyType *type, PyTuple *args, PyDict *kwargs)
{
	// TODO: support inheriting from float
	ASSERT(type == types::float_());

	ASSERT(!kwargs || kwargs->map().empty());
	if (!args || args->elements().empty()) { return PyFloat::create(0.0); }
	if (args->elements().size() > 1) {
		return Err(
			type_error("float expected at most 1 argument, got {}", args->elements().size()));
	}

	PyObject *value = nullptr;
	if (auto obj = PyObject::from(args->elements()[0]); obj.is_ok()) {
		value = obj.unwrap();
	} else {
		return obj;
	}

	if (auto *int_value = as<PyInteger>(value)) {
		return PyFloat::create(int_value->as_big_int().get_d());
	}
	if (auto *float_value = as<PyFloat>(value)) { return PyFloat::create(float_value->as_f64()); }
	if (auto *str_value = as<PyString>(value)) {
		const auto source = trim_ascii_whitespace(str_value->value());
		if (source.empty()) { return Err(value_error("could not convert string to float: ''")); }

		char *end = nullptr;
		errno = 0;
		const double parsed = std::strtod(source.c_str(), &end);
		if (end == source.c_str() || (end && *end != '\0')) {
			return Err(value_error("could not convert string to float: '{}'", str_value->value()));
		}
		return PyFloat::create(parsed);
	}
	if (auto *bytes_value = as<PyBytes>(value)) {
		return PyString::decode(bytes_value->value().b, "ascii", "strict")
			.and_then([type](PyString *decoded) -> PyResult<PyObject *> {
				auto decoded_args = PyTuple::create(decoded);
				if (decoded_args.is_err()) { return Err(decoded_args.unwrap_err()); }
				return PyFloat::__new__(type, decoded_args.unwrap(), nullptr);
			});
	}

	return Err(type_error(
		"float() argument must be a string or a real number, not '{}'", value->type()->name()));
}

PyResult<PyFloat *> PyFloat::create(double value)
{

	auto *obj = PYLANG_ALLOC(PyFloat, value);
	if (!obj) { return Err(memory_error(sizeof(PyFloat))); }
	return Ok(obj);
}

PyResult<PyFloat *> PyFloat::create(PyType *type, double value)
{
	if (type == types::float_()) { return create(value); }
	ASSERT(type->issubclass(types::float_()));
	auto result = type->underlying_type().__alloc__(type);
	return result.and_then([value](PyObject *obj) -> PyResult<PyFloat *> {
		static_cast<PyFloat &>(*obj).m_value = Number{ value };
		return Ok(static_cast<PyFloat *>(obj));
	});
}

int64_t PyFloat::hash_f64(double value)
{
	if (!is_finite(value)) {
		if (std::isinf(value)) { return value > 0 ? kPyHashInf : -kPyHashInf; }
		return kPyHashNan;
	}

	int exponent = 0;
	double mantissa = std::frexp(value, &exponent);
	int sign = 1;
	if (mantissa < 0.0) {
		sign = -1;
		mantissa = -mantissa;
	}

	uint64_t hash = 0;
	while (mantissa != 0.0) {
		hash = ((hash << 28U) & kPyHashModulus) | (hash >> (kPyHashBits - 28));
		mantissa *= 268435456.0;
		exponent -= 28;
		const auto integral = static_cast<uint64_t>(mantissa);
		mantissa -= static_cast<double>(integral);
		hash += integral;
		if (hash >= kPyHashModulus) { hash -= kPyHashModulus; }
	}

	exponent =
		exponent >= 0 ? exponent % kPyHashBits : kPyHashBits - 1 - ((-1 - exponent) % kPyHashBits);
	hash = ((hash << exponent) & kPyHashModulus) | (hash >> (kPyHashBits - exponent));

	return normalize_python_hash(static_cast<int64_t>(hash) * sign);
}

PyResult<int64_t> PyFloat::__hash__() const { return Ok(hash_f64(as_f64())); }

PyResult<PyObject *> PyFloat::__round__(PyObject *ndigits_obj) const
{
	const double value = as_f64();
	if (!ndigits_obj || ndigits_obj == py_none()) {
		double rounded = std::round(value);
		if (is_halfway(value, rounded)) { rounded = 2.0 * std::round(value / 2.0); }
		return PyInteger::create(BigIntType{ rounded });
	}

	if (!ndigits_obj->type()->issubclass(types::integer())) {
		return Err(type_error(
			"'{}' object cannot be interpreted as an integer", ndigits_obj->type()->name()));
	}

	if (!is_finite(value)) { return PyFloat::create(value); }

	auto ndigits_big = static_cast<const PyInteger &>(*ndigits_obj).as_big_int();
	const auto ndigits = ndigits_big.get_si();

	constexpr int ndigits_max = static_cast<int>((DBL_MANT_DIG - DBL_MIN_EXP) * kLog10_2UpperBound);
	constexpr int ndigits_min = -static_cast<int>((DBL_MAX_EXP + 1) * kLog10_2UpperBound);

	if (ndigits > ndigits_max) { return PyFloat::create(value); }
	if (ndigits < ndigits_min) { return PyFloat::create(0.0 * value); }

	double pow1 = 1.0;
	double pow2 = 1.0;
	double scaled = 0.0;
	if (ndigits >= 0) {
		if (ndigits > 22) {
			pow1 = std::pow(10.0, static_cast<double>(ndigits - 22));
			pow2 = 1e22;
		} else {
			pow1 = std::pow(10.0, static_cast<double>(ndigits));
		}
		scaled = (value * pow1) * pow2;
		if (!is_finite(scaled)) { return PyFloat::create(value); }
	} else {
		pow1 = std::pow(10.0, static_cast<double>(-ndigits));
		scaled = value / pow1;
	}

	double rounded = std::round(scaled);
	if (is_halfway(scaled, rounded)) { rounded = 2.0 * std::round(scaled / 2.0); }

	if (ndigits >= 0) {
		rounded = (rounded / pow2) / pow1;
	} else {
		rounded *= pow1;
	}

	if (!is_finite(rounded)) { return Err(value_error("overflow occurred during round")); }
	return PyFloat::create(rounded);
}

PyResult<PyObject *> PyFloat::__floor__() const
{
	return PyInteger::create(BigIntType{ std::floor(as_f64()) });
}

PyResult<PyObject *> PyFloat::__ceil__() const
{
	return PyInteger::create(BigIntType{ std::ceil(as_f64()) });
}

PyResult<PyObject *> PyFloat::__trunc__() const
{
	return PyInteger::create(BigIntType{ as_f64() });
}

PyResult<PyObject *> PyFloat::is_integer() const
{
	const double value = as_f64();
	if (!is_finite(value)) { return PyObject::from(NameConstant{ false }); }
	return PyObject::from(NameConstant{ std::floor(value) == value });
}

PyResult<PyObject *> PyFloat::hex() const { return PyString::create(float_to_hex(as_f64())); }

PyResult<PyObject *> PyFloat::as_integer_ratio() const
{
	const double value = as_f64();
	if (std::isinf(value)) { return Err(value_error("cannot convert Infinity to integer ratio")); }
	if (std::isnan(value)) { return Err(value_error("cannot convert NaN to integer ratio")); }

	double float_part = value;
	int exponent = 0;
	float_part = std::frexp(float_part, &exponent);
	for (int i = 0; i < 300 && float_part != std::floor(float_part); ++i) {
		float_part *= 2.0;
		--exponent;
	}

	auto numerator = PyInteger::create(BigIntType{ float_part });
	if (numerator.is_err()) { return Err(numerator.unwrap_err()); }
	auto denominator = PyInteger::create(1);
	if (denominator.is_err()) { return Err(denominator.unwrap_err()); }
	auto shift = PyInteger::create(static_cast<int64_t>(std::abs(exponent)));
	if (shift.is_err()) { return Err(shift.unwrap_err()); }

	PyResult<PyObject *> adjusted = exponent > 0 ? numerator.unwrap()->__lshift__(shift.unwrap())
												 : denominator.unwrap()->__lshift__(shift.unwrap());
	if (adjusted.is_err()) { return adjusted; }

	if (exponent > 0) { return PyTuple::create(adjusted.unwrap(), denominator.unwrap()); }
	return PyTuple::create(numerator.unwrap(), adjusted.unwrap());
}

PyResult<PyObject *> PyFloat::fromhex(PyType *type, PyTuple *args, PyDict *kwargs)
{
	ASSERT(!kwargs || kwargs->map().empty());
	if (!args || args->size() != 1) {
		return Err(type_error("fromhex expected exactly one argument"));
	}

	auto value = PyObject::from(args->elements()[0]);
	if (value.is_err()) { return value; }
	auto str_value = get_string_argument(value.unwrap(), "fromhex");
	if (str_value.is_err()) { return Err(str_value.unwrap_err()); }

	const auto source = trim_ascii_whitespace(str_value.unwrap()->value());
	if (source.empty()) { return Err(value_error("invalid hexadecimal floating-point string")); }

	char *end = nullptr;
	errno = 0;
	const double parsed = std::strtod(source.c_str(), &end);
	if (end == source.c_str() || (end && *end != '\0')) {
		return Err(value_error("invalid hexadecimal floating-point string"));
	}
	if (errno == ERANGE && !std::isfinite(parsed) && source.find("inf") == std::string::npos
		&& source.find("INF") == std::string::npos) {
		return Err(value_error("hexadecimal value too large to represent as a float"));
	}

	return create_float_for_type(type, parsed);
}


/*
PyType *PyFloat::static_type() const { return types::float_(); }
*/

double PyFloat::as_f64() const
{
	ASSERT(std::holds_alternative<double>(m_value.value));
	return std::get<double>(m_value.value);
}

namespace {

	std::once_flag float_flag;

	std::unique_ptr<TypePrototype> register_float()
	{
		return std::move(klass<PyFloat>("float")
				.def("__round__", &PyFloat::__round__)
				.def("__floor__", &PyFloat::__floor__)
				.def("__ceil__", &PyFloat::__ceil__)
				.def("__trunc__", &PyFloat::__trunc__)
				.def("is_integer", &PyFloat::is_integer)
				.def("hex", &PyFloat::hex)
				.def("as_integer_ratio", &PyFloat::as_integer_ratio)
				.classmethod("fromhex", &PyFloat::fromhex)
				.type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyFloat::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(float_flag, []() { type = register_float(); });
		return std::move(type);
	};
}

}// namespace py
