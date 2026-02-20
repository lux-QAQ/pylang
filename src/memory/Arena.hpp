#pragma once

// =============================================================================
// Arena Allocator — Pylang 核心内存分配器
// =============================================================================
//
// Arena (Region-based) 分配器，替代 Heap + GC。
//
// 设计原则:
//   1. Bump allocation: O(1) 分配，仅移动偏移指针
//   2. 批量释放: Arena 销毁时统一调用析构函数并释放内存
//   3. 线程局部: 每线程持有独立 Arena，分配无需加锁
//   4. 零 GC 开销: 无标记、无扫描、无引用计数
//
// 对象生命周期由 Arena 层次结构决定:
//   ProgramArena  → 程序退出时释放 (Immortal objects)
//   ModuleArena   → 程序退出时释放 (模块不卸载)
//   ThreadArena   → ScopedRegion 析构时释放 (函数级)
//
// =============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace py {

class Arena
{
public:
	static constexpr size_t kDefaultBlockSize = 64 * 1024;// 64 KB
	static constexpr size_t kMaxBlockSize = 4 * 1024 * 1024;// 4 MB

	explicit Arena(size_t default_block_size = kDefaultBlockSize)
		: m_default_block_size(default_block_size)
	{}

	~Arena() { reset(); }

	// Non-copyable
	Arena(const Arena &) = delete;
	Arena &operator=(const Arena &) = delete;

	// Movable
	Arena(Arena &&other) noexcept
		: m_blocks(std::move(other.m_blocks))
		, m_destructors(std::move(other.m_destructors))
		, m_default_block_size(other.m_default_block_size)
		, m_total_allocated(other.m_total_allocated)
	{
		other.m_total_allocated = 0;
	}

	Arena &operator=(Arena &&other) noexcept
	{
		if (this != &other) {
			reset();
			m_blocks = std::move(other.m_blocks);
			m_destructors = std::move(other.m_destructors);
			m_default_block_size = other.m_default_block_size;
			m_total_allocated = other.m_total_allocated;
			other.m_total_allocated = 0;
		}
		return *this;
	}

	// ---- 主分配接口 ----

	/// 分配一个 T 类型的对象, 调用构造函数
	/// 返回 nullptr 仅在极端 OOM 场景 (new 抛异常前)
	template<typename T, typename... Args> T *allocate(Args &&...args)
	{
		static_assert(alignof(T) <= alignof(std::max_align_t),
			"Over-aligned types not yet supported");

		void *mem = bump_allocate(sizeof(T), alignof(T));
		if (!mem) return nullptr;

		T *obj = new (mem) T(std::forward<Args>(args)...);

		// 非平凡析构类型需要注册析构函数
		if constexpr (!std::is_trivially_destructible_v<T>) {
			m_destructors.push_back(
				DtorEntry{ mem, [](void *p) { static_cast<T *>(p)->~T(); } });
		}

		return obj;
	}

	/// 带额外字节的分配 (变长对象: PyBytes, PyTuple 等)
	template<typename T, typename... Args>
	T *allocate_with_extra(size_t extra_bytes, Args &&...args)
	{
		static_assert(alignof(T) <= alignof(std::max_align_t),
			"Over-aligned types not yet supported");

		void *mem = bump_allocate(sizeof(T) + extra_bytes, alignof(T));
		if (!mem) return nullptr;

		T *obj = new (mem) T(std::forward<Args>(args)...);

		if constexpr (!std::is_trivially_destructible_v<T>) {
			m_destructors.push_back(
				DtorEntry{ mem, [](void *p) { static_cast<T *>(p)->~T(); } });
		}

		return obj;
	}

	/// 分配原始内存 (不调用构造函数)
	void *allocate_raw(size_t size, size_t align = alignof(std::max_align_t))
	{
		return bump_allocate(size, align);
	}

	/// 批量释放: 逆序调用所有析构函数, 然后释放内存块
	void reset()
	{
		// 逆序析构 — 保证后分配的对象先析构 (LIFO)
		for (auto it = m_destructors.rbegin(); it != m_destructors.rend(); ++it) {
			it->dtor(it->ptr);
		}
		m_destructors.clear();

		// 释放所有内存块
		for (auto &block : m_blocks) { ::operator delete(block.memory); }
		m_blocks.clear();
		m_total_allocated = 0;
	}

