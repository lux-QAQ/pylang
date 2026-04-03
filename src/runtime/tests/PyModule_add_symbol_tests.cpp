#include "runtime/PyDict.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/Value.hpp"

#include <gtest/gtest.h>

using namespace py;

class PyModuleAddSymbolTest : public ::testing::Test
{
  protected:
	PyModule *mod{ nullptr };

	void SetUp() override
	{
		mod = PyModule::create(PyDict::create().unwrap(),
			PyString::create("test").unwrap(),
			PyString::create("").unwrap())
				  .unwrap();
	}
};

TEST_F(PyModuleAddSymbolTest, ValueActuallyInserted)
{
	auto *key = PyString::create("x").unwrap();
	auto *val = PyInteger::create(42).unwrap();
	mod->add_symbol(key, Value{ val });

	auto &map = mod->symbol_table()->map();
	auto it = map.find(key);
	ASSERT_NE(it, map.end());

	auto obj = PyObject::from(it->second);
	ASSERT_TRUE(obj.is_ok());
	EXPECT_EQ(obj.unwrap(), val);
}

TEST_F(PyModuleAddSymbolTest, OverwriteUpdatesValue)
{
	auto *key = PyString::create("x").unwrap();
	auto *val1 = PyInteger::create(1).unwrap();
	auto *val2 = PyInteger::create(2).unwrap();

	mod->add_symbol(key, Value{ val1 });
	mod->add_symbol(key, Value{ val2 });

	auto &map = mod->symbol_table()->map();
	auto it = map.find(key);
	ASSERT_NE(it, map.end());

	auto obj = PyObject::from(it->second);
	ASSERT_TRUE(obj.is_ok());
	EXPECT_EQ(obj.unwrap(), val2);
}

TEST_F(PyModuleAddSymbolTest, VersionAndValueConsistent)
{
	auto *key = PyString::create("counter").unwrap();

	for (int i = 0; i < 100; ++i) {
		auto *val = PyInteger::create(i).unwrap();
		mod->add_symbol(key, Value{ val });

		// 版本号 == 写入次数
		EXPECT_EQ(mod->get_key_version("counter"), static_cast<uint64_t>(i + 1));

		// 值正确
		auto &map = mod->symbol_table()->map();
		auto it = map.find(key);
		ASSERT_NE(it, map.end());
		auto obj = PyObject::from(it->second);
		ASSERT_TRUE(obj.is_ok());
		EXPECT_EQ(as<PyInteger>(obj.unwrap())->as_i64(), i);
	}
}

TEST_F(PyModuleAddSymbolTest, MultipleKeysAllPresent)
{
	constexpr int N = 50;
	for (int i = 0; i < N; ++i) {
		auto *key = PyString::create("var_" + std::to_string(i)).unwrap();
		auto *val = PyInteger::create(i * 10).unwrap();
		mod->add_symbol(key, Value{ val });
	}

	auto &map = mod->symbol_table()->map();
	for (int i = 0; i < N; ++i) {
		auto *key = PyString::create("var_" + std::to_string(i)).unwrap();
		auto it = map.find(key);
		ASSERT_NE(it, map.end()) << "Missing key: var_" << i;

		auto obj = PyObject::from(it->second);
		ASSERT_TRUE(obj.is_ok());
		EXPECT_EQ(as<PyInteger>(obj.unwrap())->as_i64(), i * 10);
	}
}

TEST_F(PyModuleAddSymbolTest, ConstructorSymbolsPreserved)
{
	// 构造函数插入的 __name__, __doc__ 等不应被 add_symbol 破坏
	auto &map = mod->symbol_table()->map();
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("__name__").unwrap())));
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("__doc__").unwrap())));
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("__package__").unwrap())));
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("__loader__").unwrap())));
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("__spec__").unwrap())));

	// 新增 key 后原有 key 仍在
	mod->add_symbol(PyString::create("new_key").unwrap(), Value{ py_none() });
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("__name__").unwrap())));
	EXPECT_TRUE(map.contains(RtValue::from_ptr(PyString::create("new_key").unwrap())));
}