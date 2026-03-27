//
// 模拟编译后的 .py 文件调用 runtime 的完整路径
// 这是最接近真实 AOT 编译产物的测试
//
#include "interpreter/InterpreterCore.hpp"
#include "runtime/Import.hpp"
#include "runtime/ModuleRegistry.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFloat.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/StopIteration.hpp"
#include "runtime/Value.hpp"
#include "runtime/builtinTypeInit.hpp"
#include "runtime/modules/Modules.hpp"
#include "runtime/modules/config.hpp"
#include "runtime/types/builtin.hpp"

#include <gtest/gtest.h>

using namespace py;

// =============================================================================
// Fixture: 模拟编译后程序的执行环境
// =============================================================================
class RuntimeAPITest : public ::testing::Test
{
  protected:
	RuntimeContext ctx;
	RuntimeContext *saved{ nullptr };

	void SetUp() override
	{
		saved = RuntimeContext::has_current() ? &RuntimeContext::current() : nullptr;
		RuntimeContext::set_current(&ctx);
		initialize_types();
		register_all_builtins();
	}

	void TearDown() override { RuntimeContext::set_current(saved); }
};

// =============================================================================
// 1. 基础对象创建与运算 — 编译器生成的最基本操作
// =============================================================================

TEST_F(RuntimeAPITest, IntegerArithmetic)
{
	// 模拟: x = 10 + 20
	auto *a = PyInteger::create(10).unwrap();
	auto *b = PyInteger::create(20).unwrap();

	auto result = a->add(b);
	ASSERT_TRUE(result.is_ok());
	auto *sum = as<PyInteger>(result.unwrap());
	ASSERT_NE(sum, nullptr);
	EXPECT_EQ(sum->as_i64(), 30);
}

TEST_F(RuntimeAPITest, FloatArithmetic)
{
	// 模拟: x = 3.14 * 2.0
	auto *a = PyFloat::create(3.14).unwrap();
	auto *b = PyFloat::create(2.0).unwrap();

	auto result = a->__mul__(b);
	ASSERT_TRUE(result.is_ok());
	auto *product = as<PyFloat>(result.unwrap());
	ASSERT_NE(product, nullptr);
	EXPECT_DOUBLE_EQ(product->as_f64(), 6.28);
}

TEST_F(RuntimeAPITest, StringConcatenation)
{
	// 模拟: s = "hello" + " " + "world"
	auto *a = PyString::create("hello").unwrap();
	auto *b = PyString::create(" world").unwrap();

	auto result = a->add(b);
	ASSERT_TRUE(result.is_ok());
	auto *s = as<PyString>(result.unwrap());
	ASSERT_NE(s, nullptr);
	EXPECT_EQ(s->value(), "hello world");
}

TEST_F(RuntimeAPITest, ListOperations)
{
	auto *lst = PyList::create().unwrap();
	lst->append(PyInteger::create(1).unwrap());
	lst->append(PyInteger::create(2).unwrap());
	lst->append(PyInteger::create(3).unwrap());

	auto len_result = lst->__len__();
	ASSERT_TRUE(len_result.is_ok());
	EXPECT_EQ(len_result.unwrap(), 3u);

	// lst[0] == 1
	auto item = lst->__getitem__(PyInteger::create(0).unwrap());
	ASSERT_TRUE(item.is_ok());
	EXPECT_EQ(as<PyInteger>(item.unwrap())->as_i64(), 1);
}

TEST_F(RuntimeAPITest, DictOperations)
{
	// 模拟:
	//   d = {}
	//   d["key"] = 42
	//   assert d["key"] == 42
	auto *d = PyDict::create().unwrap();
	auto *key = PyString::create("key").unwrap();
	auto *val = PyInteger::create(42).unwrap();

	d->insert(key, val);

	auto result = d->getitem(key);
	ASSERT_TRUE(result.is_ok());
	EXPECT_EQ(as<PyInteger>(result.unwrap())->as_i64(), 42);
}

TEST_F(RuntimeAPITest, DictOperationsWithFloatKey)
{
	auto *d = PyDict::create().unwrap();
	auto *key = PyFloat::create(1.2).unwrap();
	auto *val = PyString::create("ok").unwrap();

	d->insert(key, val);

	auto result = d->getitem(PyFloat::create(1.2).unwrap());
	ASSERT_TRUE(result.is_ok());
	EXPECT_EQ(as<PyString>(result.unwrap())->value(), "ok");
}

