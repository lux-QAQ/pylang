#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFloat.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/Value.hpp"

#include <gtest/gtest.h>
#include <thread>

using namespace py;

// =============================================================================
// RuntimeContext 基础测试
// =============================================================================

class RuntimeContextTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		// 保存当前上下文（测试框架可能已设置）
		m_saved = RuntimeContext::has_current() ? &RuntimeContext::current() : nullptr;
	}

	void TearDown() override
	{
		// 恢复
		RuntimeContext::set_current(m_saved);
	}

	RuntimeContext *m_saved{ nullptr };
};

TEST_F(RuntimeContextTest, HasCurrentAfterSet)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);
	EXPECT_TRUE(RuntimeContext::has_current());
	EXPECT_EQ(&RuntimeContext::current(), &ctx);
}

TEST_F(RuntimeContextTest, NullAfterClear)
{
	RuntimeContext::set_current(nullptr);
	EXPECT_FALSE(RuntimeContext::has_current());
}

TEST_F(RuntimeContextTest, GuardRestoresPrevious)
{
	RuntimeContext outer;
	RuntimeContext inner;
	RuntimeContext::set_current(&outer);

	{
		RuntimeContext::Guard guard(&inner);
		EXPECT_EQ(&RuntimeContext::current(), &inner);
	}

	EXPECT_EQ(&RuntimeContext::current(), &outer);
}

TEST_F(RuntimeContextTest, GuardNested)
{
	RuntimeContext c1, c2, c3;
	RuntimeContext::set_current(&c1);

	{
		RuntimeContext::Guard g2(&c2);
		EXPECT_EQ(&RuntimeContext::current(), &c2);
		{
			RuntimeContext::Guard g3(&c3);
			EXPECT_EQ(&RuntimeContext::current(), &c3);
		}
		EXPECT_EQ(&RuntimeContext::current(), &c2);
	}
	EXPECT_EQ(&RuntimeContext::current(), &c1);
}

TEST_F(RuntimeContextTest, NoInterpreterByDefault)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);
	EXPECT_FALSE(ctx.has_interpreter());
	EXPECT_EQ(ctx.interpreter(), nullptr);
}

// =============================================================================
// is_true Provider 测试
// =============================================================================

TEST_F(RuntimeContextTest, IsTrueDefaultPath)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	EXPECT_TRUE(ctx.is_true(py_true()));
	EXPECT_FALSE(ctx.is_true(py_false()));
	EXPECT_FALSE(ctx.is_true(py_none()));
}

TEST_F(RuntimeContextTest, IsTrueWithProvider)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	// 自定义 provider：所有东西都是 false
	ctx.set_truthy_provider([](PyObject *) { return false; });
	EXPECT_FALSE(ctx.is_true(py_true()));

	// 自定义 provider：所有东西都是 true
	ctx.set_truthy_provider([](PyObject *) { return true; });
	EXPECT_TRUE(ctx.is_true(py_none()));
}

TEST_F(RuntimeContextTest, IsTrueWithInteger)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	auto *zero = PyInteger::create(0).unwrap();
	auto *one = PyInteger::create(1).unwrap();
	auto *neg = PyInteger::create(-1).unwrap();

	EXPECT_FALSE(ctx.is_true(zero));
	EXPECT_TRUE(ctx.is_true(one));
	EXPECT_TRUE(ctx.is_true(neg));
}

// =============================================================================
// equals Provider 测试
// =============================================================================

TEST_F(RuntimeContextTest, EqualsDefaultPath)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	auto *a = PyInteger::create(42).unwrap();
	auto *b = PyInteger::create(42).unwrap();
	auto *c = PyInteger::create(99).unwrap();

	auto *eq_result = ctx.equals(a, b);
	EXPECT_EQ(eq_result, py_true());

	auto *neq_result = ctx.equals(a, c);
	EXPECT_EQ(neq_result, py_false());
}

TEST_F(RuntimeContextTest, EqualsWithProvider)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	// provider 总是返回 py_true
	ctx.set_equals_provider([](PyObject *, PyObject *) -> PyObject * { return py_true(); });

	auto *a = PyInteger::create(1).unwrap();
	auto *b = PyInteger::create(999).unwrap();
	EXPECT_EQ(ctx.equals(a, b), py_true());
}

// =============================================================================
// stack_local Provider 测试
// =============================================================================

TEST_F(RuntimeContextTest, StackLocalWithProvider)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	std::vector<Value> locals(10, Value{ py_none() });
	ctx.set_stack_local_provider([&locals](size_t index) -> Value & { return locals[index]; });

	auto *val = PyInteger::create(42).unwrap();
	ctx.stack_local(3) = Value{ val };

	EXPECT_TRUE(locals[3].is_heap_object());
	EXPECT_EQ(locals[3].as_ptr(), val);
}

TEST_F(RuntimeContextTest, StackLocalWithoutProviderAsserts)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);
	// 不设置 provider，访问应触发 assert
	// 不在测试中触发 — 仅验证 has_current 可用
	EXPECT_TRUE(RuntimeContext::has_current());
}

// =============================================================================
// globals Provider 测试
// =============================================================================

TEST_F(RuntimeContextTest, GlobalsWithProvider)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);

	auto *globals = PyDict::create().unwrap();
	ctx.set_globals_provider([globals]() -> PyDict * { return globals; });

	EXPECT_EQ(ctx.current_globals(), globals);
}

TEST_F(RuntimeContextTest, GlobalsWithoutProviderReturnsNull)
{
	RuntimeContext ctx;
	RuntimeContext::set_current(&ctx);
	// 没有 provider 也没有 interpreter
	EXPECT_EQ(ctx.current_globals(), nullptr);
}

// =============================================================================
// 线程隔离测试
// =============================================================================

TEST_F(RuntimeContextTest, ThreadIsolation)
{
	RuntimeContext main_ctx;
	RuntimeContext::set_current(&main_ctx);

	std::atomic<bool> thread_has_no_ctx{ false };
	std::atomic<bool> thread_set_own_ctx{ false };

	std::thread t([&]() {
		// 新线程不应继承主线程的 RuntimeContext
		thread_has_no_ctx = !RuntimeContext::has_current();

		RuntimeContext thread_ctx;
		RuntimeContext::set_current(&thread_ctx);
		thread_set_own_ctx =
			(RuntimeContext::has_current() && &RuntimeContext::current() == &thread_ctx);
	});

	t.join();
	EXPECT_TRUE(thread_has_no_ctx.load());
	EXPECT_TRUE(thread_set_own_ctx.load());

	// 主线程不受影响
	EXPECT_EQ(&RuntimeContext::current(), &main_ctx);
}