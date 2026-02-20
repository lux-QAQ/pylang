#pragma once

// =============================================================================
// ScopedRegion — 函数级作用域 Arena
// =============================================================================
//
// RAII 守卫: 构造时创建新 Arena 并设为当前线程的活跃 Arena,
// 析构时恢复之前的 Arena 并释放 Region 内所有对象。
//
// 编译器为每个函数调用生成:
//   void compiled_func() {
//       ScopedRegion region;
//       // 所有局部分配自动进入 region
//       auto* temp = Arena::current().allocate<PyList>();
//       // region 析构时, temp 被自动释放
//   }
//
// =============================================================================

#include "Arena.hpp"

namespace py {

struct ScopedRegion
{
	Arena region;
	Arena *previous;

	explicit ScopedRegion(size_t block_size = Arena::kDefaultBlockSize)
		: region(block_size), previous(Arena::has_current() ? &Arena::current() : nullptr)
	{
		Arena::set_current(&region);
	}

	~ScopedRegion()
	{
		region.reset();// 析构所有对象 + 释放内存
		Arena::set_current(previous);
	}

	// Non-copyable, non-movable
	ScopedRegion(const ScopedRegion &) = delete;
	ScopedRegion &operator=(const ScopedRegion &) = delete;
	ScopedRegion(ScopedRegion &&) = delete;
	ScopedRegion &operator=(ScopedRegion &&) = delete;
};

}// namespace py
