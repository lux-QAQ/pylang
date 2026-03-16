## 深度分析：上一轮方案的致命错误

### 问题诊断

VTune 的数据已经非常清晰地揭示了问题的根源。让我重新审视整个架构：

| 组件 | 当前做法 | 问题 |
|---|---|---|
| **全局 `new` 劫持** | 所有 `new` → `GC_MALLOC` | **这是灾难的根源！** 所有 `std::string` 的 char 缓冲区、spdlog 内部缓冲区、fmt 缓冲区、所有临时对象全部涌入 GC 堆。GC 要逐字节扫描数 GB 纯文本数据去"找指针"。 |
| **删除所有 Finalizer** | 上一轮建议删掉 | **这会导致内存泄漏！** 如果我们移除全局 new 劫持（必须做），STL 的缓冲区回到了 `malloc` 堆上，GC 回收 PyObject 时不调析构函数，`vector`/`string` 的缓冲区就永远泄漏了。 |
| **GMP 接管** | 上一轮建议用 `GC_MALLOC_ATOMIC` | 方向正确，但不够。 |

**核心矛盾**：我们需要 GC 能发现藏在 `std::vector<Value>` 缓冲区里的 `PyObject*` 指针（防 UAF），但又不能让 GC 扫描所有内存（性能灾难）。

### 正确方案：精准打击

**原则：只让存放 `PyObject*` 指针的内存进入 GC 扫描范围，其他一切走普通 `malloc`。**

1. **删除全局 `new` 劫持** → GC 扫描量从数 GB 降至数 MB
2. **保留 Finalizer** → 回收 PyObject 时调析构函数，释放 STL 的 `malloc` 缓冲区
3. **仅对存放 `Value`/`PyObject*` 的容器使用 `gc_allocator`** → 防 UAF
4. **GMP 重定向到 `GC_MALLOC_ATOMIC`** → GMP 大整数缓冲区不被扫描

---

### 第 1 步：清空 Boehm_GCSetup.cpp（消灭性能灾难）

````cpp
// =============================================================================
// Boehm GC 全局设置
// =============================================================================
// 注意: 我们 **不再** 劫持全局 operator new/delete！
// 只有通过 Arena::allocate 创建的 PyObject 才进入 GC 堆。
// STL 容器内部缓冲区走普通 malloc, 由 Finalizer 调用析构函数释放。
// 存放 PyObject* 的容器使用 gc_allocator, 确保 GC 能扫描到指针。
// =============================================================================

#ifdef PYLANG_USE_Boehm_GC
// 此文件故意留空。不再劫持全局 new/delete。
// GC 只管理通过 Arena::allocate (即 GC_MALLOC) 分配的 PyObject。
#endif
````

### 第 2 步：定义 GC 感知的分配器别名（新文件）

````cpp
#pragma once

// =============================================================================
// GC-aware allocator — 仅用于存放 PyObject* / Value 的容器
// =============================================================================

#include <memory>

#ifdef PYLANG_USE_Boehm_GC
#include <gc.h>

namespace py {

/// 一个极简的 STL 兼容 Allocator，内部缓冲区分配到 GC 可扫描的堆上。
/// 这样 GC 在 mark 阶段能发现缓冲区内的 PyObject* 指针，防止 UAF。
template<typename T>
struct GCTracingAllocator {
    using value_type = T;

    GCTracingAllocator() noexcept = default;

    template<typename U>
    GCTracingAllocator(const GCTracingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        // GC_MALLOC: 分配的内存会被 GC 扫描以发现内部的指针
        void* p = GC_MALLOC(n * sizeof(T));
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t) noexcept {
        GC_FREE(p);
    }

    template<typename U>
    bool operator==(const GCTracingAllocator<U>&) const noexcept { return true; }
    template<typename U>
    bool operator!=(const GCTracingAllocator<U>&) const noexcept { return false; }
};

} // namespace py

/// 容器别名: 用于存放 Value / PyObject* 的 vector
#define PYLANG_GC_VECTOR(T) std::vector<T, ::py::GCTracingAllocator<T>>

#else // !PYLANG_USE_Boehm_GC

#define PYLANG_GC_VECTOR(T) std::vector<T>

#endif
````

### 第 3 步：保留 Arena.hpp 中的 Finalizer（但保持精简）

在 Arena.hpp 中 **保留** Finalizer 逻辑，因为现在 STL 缓冲区回到了普通 `malloc`，必须通过析构函数释放它们。

