#include "runtime/PyBool.hpp"
#include "runtime/PyFloat.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/Value.hpp"

#include <gtest/gtest.h>

using namespace py;

// =============================================================================
// truthy(const Value&) — 不依赖 Interpreter 的独立路径
// =============================================================================

TEST(ValueTruthy, NoneIsFalsy)
{
	Value v{ py_none() };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, TrueIsTruthy)
{
	Value v{ py_true() };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, FalseIsFalsy)
{
	Value v{ py_false() };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, ZeroIntegerIsFalsy)
{
	auto *zero = PyInteger::create(0).unwrap();
	Value v{ zero };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NonZeroIntegerIsTruthy)
{
	auto *one = PyInteger::create(1).unwrap();
	Value v{ one };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, NegativeIntegerIsTruthy)
{
	auto *neg = PyInteger::create(-42).unwrap();
	Value v{ neg };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, ZeroFloatIsFalsy)
{
	auto *zero = PyFloat::create(0.0).unwrap();
	Value v{ zero };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NonZeroFloatIsTruthy)
{
	auto *f = PyFloat::create(3.14).unwrap();
	Value v{ f };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, EmptyStringIsFalsy)
{
	auto *s = PyString::create("").unwrap();
	Value v{ s };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NonEmptyStringIsTruthy)
{
	auto *s = PyString::create("hello").unwrap();
	Value v{ s };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, EmptyListIsFalsy)
{
	auto *l = PyList::create().unwrap();
	Value v{ l };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NonEmptyListIsTruthy)
{
	auto *l = PyList::create().unwrap();
	l->append(py_none());
	Value v{ l };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, NameConstantTrue)
{
	Value v{ py_true() };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}

TEST(ValueTruthy, NameConstantFalse)
{
	Value v{ py_false() };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NameConstantNone)
{
	Value v{ py_none() };
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NumberZero)
{
	Value v = RtValue::from_int(0);
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_FALSE(r.unwrap());
}

TEST(ValueTruthy, NumberNonZero)
{
	Value v = RtValue::from_int(42);
	auto r = truthy(v);
	ASSERT_TRUE(r.is_ok());
	EXPECT_TRUE(r.unwrap());
}