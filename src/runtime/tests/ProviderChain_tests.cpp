//
// 测试 RuntimeContext provider 完整链路
// 验证 VM 层设置的 provider 与 runtime 层消费者正确联动
//
#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/Value.hpp"

#include <gtest/gtest.h>

using namespace py;

class ProviderChainTest : public ::testing::Test
{
  protected:
	RuntimeContext ctx;
	RuntimeContext *saved{ nullptr };

	void SetUp() override
	{
		saved = RuntimeContext::has_current() ? &RuntimeContext::current() : nullptr;
		RuntimeContext::set_current(&ctx);
	}

	void TearDown() override { RuntimeContext::set_current(saved); }
};

// =============================================================================
// stack_local provider — 模拟 VM 层设置的栈访问
// =============================================================================

TEST_F(ProviderChainTest, StackLocalReadWrite)
{
	// 模拟 VM.cpp 中设置的 stack_local provider
	std::vector<Value> stack(16, Value{ py_none() });
	ctx.set_stack_local_provider([&stack](size_t idx) -> Value & { return stack[idx]; });

	// 模拟 PyCode::eval 中的栈操作
	ctx.stack_local(0) = Value{ PyInteger::create(10).unwrap() };
	ctx.stack_local(1) = Value{ PyInteger::create(20).unwrap() };

	// 验证
	auto *v0 = ctx.stack_local(0).as_ptr();
	auto *v1 = ctx.stack_local(1).as_ptr();
	EXPECT_EQ(as<PyInteger>(v0)->as_i64(), 10);
	EXPECT_EQ(as<PyInteger>(v1)->as_i64(), 20);
}

TEST_F(ProviderChainTest, StackLocalOverwrite)
{
	std::vector<Value> stack(8, Value{ py_none() });
	ctx.set_stack_local_provider([&stack](size_t idx) -> Value & { return stack[idx]; });

	ctx.stack_local(0) = Value{ PyInteger::create(1).unwrap() };
	ctx.stack_local(0) = Value{ PyInteger::create(2).unwrap() };

	auto *v = ctx.stack_local(0).as_ptr();
	EXPECT_EQ(as<PyInteger>(v)->as_i64(), 2);
}

// =============================================================================
// globals provider — 模拟编译后代码访问全局变量
// =============================================================================

TEST_F(ProviderChainTest, GlobalsProviderReturnsDict)
{
	auto *globals = PyDict::create().unwrap();
	globals->insert(PyString::create("x").unwrap(), PyInteger::create(42).unwrap());

	ctx.set_globals_provider([globals]() { return globals; });

	auto *g = ctx.current_globals();
	ASSERT_NE(g, nullptr);
	EXPECT_EQ(g, globals);

	auto it = g->map().find(PyString::create("x").unwrap());
	ASSERT_NE(it, g->map().end());
}

// =============================================================================
// truthy + equals 联动 — 模拟 if 语句编译路径
// =============================================================================

TEST_F(ProviderChainTest, IfStatementSimulation)
{
	// 模拟:
	//   if x == 10:
	//       result = "yes"
	//   else:
	//       result = "no"

	auto *x = PyInteger::create(10).unwrap();
	auto *ten = PyInteger::create(10).unwrap();

	// 编译器生成: eq_result = equals(x, 10)
	auto *eq_result = ctx.equals(x, ten);

	// 编译器生成: if is_true(eq_result)
	PyObject *result = nullptr;
	if (ctx.is_true(eq_result)) {
		result = PyString::create("yes").unwrap();
	} else {
		result = PyString::create("no").unwrap();
	}

	EXPECT_EQ(as<PyString>(result)->value(), "yes");
}

TEST_F(ProviderChainTest, WhileLoopSimulation)
{
	// 模拟:
	//   i = 0; total = 0
	//   while i < 5:
	//       total += i
	//       i += 1

	int64_t i = 0;
	int64_t total = 0;

	while (true) {
		auto *py_i = PyInteger::create(i).unwrap();
		auto *py_5 = PyInteger::create(5).unwrap();

		auto cmp = py_i->lt(py_5);
		ASSERT_TRUE(cmp.is_ok());

		if (!ctx.is_true(cmp.unwrap())) break;

		total += i;
		i++;
	}

	EXPECT_EQ(i, 5);
	EXPECT_EQ(total, 10);// 0+1+2+3+4
}

// =============================================================================
// 多 provider 同时工作
// =============================================================================

TEST_F(ProviderChainTest, AllProvidersSimultaneous)
{
	std::vector<Value> stack(4, Value{ py_none() });
	auto *globals = PyDict::create().unwrap();

	ctx.set_stack_local_provider([&stack](size_t idx) -> Value & { return stack[idx]; });
	ctx.set_globals_provider([globals]() { return globals; });

	// 全部工作
	ctx.stack_local(0) = Value{ PyInteger::create(1).unwrap() };
	globals->insert(PyString::create("g").unwrap(), PyInteger::create(2).unwrap());

	EXPECT_EQ(as<PyInteger>(ctx.stack_local(0).as_ptr())->as_i64(), 1);
	EXPECT_NE(ctx.current_globals(), nullptr);
	EXPECT_TRUE(ctx.is_true(PyInteger::create(1).unwrap()));
	EXPECT_EQ(ctx.equals(PyInteger::create(1).unwrap(), PyInteger::create(1).unwrap()), py_true());
}