#include "gtest/gtest.h"

#include "memory/Arena.hpp"
#include "memory/ArenaManager.hpp"
#include "memory/ScopedRegion.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace py;

// =============================================================================
// Test fixtures and helper types
// =============================================================================

// 简单 POD 类型
struct TrivialObj
{
	int x;
	int y;
	TrivialObj(int a, int b) : x(a), y(b) {}
};

// 带析构函数的类型 — 用于验证析构顺序
struct TrackedObj
{
	static inline std::vector<int> destruction_order;
	int id;

	explicit TrackedObj(int id_) : id(id_) {}
	~TrackedObj() { destruction_order.push_back(id); }
};

// 变长对象 — 模拟 PyBytes 等
struct VarLenObj
{
	size_t len;
	// 额外字节紧跟在结构体之后
	uint8_t *extra_data() { return reinterpret_cast<uint8_t *>(this) + sizeof(VarLenObj); }
	explicit VarLenObj(size_t l) : len(l) {}
};

// 大对象 — 测试跨 block 分配
struct LargeObj
{
	std::array<uint8_t, 4096> data{};
	int id;
	explicit LargeObj(int id_) : id(id_) { data.fill(static_cast<uint8_t>(id_)); }
};

// =============================================================================
// Arena 基础测试
// =============================================================================

TEST(Arena, AllocateTrivial)
{
	Arena arena(1024);
	auto *obj = arena.allocate<TrivialObj>(10, 20);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->x, 10);
	EXPECT_EQ(obj->y, 20);
	EXPECT_GT(arena.bytes_allocated(), 0u);
	EXPECT_EQ(arena.block_count(), 1u);
	// 平凡析构类型不注册析构函数
	EXPECT_EQ(arena.destructor_count(), 0u);
}

TEST(Arena, AllocateNonTrivial)
{
	TrackedObj::destruction_order.clear();
	{
		Arena arena(1024);
		auto *obj = arena.allocate<TrackedObj>(42);
		ASSERT_NE(obj, nullptr);
		EXPECT_EQ(obj->id, 42);
		EXPECT_EQ(arena.destructor_count(), 1u);
	}
	// Arena 析构时应调用 TrackedObj 的析构函数
	ASSERT_EQ(TrackedObj::destruction_order.size(), 1u);
	EXPECT_EQ(TrackedObj::destruction_order[0], 42);
}

TEST(Arena, DestructionOrderIsLIFO)
{
	TrackedObj::destruction_order.clear();
	{
		Arena arena(4096);
		arena.allocate<TrackedObj>(1);
		arena.allocate<TrackedObj>(2);
		arena.allocate<TrackedObj>(3);
	}
	// 应该按 LIFO 顺序析构: 3, 2, 1
	ASSERT_EQ(TrackedObj::destruction_order.size(), 3u);
	EXPECT_EQ(TrackedObj::destruction_order[0], 3);
	EXPECT_EQ(TrackedObj::destruction_order[1], 2);
	EXPECT_EQ(TrackedObj::destruction_order[2], 1);
}

TEST(Arena, AllocateWithExtra)
{
	Arena arena(1024);
	auto *obj = arena.allocate_with_extra<VarLenObj>(64, 64);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->len, 64u);

	// 写入额外字节区域，确保内存可用
	uint8_t *extra = obj->extra_data();
	for (size_t i = 0; i < 64; ++i) { extra[i] = static_cast<uint8_t>(i); }
	for (size_t i = 0; i < 64; ++i) { EXPECT_EQ(extra[i], static_cast<uint8_t>(i)); }
}

TEST(Arena, MultipleBlocks)
{
	Arena arena(64);// 很小的 block，强制多 block
	std::vector<TrivialObj *> objects;
	for (int i = 0; i < 100; ++i) {
		auto *obj = arena.allocate<TrivialObj>(i, i * 2);
		ASSERT_NE(obj, nullptr);
		objects.push_back(obj);
	}

	EXPECT_GT(arena.block_count(), 1u);

	// 验证所有对象仍然有效
	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(objects[i]->x, i);
		EXPECT_EQ(objects[i]->y, i * 2);
	}
}

TEST(Arena, LargeObjectAllocation)
{
	Arena arena(1024);// block 远小于 LargeObj
	auto *obj = arena.allocate<LargeObj>(7);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->id, 7);
	EXPECT_EQ(obj->data[0], 7);
	EXPECT_EQ(obj->data[4095], 7);
}

