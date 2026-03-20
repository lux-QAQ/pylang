#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#ifdef PYLANG_USE_Boehm_GC
#include <gc.h>
#endif

namespace py {

#ifdef PYLANG_USE_Boehm_GC
template<typename T> struct GCTracingAllocator
{
	using value_type = T;

	GCTracingAllocator() noexcept = default;
	template<typename U> GCTracingAllocator(const GCTracingAllocator<U> &) noexcept {}

	T *allocate(std::size_t n)
	{
		// [修复] 检查类型的性质，如果是纯普通标量或者 POD 纯值，直接 ATOMIC
		// 发配。这把字符串/字节数组等直接拔出 mark 的遍历图
		void *p = nullptr;
		if constexpr (std::is_scalar_v<T>) {
			p = GC_MALLOC_ATOMIC(n * sizeof(T));
		} else {
			p = GC_MALLOC(n * sizeof(T));
		}

		if (!p) throw std::bad_alloc();
		return static_cast<T *>(p);
	}

	// ✅ 关键修复：必须是空操作
	// 原因链：
	//   1. buffer 在 GC 堆上，GC 负责回收
	//   2. vector resize 时旧 buffer 变为不可达 → GC 自动回收（安全）
	//   3. finalizer 运行 ~vector() 时调用此函数 → 如果 GC_FREE 则可能 double-free
	//   4. 即使 buffer 尚未被回收，no-op 也安全：GC 下一轮会收走它
	void deallocate(T *, std::size_t) noexcept { /* no-op */ }

	template<typename U> bool operator==(const GCTracingAllocator<U> &) const noexcept
	{
		return true;
	}
	template<typename U> bool operator!=(const GCTracingAllocator<U> &) const noexcept
	{
		return false;
	}
};
#else
template<typename T> using GCTracingAllocator = std::allocator<T>;
#endif

template<typename T> using GCVector = std::vector<T, GCTracingAllocator<T>>;

}// namespace py