TEST_F(RuntimeAPITest, TupleUnpacking)
{
	// 模拟: a, b, c = (1, 2, 3)
	auto *t = PyTuple::create(
		PyInteger::create(1).unwrap(), PyInteger::create(2).unwrap(), PyInteger::create(3).unwrap())
				  .unwrap();

	EXPECT_EQ(t->size(), 3u);
	EXPECT_EQ(as<PyInteger>((*t)[0].unwrap())->as_i64(), 1);
	EXPECT_EQ(as<PyInteger>((*t)[1].unwrap())->as_i64(), 2);
	EXPECT_EQ(as<PyInteger>((*t)[2].unwrap())->as_i64(), 3);
}

// =============================================================================
// 2. 比较与布尔运算 — richcompare 路径
// =============================================================================

TEST_F(RuntimeAPITest, IntegerComparison)
{
	auto *a = PyInteger::create(10).unwrap();
	auto *b = PyInteger::create(20).unwrap();

	auto lt = a->lt(b);
	ASSERT_TRUE(lt.is_ok());
	EXPECT_EQ(lt.unwrap(), py_true());

	auto gt = a->gt(b);
	ASSERT_TRUE(gt.is_ok());
	EXPECT_EQ(gt.unwrap(), py_false());

	auto eq = a->eq(a);
	ASSERT_TRUE(eq.is_ok());
	EXPECT_EQ(eq.unwrap(), py_true());
}

TEST_F(RuntimeAPITest, RuntimeContextEquals)
{
	auto *a = PyInteger::create(42).unwrap();
	auto *b = PyInteger::create(42).unwrap();
	auto *c = PyInteger::create(99).unwrap();

	EXPECT_EQ(ctx.equals(a, b), py_true());
	EXPECT_EQ(ctx.equals(a, c), py_false());
}

TEST_F(RuntimeAPITest, RuntimeContextIsTrueChain)
{
	// 测试完整的 is_true 判断链
	EXPECT_TRUE(ctx.is_true(PyInteger::create(1).unwrap()));
	EXPECT_FALSE(ctx.is_true(PyInteger::create(0).unwrap()));
	EXPECT_TRUE(ctx.is_true(PyString::create("x").unwrap()));
	EXPECT_FALSE(ctx.is_true(PyString::create("").unwrap()));

	auto *lst = PyList::create().unwrap();
	EXPECT_FALSE(ctx.is_true(lst));
	lst->append(py_none());
	EXPECT_TRUE(ctx.is_true(lst));
}

// =============================================================================
// 3. 模块系统 — 编译后的 PyInit_xxx 调用链
// =============================================================================

TEST_F(RuntimeAPITest, ModuleCreateAndRegister)
{
	// 模拟编译后的 PyInit_mymodule()
	auto *dict = PyDict::create().unwrap();
	auto *mod =
		PyModule::create(dict, PyString::create("mymodule").unwrap(), PyString::create("").unwrap())
			.unwrap();

	mod->add_symbol(PyString::create("PI").unwrap(), Value{ PyFloat::create(3.14159).unwrap() });
	mod->add_symbol(PyString::create("VERSION").unwrap(), Value{ PyInteger::create(1).unwrap() });

	ModuleRegistry::instance().register_module("mymodule", mod);

	// 模拟另一个模块 import mymodule
	auto *found = ModuleRegistry::instance().find("mymodule");
	ASSERT_NE(found, nullptr);
	EXPECT_EQ(found, mod);

	// 访问属性
	auto pi = found->__getattribute__(PyString::create("PI").unwrap());
	ASSERT_TRUE(pi.is_ok());
	EXPECT_DOUBLE_EQ(as<PyFloat>(pi.unwrap())->as_f64(), 3.14159);
}

TEST_F(RuntimeAPITest, ModuleAttributeAccess)
{
	// 模拟: import builtins; builtins.print
	auto *builtins = ModuleRegistry::instance().find("builtins");
	ASSERT_NE(builtins, nullptr);

	auto print_attr = builtins->__getattribute__(PyString::create("print").unwrap());
	ASSERT_TRUE(print_attr.is_ok());
	EXPECT_NE(print_attr.unwrap(), nullptr);
}

