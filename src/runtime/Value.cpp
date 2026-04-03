#include "Value.hpp"
#include "PyBool.hpp"
#include "PyBytes.hpp"
#include "PyEllipsis.hpp"
#include "PyFloat.hpp"
#include "PyInteger.hpp"
#include "PyNone.hpp"
#include "PyNumber.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"
#include "PyType.hpp"
#include "TypeError.hpp"
#include "interpreter/InterpreterCore.hpp"

#include <cstdlib>
#include <iostream>
#include <locale>
#include <ranges>

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>

using namespace py;

std::string Number::to_string() const
{
	return std::visit([](const auto &value) { return fmt::format("{}", value); }, value);
}

Number Number::exp(const Number &rhs) const
{
	return std::visit(
		overloaded{
			[](const mpz_class &lhs_value, const mpz_class &rhs_value) -> Number {
				if (rhs_value.fits_ulong_p()) {
					mpz_class result{};
					mpz_pow_ui(result.get_mpz_t(), lhs_value.get_mpz_t(), rhs_value.get_ui());
					return Number{ std::move(result) };
				} else {
					return Number{ std::pow(lhs_value.get_d(), rhs_value.get_d()) };
				}
			},
			[](const mpz_class &lhs_value, const double &rhs_value) -> Number {
				return Number{ std::pow(lhs_value.get_d(), rhs_value) };
			},
			[](const double &lhs_value, const mpz_class &rhs_value) -> Number {
				return Number{ std::pow(lhs_value, rhs_value.get_d()) };
			},
			[](const double &lhs_value, const double &rhs_value) -> Number {
				return Number{ std::pow(lhs_value, rhs_value) };
			},
		},
		value,
		rhs.value);
}

Number Number::operator+(const Number &rhs) const
{
	auto result = *this;
	return result += rhs;
}

Number &Number::operator+=(const Number &rhs)
{
	std::visit(
		overloaded{
			[](mpz_class &lhs_value, const mpz_class &rhs_value) -> void {
				lhs_value += rhs_value;
			},
			[this](mpz_class lhs_value, const double &rhs_value) -> void {
				this->value = lhs_value.get_d() + rhs_value;
			},
			[](double &lhs_value, const mpz_class &rhs_value) -> void {
				lhs_value += rhs_value.get_d();
			},
			[](double &lhs_value, const double &rhs_value) -> void { lhs_value += rhs_value; },
		},
		value,
		rhs.value);
	return *this;
}

Number Number::operator-(const Number &rhs) const
{
	return std::visit(overloaded{
						  [](const mpz_class &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value - rhs_value };
						  },
						  [](const mpz_class &lhs_value, const double &rhs_value) -> Number {
							  return Number{ lhs_value.get_d() - rhs_value };
						  },
						  [](const double &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value - rhs_value.get_d() };
						  },
						  [](const double &lhs_value, const double &rhs_value) -> Number {
							  return Number{ lhs_value - rhs_value };
						  },
					  },
		value,
		rhs.value);
}

Number Number::operator%(const Number &rhs) const
{
	return std::visit(overloaded{
						  [](const mpz_class &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value % rhs_value };
						  },
						  [](const mpz_class &lhs_value, const double &rhs_value) -> Number {
							  return Number{ std::remainder(lhs_value.get_d(), rhs_value) };
						  },
						  [](const double &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ std::remainder(lhs_value, rhs_value.get_d()) };
						  },
						  [](const double &lhs_value, const double &rhs_value) -> Number {
							  return Number{ std::remainder(lhs_value, rhs_value) };
						  },
					  },
		value,
		rhs.value);
}

Number Number::operator/(const Number &other) const
{
	return std::visit(overloaded{
						  [](const mpz_class &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value.get_d() / rhs_value.get_d() };
						  },
						  [](const mpz_class &lhs_value, const double &rhs_value) -> Number {
							  return Number{ lhs_value.get_d() / rhs_value };
						  },
						  [](const double &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value / rhs_value.get_d() };
						  },
						  [](const double &lhs_value, const double &rhs_value) -> Number {
							  return Number{ lhs_value / rhs_value };
						  },
					  },
		value,
		other.value);
}