TEST(Arena, AllocateRaw)
{
	Arena arena(1024);
	void *mem = arena.allocate_raw(256, 16);
	ASSERT_NE(mem, nullptr);
	// 检查对齐
	EXPECT_EQ(reinterpret_cast<uintptr_t>(mem) % 16, 0u);
}

TEST(Arena, Reset)
{
	TrackedObj::destruction_order.clear();
	Arena arena(1024);
	arena.allocate<TrackedObj>(1);
	arena.allocate<TrackedObj>(2);

	EXPECT_GT(arena.bytes_allocated(), 0u);
	EXPECT_EQ(arena.destructor_count(), 2u);

	arena.reset();

	EXPECT_EQ(arena.bytes_allocated(), 0u);
	EXPECT_EQ(arena.block_count(), 0u);
	EXPECT_EQ(arena.destructor_count(), 0u);
	ASSERT_EQ(TrackedObj::destruction_order.size(), 2u);

	// reset 后可以继续分配
	auto *obj = arena.allocate<TrivialObj>(99, 100);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->x, 99);
}

TEST(Arena, MoveConstruct)
{
	TrackedObj::destruction_order.clear();

	Arena arena1(1024);
	arena1.allocate<TrackedObj>(1);

	Arena arena2(std::move(arena1));
	// arena1 应该为空
	EXPECT_EQ(arena1.bytes_allocated(), 0u);
	EXPECT_EQ(arena1.block_count(), 0u);

	// arena2 接管了对象
	EXPECT_GT(arena2.bytes_allocated(), 0u);
	EXPECT_EQ(arena2.destructor_count(), 1u);

	// 析构 arena2 应触发 TrackedObj 析构
	arena2.reset();
	ASSERT_EQ(TrackedObj::destruction_order.size(), 1u);
	EXPECT_EQ(TrackedObj::destruction_order[0], 1);
}

// =============================================================================
// SavePoint / Restore 测试
// =============================================================================

TEST(Arena, SaveAndRestore)
{
	TrackedObj::destruction_order.clear();

	Arena arena(4096);
	arena.allocate<TrackedObj>(1);
	arena.allocate<TrackedObj>(2);

	auto sp = arena.save();

	arena.allocate<TrackedObj>(3);
	arena.allocate<TrackedObj>(4);
	EXPECT_EQ(arena.destructor_count(), 4u);

	arena.restore(sp);
	// 3 和 4 应该被析构 (LIFO: 4, 3)
	ASSERT_EQ(TrackedObj::destruction_order.size(), 2u);
	EXPECT_EQ(TrackedObj::destruction_order[0], 4);
	EXPECT_EQ(TrackedObj::destruction_order[1], 3);
	EXPECT_EQ(arena.destructor_count(), 2u);

	// 1 和 2 仍然存活
	arena.reset();
	ASSERT_EQ(TrackedObj::destruction_order.size(), 4u);
	EXPECT_EQ(TrackedObj::destruction_order[2], 2);
	EXPECT_EQ(TrackedObj::destruction_order[3], 1);
}

// =============================================================================
// thread_local Current Arena 测试
// =============================================================================

TEST(Arena, CurrentArena)
{
	EXPECT_FALSE(Arena::has_current());

	Arena arena(1024);
	Arena::set_current(&arena);
	EXPECT_TRUE(Arena::has_current());
	EXPECT_EQ(&Arena::current(), &arena);

	Arena::set_current(nullptr);
	EXPECT_FALSE(Arena::has_current());
}

// =============================================================================
// ScopedRegion 测试
// =============================================================================

TEST(ScopedRegion, BasicLifecycle)
{
	TrackedObj::destruction_order.clear();

	Arena outer(1024);
	Arena::set_current(&outer);

	{
		ScopedRegion region;
		EXPECT_EQ(&Arena::current(), &region.region);
		EXPECT_NE(&Arena::current(), &outer);

		Arena::current().allocate<TrackedObj>(10);
		Arena::current().allocate<TrackedObj>(20);
	}
	// region 析构后:
	// - TrackedObj 10, 20 应该被析构
	// - 当前 Arena 恢复为 outer
	EXPECT_EQ(&Arena::current(), &outer);
	ASSERT_EQ(TrackedObj::destruction_order.size(), 2u);
	EXPECT_EQ(TrackedObj::destruction_order[0], 20);
	EXPECT_EQ(TrackedObj::destruction_order[1], 10);

	Arena::set_current(nullptr);
}

