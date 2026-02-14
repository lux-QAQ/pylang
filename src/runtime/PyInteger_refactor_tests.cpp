#include "gtest/gtest.h"

#include "runtime/PyInteger.hpp"

using namespace py;

TEST(PyIntegerRefactor, CreateReturnsCorrectValue)
{
    auto r = PyInteger::create(12345);
    ASSERT_TRUE(r.is_ok());
    auto *i = r.unwrap();
    ASSERT_NE(i, nullptr);
    EXPECT_EQ(i->as_i64(), 12345);
}

#ifdef PYLANG_USE_SHARED_PTR
TEST(PyIntegerRefactor, RegistryWithSharedPtrMode)
{
    auto r = PyInteger::create(42);
    ASSERT_TRUE(r.is_ok());
    auto *raw = r.unwrap();

    auto sp = py::compat::try_get_shared_for_legacy(raw);
    // If the type is constructible in this context, compat layer should register
    EXPECT_NE(sp, nullptr);
    if (sp) { EXPECT_EQ(sp.get(), raw); }
}
#endif