Number Number::operator*(const Number &other) const
{
	return std::visit(overloaded{
						  [](const mpz_class &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value * rhs_value };
						  },
						  [](const mpz_class &lhs_value, const double &rhs_value) -> Number {
							  return Number{ lhs_value.get_d() * rhs_value };
						  },
						  [](const double &lhs_value, const mpz_class &rhs_value) -> Number {
							  return Number{ lhs_value * rhs_value.get_d() };
						  },
						  [](const double &lhs_value, const double &rhs_value) -> Number {
							  return Number{ lhs_value * rhs_value };
						  },
					  },
		value,
		other.value);
}

bool Number::operator==(const PyObject *other) const
{
	if (auto other_pynumber = as<PyInteger>(other)) {
		return *this == other_pynumber->value();
	} else if (auto other_pynumber = as<PyFloat>(other)) {
		return *this == other_pynumber->value();
	} else {
		return false;
	}
}


bool Number::operator==(const Number &rhs) const
{
	return std::visit(
		[](const auto &lhs_value, const auto &rhs_value) { return lhs_value == rhs_value; },
		value,
		rhs.value);
}

Number Number::operator<<(const Number &rhs) const
{
	return std::visit(overloaded{
						  [](const mpz_class &lhs_value, const mpz_class &rhs_value) -> Number {
							  ASSERT(rhs_value.fits_ulong_p());
							  return Number{ lhs_value << rhs_value.get_ui() };
						  },
						  [](const mpz_class &, const double &) -> Number {
							  // should raise error
							  TODO();
						  },
						  [](const double &, const mpz_class &) -> Number {
							  // should raise error
							  TODO();
						  },
						  [](const double &, const double &) -> Number {
							  // should raise error
							  TODO();
						  },
					  },
		value,
		rhs.value);
}

Number Number::operator>>(const Number &rhs) const
{
	return std::visit(overloaded{
						  [](const BigIntType &lhs_value, const BigIntType &rhs_value) -> Number {
							  ASSERT(rhs_value.fits_ulong_p());
							  return Number{ lhs_value >> rhs_value.get_ui() };
						  },
						  [](const BigIntType &, const double &) -> Number {
							  // should raise error
							  TODO();
						  },
						  [](const double &, const BigIntType &) -> Number {
							  // should raise error
							  TODO();
						  },
						  [](const double &, const double &) -> Number {
							  // should raise error
							  TODO();
						  },
					  },
		value,
		rhs.value);
}

bool Number::operator<=(const Number &rhs) const
{
	return std::visit(
		[](const auto &lhs_value, const auto &rhs_value) -> bool { return lhs_value <= rhs_value; },
		value,
		rhs.value);
}
bool Number::operator<(const Number &rhs) const
{
	return std::visit(
		[](const auto &lhs_value, const auto &rhs_value) -> bool { return lhs_value < rhs_value; },
		value,
		rhs.value);
}

bool Number::operator>(const Number &rhs) const
{
	return std::visit(
		[](const auto &lhs_value, const auto &rhs_value) -> bool { return lhs_value > rhs_value; },
		value,
		rhs.value);
}

bool Number::operator>=(const Number &rhs) const
{
	return std::visit(
		[](const auto &lhs_value, const auto &rhs_value) -> bool { return lhs_value >= rhs_value; },
		value,
		rhs.value);
}