TEST(ScopedRegion, NestedRegions)
{
	TrackedObj::destruction_order.clear();

	Arena base(1024);
	Arena::set_current(&base);

	{
		ScopedRegion outer;
		Arena::current().allocate<TrackedObj>(1);

		{
			ScopedRegion inner;
			Arena::current().allocate<TrackedObj>(2);
		}
		// inner 析构: TrackedObj(2) 被释放
		ASSERT_EQ(TrackedObj::destruction_order.size(), 1u);
		EXPECT_EQ(TrackedObj::destruction_order[0], 2);

		// 恢复到 outer region
		EXPECT_EQ(&Arena::current(), &outer.region);
	}
	// outer 析构: TrackedObj(1) 被释放
	ASSERT_EQ(TrackedObj::destruction_order.size(), 2u);
	EXPECT_EQ(TrackedObj::destruction_order[1], 1);

	EXPECT_EQ(&Arena::current(), &base);
	Arena::set_current(nullptr);
}

// =============================================================================
// ArenaManager 测试
// =============================================================================

TEST(ArenaManager, InitializeAndShutdown)
{
	ArenaManager::initialize();
	EXPECT_TRUE(ArenaManager::is_initialized());
	EXPECT_TRUE(Arena::has_current());

	auto &prog = ArenaManager::program_arena();
	auto *obj = prog.allocate<TrivialObj>(1, 2);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->x, 1);

	ArenaManager::shutdown();
	EXPECT_FALSE(ArenaManager::is_initialized());
}

TEST(ArenaManager, ModuleArena)
{
	ArenaManager::initialize();

	auto &math_arena = ArenaManager::module_arena("math");
	auto &sys_arena = ArenaManager::module_arena("sys");

	// 应该返回不同的 Arena
	EXPECT_NE(&math_arena, &sys_arena);

	// 同名应返回同一个 Arena
	auto &math_arena2 = ArenaManager::module_arena("math");
	EXPECT_EQ(&math_arena, &math_arena2);

	auto *obj = math_arena.allocate<TrivialObj>(3, 14);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->x, 3);

	ArenaManager::shutdown();
}

TEST(ArenaManager, ThreadArena)
{
	ArenaManager::initialize();

	auto &ta = ArenaManager::thread_arena();
	auto *obj = ta.allocate<TrivialObj>(42, 0);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(obj->x, 42);

	ArenaManager::shutdown();
}

TEST(ArenaManager, MultiThreadIsolation)
{
	ArenaManager::initialize();

	std::atomic<bool> thread_ok{ false };
	Arena *main_arena = &ArenaManager::thread_arena();

	std::thread t([&]() {
		auto &ta = ArenaManager::thread_arena();
		// 子线程的 ThreadArena 应与主线程不同
		thread_ok = (&ta != main_arena);

		auto *obj = ta.allocate<TrivialObj>(99, 100);
		if (!obj || obj->x != 99) { thread_ok = false; }
	});

	t.join();
	EXPECT_TRUE(thread_ok.load());

	ArenaManager::shutdown();
}

// =============================================================================
// WeakRefRegistry 测试
// =============================================================================

#include "runtime/modules/weakref/WeakRefRegistry.hpp"

// 模拟 PyObject (不引入实际 PyObject 以保持测试独立)
struct FakeObj
{
	int id;
	explicit FakeObj(int id_) : id(id_) {}
};

TEST(WeakRefRegistry, BasicLifecycle)
{
	auto &reg = weakref::WeakRefRegistry::instance();
	reg.clear();

	FakeObj target(1);
	FakeObj ref_obj(2);

	auto *t = reinterpret_cast<PyObject *>(&target);
	auto *r = reinterpret_cast<PyObject *>(&ref_obj);

	reg.register_ref(t, r);

	EXPECT_TRUE(reg.is_alive(t));
	EXPECT_EQ(reg.ref_count(t), 1u);

	auto refs = reg.get_refs(t);
	ASSERT_EQ(refs.size(), 1u);
	EXPECT_EQ(refs[0], r);

	// 标记为死亡
	reg.invalidate(t);
	EXPECT_FALSE(reg.is_alive(t));
	// ref_count 不变 (弱引用仍然存在, 只是目标死了)
	EXPECT_EQ(reg.ref_count(t), 1u);

	reg.clear();
}

