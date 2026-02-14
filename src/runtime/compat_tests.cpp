#include "gtest/gtest.h"

#include "runtime/compat.hpp"
#include "runtime/memory.hpp"
#include "runtime/PyObject.hpp"
#include "types/builtin.hpp"

using namespace py;

// Small test-only type used to exercise PYLANG_ALLOC / make_py behavior.
struct TestCompatObject : public PyBaseObject {
    TestCompatObject() : PyBaseObject(types::object()) {}
    std::string to_string() const override { return "test_compat"; }
};

TEST(Compat, PYLANG_ALLOC_and_PYLANG_CREATE_default)
{
    // PYLANG_ALLOC should compile and return a valid pointer (legacy Heap path)
    auto *raw = PYLANG_ALLOC(TestCompatObject);
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw->to_string(), "test_compat");

    auto *raw2 = PYLANG_CREATE(TestCompatObject);
    ASSERT_NE(raw2, nullptr);
    EXPECT_EQ(raw2->to_string(), "test_compat");

    // make_py should construct a shared_ptr instance correctly
    auto sp = make_py<TestCompatObject>();
    ASSERT_NE(sp, nullptr);
    EXPECT_EQ(sp->to_string(), "test_compat");
}

#ifdef PYLANG_USE_SHARED_PTR
TEST(Compat, Registry_When_SharedPtr_Mode)
{
    // In shared_ptr mode heap_allocate should register the shared handle
    auto *raw = PYLANG_ALLOC(TestCompatObject);
    ASSERT_NE(raw, nullptr);

    auto sp = py::compat::try_get_shared_for_legacy(raw);
    ASSERT_NE(sp, nullptr);
    EXPECT_EQ(sp.get(), raw);

    // deregister
    py::compat::deregister_legacy_ptr(raw);
    EXPECT_EQ(py::compat::legacy_registry_size(), 0u);
}
#endif