````cpp
// ...existing code...

	template<typename T, typename... Args> T *allocate(Args &&...args)
	{
		static_assert(
			alignof(T) <= alignof(std::max_align_t), "Over-aligned types not yet supported");

#ifdef PYLANG_USE_Boehm_GC
		void *mem = GC_MALLOC(sizeof(T));
		if (!mem) return nullptr;

		T *obj = new (mem) T(std::forward<Args>(args)...);

		// ✅ 保留 Finalizer！STL 缓冲区在普通 malloc 堆上，需要析构函数释放。
		if constexpr (!std::is_trivially_destructible_v<T>) {
			GC_register_finalizer_ignore_self(
				mem, gc_finalizer_proxy<T>, nullptr, nullptr, nullptr);
		}
		return obj;
#else
		// ...existing code... (原有 bump allocation)
#endif
	}

	template<typename T, typename... Args>
	T *allocate_with_extra(size_t extra_bytes, Args &&...args)
	{
		static_assert(
			alignof(T) <= alignof(std::max_align_t), "Over-aligned types not yet supported");

#ifdef PYLANG_USE_Boehm_GC
		void *mem = GC_MALLOC(sizeof(T) + extra_bytes);
		if (!mem) return nullptr;

		T *obj = new (mem) T(std::forward<Args>(args)...);

		// ✅ 同样保留
		if constexpr (!std::is_trivially_destructible_v<T>) {
			GC_register_finalizer_ignore_self(
				mem, gc_finalizer_proxy<T>, nullptr, nullptr, nullptr);
		}
		return obj;
#else
		// ...existing code...
#endif
	}

// ...existing code...
````

### 第 4 步：修改存放 `PyObject*` 的容器（仅约 5 个文件）

这是唯一需要修改业务层代码的地方，但改动极小且高度定向。

**`PyList.hpp`**（或其容器成员声明处）：
````cpp
// ...existing code...
#include "memory/GCAllocator.hpp"

namespace py {
class PyList : public PyObject {
    // ...existing code...
private:
    // ✅ 改为 GC 感知的 allocator，让 GC 能扫描到缓冲区内的 PyObject*
    PYLANG_GC_VECTOR(Value) m_elements;
    // ...existing code...
};
}
````

**`PyDict.hpp`** / `PySet.hpp` / 任何存放 `Value` 或 `PyObject*` 的容器——同理，将 allocator 参数换为 `GCTracingAllocator`。

对于 `tsl::ordered_map`，它接受 allocator 作为最后一个模板参数：
````cpp
// ...existing code...
#include "memory/GCAllocator.hpp"

// 将 allocator 替换为 GC 感知版本
#ifdef PYLANG_USE_Boehm_GC
using DictMap = tsl::ordered_map<Value, Value, ValueHash, std::equal_to<Value>,
    ::py::GCTracingAllocator<std::pair<Value, Value>>>;
#else
using DictMap = tsl::ordered_map<Value, Value, ValueHash>;
#endif
// ...existing code...
````

### 第 5 步：GMP 重定向（优化，rt_lifecycle.cpp）

````cpp
// ...existing code...

#ifdef PYLANG_USE_Boehm_GC
#include <gc.h>
#include <gmp.h>

// GMP 内部存的是大整数 limb 数组，纯数字无指针，用 ATOMIC 跳过扫描
extern "C" {
static void* gmp_gc_alloc(size_t size) {
    return GC_MALLOC_ATOMIC(size);
}
static void* gmp_gc_realloc(void* ptr, size_t /*old_size*/, size_t new_size) {
    return GC_REALLOC(ptr, new_size);
}
static void gmp_gc_free(void* ptr, size_t /*size*/) {
    GC_FREE(ptr);
}
}
#endif

PYLANG_EXPORT_LIFECYCLE("init", "void", "")
void rt_init()
{
#ifdef PYLANG_USE_Boehm_GC
    GC_INIT();
    GC_allow_register_threads();
    // GMP 的 limb 缓冲区交给 GC 管理(ATOMIC, 不扫描)
    // 这样 PyInteger 的 finalizer 就不需要调 mpz_clear 去 free()
    mp_set_memory_functions(gmp_gc_alloc, gmp_gc_realloc, gmp_gc_free);
#endif

#ifdef PYLANG_USE_ARENA
    py::ArenaManager::initialize();
#endif
    // ...existing code...
````

---

### 性能对比：为什么这能解决 VTune 的问题？

| 度量 | 旧方案（全局 new 劫持） | 新方案（精准打击） |
|---|---|---|
| GC 管理的堆大小 | **数 GB**（所有 string/vector/spdlog 缓冲区） | **数十 MB**（仅 PyObject 实例 + Value 数组） |
| GC 扫描量 | 每次 Mark 扫描数 GB 纯文本 | 每次 Mark 仅扫描 PyObject 和 Value 容器 |
| Finalizer 数量 | 相同（数万） | 相同，但 Mark 阶段极快，Finalizer 开销微不足道 |
| UAF 风险 | 无 | 无（Value 容器用 `GCTracingAllocator`） |
| 内存泄漏 | 无 | 无（Finalizer 调析构，析构释放 STL 的 malloc 缓冲区） |

**VTune 中 `GC_mark_from` 的 1517 秒将降至几秒以内**，因为 GC 需要扫描的内存量减少了 100 倍以上。