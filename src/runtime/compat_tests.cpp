#include "gtest/gtest.h"

#include "runtime/compat.hpp"
#include "runtime/PyObject.hpp"
#include "types/builtin.hpp"

using namespace py;

// Small test-only type used to exercise PYLANG_ALLOC behavior.
struct TestCompatObject : public PyBaseObject {
	TestCompatObject() : PyBaseObject(types::object()) {}
	std::string to_string() const override { return "test_compat"; }
};

TEST(Compat, PYLANG_ALLOC_default)
{
	// PYLANG_ALLOC should compile and return a valid pointer (legacy Heap path)
	auto *raw = PYLANG_ALLOC(TestCompatObject);
	ASSERT_NE(raw, nullptr);
	EXPECT_EQ(raw->to_string(), "test_compat");
}