	// ---- thread_local 当前 Arena ----

	/// 获取当前线程的活跃 Arena
	static Arena &current()
	{
		assert(t_current_arena && "No active Arena on this thread");
		return *t_current_arena;
	}

	/// 设置当前线程的活跃 Arena (供 ScopedRegion / ArenaManager 使用)
	static void set_current(Arena *arena) { t_current_arena = arena; }

	/// 当前线程是否有活跃 Arena
	static bool has_current() { return t_current_arena != nullptr; }

	// ---- 统计信息 ----

	size_t bytes_allocated() const { return m_total_allocated; }
	size_t block_count() const { return m_blocks.size(); }
	size_t destructor_count() const { return m_destructors.size(); }

	/// 保存当前状态 (用于标记-回退: 分配临时对象后可回退到此点)
	struct SavePoint
	{
		size_t block_count;
		size_t block_offset;// 最后一个 block 的 offset
		size_t dtor_count;
		size_t total_allocated;
	};

	SavePoint save() const
	{
		return SavePoint{
			m_blocks.size(),
			m_blocks.empty() ? 0 : m_blocks.back().offset,
			m_destructors.size(),
			m_total_allocated,
		};
	}

	/// 回退到保存点: 析构保存点之后的所有对象并释放多余内存块
	void restore(const SavePoint &sp)
	{
		// 逆序析构保存点之后注册的对象
		while (m_destructors.size() > sp.dtor_count) {
			auto &entry = m_destructors.back();
			entry.dtor(entry.ptr);
			m_destructors.pop_back();
		}

		// 释放多余的内存块
		while (m_blocks.size() > sp.block_count) {
			::operator delete(m_blocks.back().memory);
			m_blocks.pop_back();
		}

		// 恢复最后一个 block 的偏移
		if (!m_blocks.empty() && m_blocks.size() == sp.block_count) {
			m_blocks.back().offset = sp.block_offset;
		}

		m_total_allocated = sp.total_allocated;
	}

private:
	struct Block
	{
		uint8_t *memory;
		size_t capacity;
		size_t offset;// bump pointer
	};

	struct DtorEntry
	{
		void *ptr;
		void (*dtor)(void *);
	};

	/// Bump allocation 核心
	void *bump_allocate(size_t size, size_t align)
	{
		// 尝试在当前 block 中分配
		if (!m_blocks.empty()) {
			auto &current = m_blocks.back();
			size_t aligned_offset = align_up(current.offset, align);
			if (aligned_offset + size <= current.capacity) {
				void *ptr = current.memory + aligned_offset;
				current.offset = aligned_offset + size;
				m_total_allocated += size;
				return ptr;
			}
		}

		// 当前 block 空间不足, 分配新 block
		add_block(size);

		auto &current = m_blocks.back();
		size_t aligned_offset = align_up(current.offset, align);
		assert(aligned_offset + size <= current.capacity);

		void *ptr = current.memory + aligned_offset;
		current.offset = aligned_offset + size;
		m_total_allocated += size;
		return ptr;
	}

	/// 分配新的内存块
	void add_block(size_t min_size)
	{
		// 新 block 至少容纳请求的大小, 并随 Arena 增长逐步扩大
		size_t block_size = m_default_block_size;

		// 增长策略: 每次翻倍, 但不超过上限
		if (!m_blocks.empty()) {
			block_size = std::min(m_blocks.back().capacity * 2, kMaxBlockSize);
		}

		// 确保至少能容纳此次分配
		block_size = std::max(block_size, min_size + alignof(std::max_align_t));

		auto *memory = static_cast<uint8_t *>(::operator new(block_size));
		m_blocks.push_back(Block{ memory, block_size, 0 });
	}

	/// 对齐辅助
	static size_t align_up(size_t offset, size_t align)
	{
		return (offset + align - 1) & ~(align - 1);
	}

	std::vector<Block> m_blocks;
	std::vector<DtorEntry> m_destructors;
	size_t m_default_block_size;
	size_t m_total_allocated{ 0 };

	static thread_local Arena *t_current_arena;
};

}// namespace py