TEST_F(RuntimeAPITest, ModuleAttributeNotFound)
{
	auto *builtins = ModuleRegistry::instance().find("builtins");
	ASSERT_NE(builtins, nullptr);

	auto result = builtins->__getattribute__(PyString::create("nonexistent_xyz").unwrap());
	EXPECT_TRUE(result.is_err());
}

// =============================================================================
// 4. Inline Cache 模拟 — 编译器生成的全局变量访问
// =============================================================================

TEST_F(RuntimeAPITest, InlineCacheHit)
{
	// 模拟编译器生成的 inline cache 逻辑
	auto *mod = PyModule::create(PyDict::create().unwrap(),
		PyString::create("cache_test").unwrap(),
		PyString::create("").unwrap())
					.unwrap();

	mod->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(10).unwrap() });

	// 缓存 key version
	uint64_t cached_key_ver = mod->get_key_version("x");
	uint64_t cached_dict_ver = mod->dict_version();

	// 读取 — cache hit: version 未变
	EXPECT_EQ(mod->get_key_version("x"), cached_key_ver);
	EXPECT_EQ(mod->dict_version(), cached_dict_ver);
}

TEST_F(RuntimeAPITest, InlineCacheMissOnUpdate)
{
	auto *mod = PyModule::create(PyDict::create().unwrap(),
		PyString::create("cache_test2").unwrap(),
		PyString::create("").unwrap())
					.unwrap();

	mod->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(10).unwrap() });

	uint64_t cached_key_ver = mod->get_key_version("x");
	uint64_t cached_dict_ver = mod->dict_version();

	// 更新 x — cache miss
	mod->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(20).unwrap() });

	EXPECT_NE(mod->get_key_version("x"), cached_key_ver);
	EXPECT_NE(mod->dict_version(), cached_dict_ver);
}

TEST_F(RuntimeAPITest, InlineCacheNoThrashingOnOtherKey)
{
	auto *mod = PyModule::create(PyDict::create().unwrap(),
		PyString::create("cache_test3").unwrap(),
		PyString::create("").unwrap())
					.unwrap();

	mod->add_symbol(PyString::create("pi").unwrap(), Value{ PyFloat::create(3.14).unwrap() });
	uint64_t pi_ver = mod->get_key_version("pi");

	// 频繁更新 counter — pi 的 key version 不变
	for (int i = 0; i < 1000; ++i) {
		mod->add_symbol(
			PyString::create("counter").unwrap(), Value{ PyInteger::create(i).unwrap() });
	}

	EXPECT_EQ(mod->get_key_version("pi"), pi_ver);
}

TEST_F(RuntimeAPITest, FullInlineCacheSimulation)
{
	// 完整模拟编译器生成的 LOAD_GLOBAL 代码
	//
	// @cache_x = { key_version, cached_value }
	//
	// define ptr @load_global_x(ptr %module):
	//   %cached_ver = load @cache_x.version
	//   %cur_ver = call get_key_version(%module, "x")
	//   if %cached_ver == %cur_ver:
	//     return @cache_x.value       # fast path
	//   else:
	//     %val = call __getattribute__("x")  # slow path
	//     store %val -> @cache_x
	//     return %val

	auto *mod = PyModule::create(
		PyDict::create().unwrap(), PyString::create("sim").unwrap(), PyString::create("").unwrap())
					.unwrap();

	mod->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(100).unwrap() });

	// --- 模拟 cache slot ---
	struct CacheSlot
	{
		uint64_t version{ 0 };
		PyObject *value{ nullptr };
	} cache;

	auto load_global = [&](const std::string &key) -> PyObject * {
		uint64_t cur_ver = mod->get_key_version(key);
		if (cache.version == cur_ver && cache.value) {
			// fast path — cache hit
			return cache.value;
		}
		// slow path
		auto result = mod->__getattribute__(PyString::create(key).unwrap());
		if (result.is_ok()) {
			cache.version = cur_ver;
			cache.value = result.unwrap();
			return cache.value;
		}
		return nullptr;
	};

	// 第一次: cold miss
	auto *val1 = load_global("x");
	ASSERT_NE(val1, nullptr);
	EXPECT_EQ(as<PyInteger>(val1)->as_i64(), 100);

	// 第二次: cache hit (不调用 __getattribute__)
	auto *val2 = load_global("x");
	EXPECT_EQ(val2, val1);

	// 更新 x → cache miss
	mod->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(200).unwrap() });
	auto *val3 = load_global("x");
	ASSERT_NE(val3, nullptr);
	EXPECT_EQ(as<PyInteger>(val3)->as_i64(), 200);

	// 再次: cache hit
	auto *val4 = load_global("x");
	EXPECT_EQ(val4, val3);
}