Number Number::floordiv(const Number &other) const
{
	return std::visit(
		overloaded{
			[](const double &lhs, const BigIntType &rhs) -> Number {
				return Number{ BigIntType{ BigIntType{ lhs } / rhs }.get_d() };
			},
			[](const BigIntType &lhs, const double &rhs) -> Number {
				return Number{ BigIntType{ lhs / BigIntType{ rhs } }.get_d() };
			},
			[](const BigIntType &lhs, const BigIntType &rhs) -> Number {
				return Number{ BigIntType{ lhs / rhs } };
			},
			[](const double &lhs, const double &rhs) -> Number {
				return Number{ BigIntType{ BigIntType{ lhs } / BigIntType{ rhs } }.get_d() };
			},
		},
		value,
		other.value);
}

String String::from_unescaped_string(const std::string &str)
{
	std::string output;
	auto it = str.begin();
	const auto end = str.end();
	while (it != end) {
		if (auto c = *it++; c != '\\') {
			output.push_back(static_cast<unsigned char>(c));
			continue;
		}

		if (it == end) {
			// return Err(value_error("Trailing \\ in string"));
			TODO();
		}
		switch (*it++) {
		case '\n':
			break;
		case '\\': {
			output.push_back('\\');
		} break;
		case '\'': {
			output.push_back('\'');
		} break;
		case '\"': {
			output.push_back('\"');
		} break;
		case 'b': {
			output.push_back('\b');
		} break;
		case 'f': {
			output.push_back('\014');
		} break;
		case 't': {
			output.push_back('\t');
		} break;
		case 'n': {
			output.push_back('\n');
		} break;
		case 'r': {
			output.push_back('\r');
		} break;
		case 'v': {
			output.push_back('\013');
		} break;
		case 'a': {
			output.push_back('\007');
		} break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': {
			auto c = *(it - 1) - '0';
			if (it != end && '0' <= *it && *it <= '7') {
				c = (c << 3) + *it++ - '0';
				if (it != end && '0' <= *it && *it <= '7') { c = (c << 3) + *it++ - '0'; }
			}
			ASSERT(c <= static_cast<int>(std::numeric_limits<unsigned char>::max()));
			output.push_back(static_cast<unsigned char>(c));
		} break;
		case 'x': {
			// two digit hex sequence representing unicode code point
			auto start = it;
			if (start == end || std::next(start) == end) { TODO(); }
			it++;
			it++;
			std::string v{ start, it };
			int c;
			std::istringstream(v) >> std::hex >> c;
			ASSERT(c <= static_cast<int>(std::numeric_limits<unsigned char>::max()));
			const auto l = c <= 0x7F ? 1 : 2;
			if (l == 1) {
				output.push_back(static_cast<unsigned char>(c));
			} else {
				output.push_back(0xC0 | (static_cast<unsigned char>(c) >> 6));
				output.push_back(0x80 | (static_cast<unsigned char>(c) & 0x3F));
			}
		} break;
		case 'u': {
			// four digit hex sequence representing unicode code point
			std::string_view sequence{ it, it + 4 };
			char *p_end = nullptr;
			const auto cp = std::strtol(sequence.data(), &p_end, 16);
			if (sequence.data() == p_end) {
				// could not parse hex
				TODO();
			}
			std::string tmp_result;
			icu::UnicodeString{ UChar32(cp) }.toUTF8String(output);
			it += 4;
		} break;
		case 'U': {
			// eight digit hex sequence representing unicode code point
			std::string_view sequence{ it, it + 8 };
			char *p_end = nullptr;
			const auto cp = std::strtol(sequence.data(), &p_end, 16);
			if (sequence.data() == p_end) {
				// could not parse hex
				TODO();
			}
			std::string tmp_result;
			icu::UnicodeString{ UChar32(cp) }.toUTF8String(output);
			it += 8;
		} break;
		case 'N': {
			// lookup unicode name from Unicode Character Database (UCD)
			// \N{name}
			if (*it != '{') {
				// malformed
				TODO();
			}
			it++;
			auto name_end = std::find(it, end, '}');
			if (name_end == end) {
				// malformed
				TODO();
			}
			// use std::string to get null terminated string, string_view would not work here (at
			// least most of the time)
			std::string name{ it, end - 1 };
			UErrorCode error_code;
			const auto cp = u_charFromName(U_CHAR_NAME_ALIAS, name.data(), &error_code);
			if (U_FAILURE(error_code)) {
				std::cerr << u_errorName(error_code) << std::endl;
				TODO();
			}
			std::string tmp_result;
			icu::UnicodeString{ cp }.toUTF8String(output);
		} break;
		default: {
			output.push_back('\\');
			output.push_back(static_cast<unsigned char>(*(it - 1)));
		} break;
		}
	}

	return String{ std::move(output) };
}

