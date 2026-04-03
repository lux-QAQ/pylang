#include "PyDict.hpp"
#include "PyNone.hpp"
#include "PyString.hpp"

#include <gtest/gtest.h>

using namespace py;

TEST(PyDict, StringLookup)
{
	auto dict_ = PyDict::create();
	ASSERT_TRUE(dict_.is_ok());
	auto *dict = dict_.unwrap();
	dict->insert(RtValue::from_ptr(PyString::create("foo").unwrap()), py_none());
	ASSERT_TRUE(dict->map().contains(RtValue::from_ptr(PyString::create("foo").unwrap())));

	auto foo_ = PyString::create("foo");
	ASSERT_TRUE(foo_.is_ok());
	auto *foo = foo_.unwrap();
	ASSERT_TRUE(dict->map().contains(foo));
}