// =============================================================================
// 5. 跨模块属性访问 — import a; a.x
// =============================================================================

TEST_F(RuntimeAPITest, CrossModuleAttributeAccess)
{
	// 模拟:
	//   # mod_a.py
	//   x = 42
	//
	//   # mod_b.py
	//   import mod_a
	//   print(mod_a.x)

	auto *mod_a = PyModule::create(PyDict::create().unwrap(),
		PyString::create("mod_a").unwrap(),
		PyString::create("").unwrap())
					  .unwrap();
	mod_a->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(42).unwrap() });
	ModuleRegistry::instance().register_module("mod_a", mod_a);

	// mod_b 中: import mod_a → Registry 查找
	auto *found = ModuleRegistry::instance().find("mod_a");
	ASSERT_NE(found, nullptr);

	// mod_a.x
	auto attr = found->__getattribute__(PyString::create("x").unwrap());
	ASSERT_TRUE(attr.is_ok());
	EXPECT_EQ(as<PyInteger>(attr.unwrap())->as_i64(), 42);

	// mod_a.x = 99 (从 mod_a 内部)
	mod_a->add_symbol(PyString::create("x").unwrap(), Value{ PyInteger::create(99).unwrap() });

	// mod_b 重新读取 — 看到新值
	attr = found->__getattribute__(PyString::create("x").unwrap());
	ASSERT_TRUE(attr.is_ok());
	EXPECT_EQ(as<PyInteger>(attr.unwrap())->as_i64(), 99);
}

// =============================================================================
// 6. NativeFunction 调用 — 不经过 VM 的直接调用路径
// =============================================================================

TEST_F(RuntimeAPITest, NativeFunctionDirect)
{
	// 模拟编译器直接调用 native function (不经过 interpreter)
	auto *func =
		PyNativeFunction::create("test_add", [](PyTuple *args, PyDict *) -> PyResult<PyObject *> {
			auto *a = as<PyInteger>((*args)[0].unwrap());
			auto *b = as<PyInteger>((*args)[1].unwrap());
			return PyInteger::create(a->as_i64() + b->as_i64());
		}).unwrap();

	auto *args =
		PyTuple::create(PyInteger::create(3).unwrap(), PyInteger::create(4).unwrap()).unwrap();

	// 直接调用（不经过 interpreter->call）
	auto result = func->operator()(args, nullptr);
	ASSERT_TRUE(result.is_ok());
	EXPECT_EQ(as<PyInteger>(result.unwrap())->as_i64(), 7);
}

// =============================================================================
// 7. isinstance / type 判断 — 编译器类型守卫
// =============================================================================

TEST_F(RuntimeAPITest, TypeChecks)
{
	auto *i = PyInteger::create(42).unwrap();
	auto *s = PyString::create("hello").unwrap();
	auto *f = PyFloat::create(3.14).unwrap();

	EXPECT_TRUE(i->type()->issubclass(types::integer()));
	EXPECT_FALSE(i->type()->issubclass(types::str()));

	EXPECT_TRUE(s->type()->issubclass(types::str()));
	EXPECT_FALSE(s->type()->issubclass(types::integer()));

	EXPECT_TRUE(f->type()->issubclass(types::float_()));
}

TEST_F(RuntimeAPITest, FloatHashMatchesEqualInteger)
{
	auto *f = PyFloat::create(1.0).unwrap();
	auto *i = PyInteger::create(1).unwrap();

	auto float_hash = f->hash();
	auto int_hash = i->hash();

	ASSERT_TRUE(float_hash.is_ok());
	ASSERT_TRUE(int_hash.is_ok());
	EXPECT_EQ(float_hash.unwrap(), int_hash.unwrap());
}

