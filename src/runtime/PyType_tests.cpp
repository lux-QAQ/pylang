#include "PyList.hpp"
#include "PyTuple.hpp"
#include "PyType.hpp"
#include "types/builtin.hpp"


#include "runtime/compat.hpp"

#include "gtest/gtest.h"

using namespace py;

TEST(PyType, ObjectClassParent)
{
	// [[maybe_unused]] auto scope = VirtualMachine::the().heap().scoped_gc_pause();
	PYLANG_GC_PAUSE_SCOPE()
	const auto &bases = types::object()->underlying_type().__bases__;
	EXPECT_TRUE(bases.empty());

	auto mro_ = types::object()->mro();
	ASSERT_TRUE(mro_.is_ok());
	auto *mro = mro_.unwrap();
	ASSERT_TRUE(mro);
	EXPECT_EQ(mro->elements().size(), 1);

	EXPECT_TRUE(mro->elements()[0].is_heap_object());
	auto *mro_0 = mro->elements()[0].as_ptr();
	EXPECT_EQ(mro_0, types::object());
}

namespace {
TypePrototype *new_type(const std::string &name, std::vector<PyType *> bases)
{
	auto *new_type = new TypePrototype{};
	new_type->__name__ = name;
	new_type->__bases__ = std::move(bases);
	return new_type;
}
}// namespace

TEST(PyType, InheritanceTriangle)
{
	//[[maybe_unused]] auto scope = VirtualMachine::the().heap().scoped_gc_pause();
	PYLANG_GC_PAUSE_SCOPE()
	PyType *B1 =
		PyType::initialize(std::unique_ptr<TypePrototype>(new_type("B1", { types::object() })));
	PyType *B2 =
		PyType::initialize(std::unique_ptr<TypePrototype>(new_type("B2", { types::object() })));

	PyType *C = PyType::initialize(std::unique_ptr<TypePrototype>(new_type("C", { B1, B2 })));

	auto C_mro_ = C->mro();
	ASSERT_TRUE(C_mro_.is_ok());
	auto *C_mro = C_mro_.unwrap();
	ASSERT_TRUE(C_mro);
	EXPECT_EQ(C_mro->elements().size(), 4);
	EXPECT_EQ(C_mro->elements()[0].as_ptr(), C);
	EXPECT_EQ(C_mro->elements()[1].as_ptr(), B1);
	EXPECT_EQ(C_mro->elements()[2].as_ptr(), B2);
	EXPECT_EQ(C_mro->elements()[3].as_ptr(), types::object());
}

TEST(PyType, InheritanceDiamond)
{
	// [[maybe_unused]] auto scope = VirtualMachine::the().heap().scoped_gc_pause();
	PYLANG_GC_PAUSE_SCOPE()
	PyType *A =
		PyType::initialize(std::unique_ptr<TypePrototype>(new_type("A", { types::object() })));
	PyType *B1 = PyType::initialize(std::unique_ptr<TypePrototype>(new_type("B1", { A })));
	PyType *B2 = PyType::initialize(std::unique_ptr<TypePrototype>(new_type("B2", { A })));
	PyType *C = PyType::initialize(std::unique_ptr<TypePrototype>(new_type("C", { B1, B2 })));

	auto C_mro_ = C->mro();
	ASSERT_TRUE(C_mro_.is_ok());
	auto *C_mro = C_mro_.unwrap();
	ASSERT_TRUE(C_mro);
	EXPECT_EQ(C_mro->elements().size(), 5);
	EXPECT_EQ(C_mro->elements()[0].as_ptr(), C);
	EXPECT_EQ(C_mro->elements()[1].as_ptr(), B1);
	EXPECT_EQ(C_mro->elements()[2].as_ptr(), B2);
	EXPECT_EQ(C_mro->elements()[3].as_ptr(), A);
	EXPECT_EQ(C_mro->elements()[4].as_ptr(), types::object());
}