TEST(WeakRefRegistry, MultipleRefs)
{
	auto &reg = weakref::WeakRefRegistry::instance();
	reg.clear();

	FakeObj target(1);
	FakeObj ref1(10), ref2(20), ref3(30);

	auto *t = reinterpret_cast<PyObject *>(&target);

	reg.register_ref(t, reinterpret_cast<PyObject *>(&ref1));
	reg.register_ref(t, reinterpret_cast<PyObject *>(&ref2));
	reg.register_ref(t, reinterpret_cast<PyObject *>(&ref3));

	EXPECT_EQ(reg.ref_count(t), 3u);
	EXPECT_TRUE(reg.is_alive(t));

	reg.invalidate(t);
	EXPECT_FALSE(reg.is_alive(t));

	reg.clear();
}

TEST(WeakRefRegistry, BatchInvalidate)
{
	auto &reg = weakref::WeakRefRegistry::instance();
	reg.clear();

	FakeObj t1(1), t2(2), t3(3);
	FakeObj r(99);

	auto *p1 = reinterpret_cast<PyObject *>(&t1);
	auto *p2 = reinterpret_cast<PyObject *>(&t2);
	auto *p3 = reinterpret_cast<PyObject *>(&t3);
	auto *pr = reinterpret_cast<PyObject *>(&r);

	reg.register_ref(p1, pr);
	reg.register_ref(p2, pr);
	reg.register_ref(p3, pr);

	EXPECT_TRUE(reg.is_alive(p1));
	EXPECT_TRUE(reg.is_alive(p2));
	EXPECT_TRUE(reg.is_alive(p3));

	reg.invalidate_batch({ p1, p3 });

	EXPECT_FALSE(reg.is_alive(p1));
	EXPECT_TRUE(reg.is_alive(p2));
	EXPECT_FALSE(reg.is_alive(p3));

	reg.clear();
}

TEST(WeakRefRegistry, RemoveTarget)
{
	auto &reg = weakref::WeakRefRegistry::instance();
	reg.clear();

	FakeObj target(1);
	FakeObj ref_obj(2);

	auto *t = reinterpret_cast<PyObject *>(&target);
	auto *r = reinterpret_cast<PyObject *>(&ref_obj);

	reg.register_ref(t, r);
	EXPECT_EQ(reg.ref_count(t), 1u);

	reg.remove_target(t);
	EXPECT_FALSE(reg.is_alive(t));
	EXPECT_EQ(reg.ref_count(t), 0u);

	reg.clear();
}

TEST(WeakRefRegistry, UnknownTargetIsNotAlive)
{
	auto &reg = weakref::WeakRefRegistry::instance();
	reg.clear();

	FakeObj unknown(999);
	auto *u = reinterpret_cast<PyObject *>(&unknown);

	EXPECT_FALSE(reg.is_alive(u));
	EXPECT_EQ(reg.ref_count(u), 0u);
	EXPECT_TRUE(reg.get_refs(u).empty());
}

// =============================================================================
// Alignment 测试
// =============================================================================

struct alignas(16) AlignedObj
{
	double val;
	explicit AlignedObj(double v) : val(v) {}
};

TEST(Arena, AlignmentRespected)
{
	Arena arena(1024);

	// 先分配一个小对象打乱偏移
	arena.allocate<char>('x');

	auto *obj = arena.allocate<AlignedObj>(3.14);
	ASSERT_NE(obj, nullptr);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(obj) % alignof(AlignedObj), 0u);
	EXPECT_DOUBLE_EQ(obj->val, 3.14);
}

// =============================================================================
// 压力测试
// =============================================================================

TEST(Arena, StressTest)
{
	Arena arena(256);// 小 block 强制频繁扩展
	constexpr int N = 10000;

	for (int i = 0; i < N; ++i) { arena.allocate<TrivialObj>(i, i); }

	EXPECT_EQ(arena.destructor_count(), 0u);// TrivialObj 无析构
	EXPECT_GT(arena.block_count(), 1u);
	EXPECT_GE(arena.bytes_allocated(), N * sizeof(TrivialObj));
}

TEST(Arena, StressTestNonTrivial)
{
	TrackedObj::destruction_order.clear();
	constexpr int N = 1000;

	{
		Arena arena(256);
		for (int i = 0; i < N; ++i) { arena.allocate<TrackedObj>(i); }
		EXPECT_EQ(arena.destructor_count(), static_cast<size_t>(N));
	}

	ASSERT_EQ(TrackedObj::destruction_order.size(), static_cast<size_t>(N));
	// 验证 LIFO: 最后分配的最先析构
	for (int i = 0; i < N; ++i) {
		EXPECT_EQ(TrackedObj::destruction_order[i], N - 1 - i);
	}
}