bool String::operator==(const PyObject *other) const
{
	if (auto other_pystr = as<PyString>(other)) {
		return s == other_pystr->value();
	} else {
		return false;
	}
}


bool Bytes::operator==(const PyObject *other) const
{
	if (auto other_pybytes = as<PyBytes>(other)) {
		return *this == other_pybytes->value();
	} else {
		return false;
	}
}

constexpr unsigned char to_digit_value(char value)
{
	if (value >= 0 && value <= 9) { return value; }
	if (value >= 'a' && value <= 'z') { return static_cast<unsigned char>((value - 'a') + 10); }
	if (value >= 'A' && value <= 'Z') { return static_cast<unsigned char>((value - 'A') + 10); }
	return 37;
}

Bytes Bytes::from_unescaped_string(const std::string &str)
{
	std::vector<std::byte> bytes;
	auto it = str.begin();
	const auto end = str.end();
	while (it != end) {
		if (auto c = *it++; c != '\\') {
			bytes.push_back(std::byte{ static_cast<unsigned char>(c) });
			continue;
		}

		if (it == end) {
			// return Err(value_error("Trailing \\ in string"));
			TODO();
		}

		switch (*it++) {
		case '\n':
			break;
		case '\\': {
			bytes.emplace_back(std::byte{ '\\' });
		} break;
		case '\'': {
			bytes.emplace_back(std::byte{ '\'' });
		} break;
		case '\"': {
			bytes.emplace_back(std::byte{ '\"' });
		} break;
		case 'b': {
			bytes.emplace_back(std::byte{ '\b' });
		} break;
		case 'f': {
			bytes.emplace_back(std::byte{ '\014' });
		} break;
		case 't': {
			bytes.emplace_back(std::byte{ '\t' });
		} break;
		case 'n': {
			bytes.emplace_back(std::byte{ '\n' });
		} break;
		case 'r': {
			bytes.emplace_back(std::byte{ '\r' });
		} break;
		case 'v': {
			bytes.emplace_back(std::byte{ '\013' });
		} break;
		case 'a': {
			bytes.emplace_back(std::byte{ '\007' });
		} break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': {
			auto c = *(it - 1) - '0';
			if (it != end && '0' <= *it && *it <= '7') {
				c = (c << 3) + *it++ - '0';
				if (it != end && '0' <= *it && *it <= '7') { c = (c << 3) + *it++ - '0'; }
			}
			ASSERT(c < static_cast<int>(std::numeric_limits<unsigned char>::max()));
			bytes.push_back(std::byte{ static_cast<unsigned char>(c) });
		} break;
		case 'x': {
			ASSERT(it + 1 < end);
			const auto digit1 = to_digit_value(*it);
			const auto digit2 = to_digit_value(*(it + 1));
			if (digit1 < 16 && digit2 < 16) {
				it += 2;
				const auto c = static_cast<unsigned char>((digit1 << 4) + digit2);
				bytes.push_back(std::byte{ c });
			}
			auto &f = std::use_facet<std::ctype<char>>(std::locale());
			/* skip \x */
			if (it < end && f.is(std::ctype<char>::xdigit, *it)) {
				it++;// and a hexdigit
			}
		} break;
		default: {
			bytes.push_back(std::byte{ '\\' });
			--it;
		} break;
		}
	}

	return Bytes{ std::move(bytes) };
}