TEST_F(RuntimeAPITest, FloatRoundUsesBankersRounding)
{
	auto *f = PyFloat::create(2.5).unwrap();
	auto result = f->__round__(py_none());

	ASSERT_TRUE(result.is_ok());
	EXPECT_EQ(as<PyInteger>(result.unwrap())->as_i64(), 2);
}

TEST_F(RuntimeAPITest, FloatAsIntegerRatio)
{
	auto *f = PyFloat::create(-0.25).unwrap();
	auto result = f->as_integer_ratio();

	ASSERT_TRUE(result.is_ok());
	auto *tuple = as<PyTuple>(result.unwrap());
	ASSERT_NE(tuple, nullptr);
	EXPECT_EQ(as<PyInteger>((*tuple)[0].unwrap())->as_i64(), -1);
	EXPECT_EQ(as<PyInteger>((*tuple)[1].unwrap())->as_i64(), 4);
}

TEST_F(RuntimeAPITest, FloatHexRoundTrip)
{
	auto *f = PyFloat::create(3.5).unwrap();
	auto hex_value = f->hex();

	ASSERT_TRUE(hex_value.is_ok());
	auto *args = PyTuple::create(hex_value.unwrap()).unwrap();
	auto parsed = PyFloat::fromhex(types::float_(), args, nullptr);

	ASSERT_TRUE(parsed.is_ok());
	EXPECT_DOUBLE_EQ(as<PyFloat>(parsed.unwrap())->as_f64(), 3.5);
}

// =============================================================================
// 8. 迭代器协议 — for 循环编译路径
// =============================================================================

TEST_F(RuntimeAPITest, ListIteration)
{
	auto *lst = PyList::create().unwrap();
	lst->append(PyInteger::create(1).unwrap());
	lst->append(PyInteger::create(2).unwrap());
	lst->append(PyInteger::create(3).unwrap());

	auto iter_result = lst->__iter__();
	ASSERT_TRUE(iter_result.is_ok());
	auto *iter = iter_result.unwrap();

	int64_t total = 0;
	while (true) {
		auto next = iter->next();
		if (next.is_err()) {
			EXPECT_TRUE(next.unwrap_err()->type()->issubclass(types::stop_iteration()));
			break;
		}
		total += as<PyInteger>(next.unwrap())->as_i64();
	}
	EXPECT_EQ(total, 6);
}

TEST_F(RuntimeAPITest, DictIteration)
{
	auto *d = PyDict::create().unwrap();
	d->insert(PyString::create("a").unwrap(), PyInteger::create(1).unwrap());
	d->insert(PyString::create("b").unwrap(), PyInteger::create(2).unwrap());

	auto iter_result = d->iter();
	ASSERT_TRUE(iter_result.is_ok());
	auto *iter = iter_result.unwrap();

	int count = 0;
	while (true) {
		auto next = iter->next();
		if (next.is_err()) break;
		EXPECT_NE(as<PyString>(next.unwrap()), nullptr);
		count++;
	}
	EXPECT_EQ(count, 2);
}

// =============================================================================
// 9. 异常传播 — try/except 编译路径
// =============================================================================

TEST_F(RuntimeAPITest, ExceptionFromOperation)
{
	// 模拟: "hello" + 42 → TypeError
	auto *s = PyString::create("hello").unwrap();
	auto *i = PyInteger::create(42).unwrap();

	auto result = s->add(i);
	EXPECT_TRUE(result.is_err());
}

TEST_F(RuntimeAPITest, IndexOutOfRange)
{
	auto *lst = PyList::create().unwrap();
	lst->append(PyInteger::create(1).unwrap());

	auto result = lst->__getitem__(PyInteger::create(10).unwrap());
	EXPECT_TRUE(result.is_err());
}

TEST_F(RuntimeAPITest, KeyNotFound)
{
	auto *d = PyDict::create().unwrap();
	auto result = d->getitem(PyString::create("missing").unwrap());
	EXPECT_TRUE(result.is_err());
}