std::string Bytes::to_string() const
{
	static constexpr std::string_view hex_digits = "0123456789abcdef";
	std::ostringstream os;
	os << "b'";
	for (const auto &byte : b) {
		const auto byte_ = std::to_integer<unsigned char>(byte);
		if (byte_ == '\\') {
			os << "\\";
		} else if (byte_ == '\t') {
			os << "\\t";
		} else if (byte_ == '\n') {
			os << "\\n";
		} else if (byte_ == '\r') {
			os << "\\r";
		} else if (byte_ < ' ' || byte_ >= 0x7f) {
			os << "\\x" << hex_digits[(byte_ & 0xf0) >> 4] << hex_digits[byte_ & 0xf];
		} else {
			os << byte_;
		}
	}
	os << "'";
	return os.str();
}

bool Ellipsis::operator==(const PyObject *other) const { return other == py_ellipsis(); }

bool NoneType::operator==(const PyObject *other) const { return other == py_none(); }

bool NameConstant::operator==(const NameConstant &other) const { return value == other.value; }

bool NameConstant::operator==(const PyObject *other) const
{
	if (std::holds_alternative<NoneType>(value)) return other == py_none();
	if (std::get<bool>(value)) return other == py_true();
	return other == py_false();
}

bool NameConstant::operator==(const Number &other) const
{
	if (std::holds_alternative<NoneType>(value)) return false;
	const bool b = std::get<bool>(value);
	if (b) return other == Number{ 1 };
	return other == Number{ 0 };
}

bool Number::operator==(const NameConstant &other) const { return other == *this; }

std::string Tuple::to_string() const
{
	std::ostringstream os;
	os << "(";

	// 这里曾经是导致无法编译的地方: std::visit 解包
	// 现在直接交由 PyObject 装箱打印处理
	for (const auto &el : elements | std::ranges::views::drop(1)) {
		os << py::ensure_box(el.as_pyobject_raw())->to_string();
		os << ", ";
	}
	if (!elements.empty()) { os << py::ensure_box(elements.back().as_pyobject_raw())->to_string(); }
	os << ")";
	return os.str();
}


namespace py {

std::ostream &operator<<(std::ostream &os, const Tuple &tuple)
{
	os << tuple.to_string();
	return os;
}

PyResult<RtValue> add(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->add(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> subtract(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->subtract(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> multiply(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->multiply(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> exp(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->pow(rhs.box(), py_none()).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> lshift(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->lshift(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> rshift(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->rshift(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> modulo(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->modulo(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> true_divide(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->truediv(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> floordiv(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->floordiv(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> equals(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_EQ).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> equals(const RtValue &lhs, const RtValue &rhs)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_EQ).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> not_equals(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_NE).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> less_than_equals(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_LE).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> less_than(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_LT).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> greater_than(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_GT).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> greater_than_equals(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->richcompare(rhs.box(), RichCompare::Py_GE).and_then([](PyObject *obj) {
		return Ok(RtValue::from_ptr(obj));
	});
}

PyResult<RtValue> and_(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->and_(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> or_(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->or_(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<RtValue> xor_(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->xor_(rhs.box()).and_then(
		[](PyObject *obj) { return Ok(RtValue::from_ptr(obj)); });
}

PyResult<bool> is(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return Ok(lhs.box() == rhs.box());
}

PyResult<bool> in(const RtValue &lhs, const RtValue &rhs, Interpreter &)
{
	return lhs.box()->contains(rhs.box());
}

PyResult<bool> truthy(const RtValue &value, Interpreter &) { return truthy(value); }

PyResult<bool> truthy(const RtValue &value) { return value.box()->true_(); }

bool operator==(const RtValue &lhs_value, const RtValue &rhs_value)
{
	auto r = lhs_value.box()->richcompare(rhs_value.box(), RichCompare::Py_EQ);
	if (r.is_err()) return false;
	return r.unwrap() == py_true();
}

}// namespace py