#pragma once

// =============================================================================
// Arena Allocator — Pylang 核心内存分配器 (兼容 Boehm GC 模式)
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

#include "memory/GCTracingAllocator.hpp"

#ifdef PYLANG_USE_Boehm_GC
#include "runtime/forward.hpp"
#include <gc.h>
#endif

namespace py {

class PyInteger;
class PyFloat;
class PyBool;
class PyNone;
class PyTuple;
class PyList;
class PyListIterator;
class PyTupleIterator;
class PyAsyncGenerator;
class PyCell;
class PyCode;
class PyCoroutine;
class PyDict;
class PyFrozenSet;
class PyFunction;
class PyFrame;
class PyGenerator;
class PyNativeFunction;
class PyNumber;
class PyMethodDescriptor;
class PyModule;
class PyObject;
class PySet;
class PySlice;
class PySlotWrapper;
class PyString;
class PyTraceback;
class PyType;
struct TypePrototype;
struct PyBuffer;

} // namespace py

// ===================================================================================
// [核心豁免名单] 那些表面有 C++ non-trivial 析构，但背地里所有状态全被托管的对象。
// 以下对象的 finalizer 注册将被无情抹除，直接缩减天文数字级别的回收列表！
// ===================================================================================
PYLANG_GC_FORCE_TRIVIAL(py::PyInteger)
PYLANG_GC_FORCE_TRIVIAL(py::PyFloat)
PYLANG_GC_FORCE_TRIVIAL(py::PyBool)
PYLANG_GC_FORCE_TRIVIAL(py::PyNone)
PYLANG_GC_FORCE_TRIVIAL(py::PyType)

namespace py {

template<typename T> constexpr bool gc_needs_finalizer()
{
#ifdef PYLANG_USE_Boehm_GC
    return !py::gc::is_trivial_dtor_v<T>;
#else
    return !std::is_trivially_destructible_v<T>;
#endif
}

class Arena
{
#ifdef PYLANG_USE_Boehm_GC
    // Boehm GC 终结器代理，在回收内存前安全地调用 C++ 析构
    template<typename T> static void gc_finalizer_proxy(void *obj, void *client_data)
    {
        (void)client_data;
        static_cast<T *>(obj)->~T();
    }
#endif

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
        : m_blocks(std::move(other.m_blocks)), m_destructors(std::move(other.m_destructors)),
          m_default_block_size(other.m_default_block_size),
          m_total_allocated(other.m_total_allocated)
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
    template<typename T, typename... Args> T *allocate(Args &&...args)
    {
        static_assert(
            alignof(T) <= alignof(std::max_align_t), "Over-aligned types not yet supported");

#ifdef PYLANG_USE_Boehm_GC
        void *mem = nullptr;
        // 路由进入智能战线判定体系：这保证了即使对象极其复杂，只要有豁免就能走快捷通道
        if constexpr (py::gc::is_atomic_v<T>) {
            mem = GC_MALLOC_ATOMIC(sizeof(T));
        } else {
            mem = GC_MALLOC(sizeof(T));
        }

        if (!mem) return nullptr;

        T *obj = new (mem) T(std::forward<Args>(args)...);

        if constexpr (gc_needs_finalizer<T>()) {
            GC_register_finalizer_no_order(mem, gc_finalizer_proxy<T>, nullptr, nullptr, nullptr);
        }
        return obj;
#else
        // 原有的 Bump allocation 逻辑
        void *mem = bump_allocate(sizeof(T), alignof(T));
        if (!mem) return nullptr;

        T *obj = new (mem) T(std::forward<Args>(args)...);

        // 非平凡析构类型需要注册析构函数
        if constexpr (!std::is_trivially_destructible_v<T>) {
            m_destructors.push_back(DtorEntry{ mem, [](void *p) { static_cast<T *>(p)->~T(); } });
        }
        return obj;
#endif
    }

    /// 带额外字节的分配 (变长对象: PyBytes, PyTuple 等)
    template<typename T, typename... Args>
    T *allocate_with_extra(size_t extra_bytes, Args &&...args)
    {
        static_assert(
            alignof(T) <= alignof(std::max_align_t), "Over-aligned types not yet supported");

#ifdef PYLANG_USE_Boehm_GC
        void *mem = nullptr;
        if constexpr (py::gc::is_atomic_v<T>) {
            mem = GC_MALLOC_ATOMIC(sizeof(T) + extra_bytes);
        } else {
            mem = GC_MALLOC(sizeof(T) + extra_bytes);
        }

        if (!mem) return nullptr;

        T *obj = new (mem) T(std::forward<Args>(args)...);

        if constexpr (gc_needs_finalizer<T>()) {
            GC_register_finalizer_no_order(mem, gc_finalizer_proxy<T>, nullptr, nullptr, nullptr);
        }
        return obj;
#else
        void *mem = bump_allocate(sizeof(T) + extra_bytes, alignof(T));
        if (!mem) return nullptr;

        T *obj = new (mem) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            m_destructors.push_back(DtorEntry{ mem, [](void *p) { static_cast<T *>(p)->~T(); } });
        }
        return obj;
#endif
    }

    /// 分配原始内存 (不调用构造函数)
    template<bool ContainsPointers = true>
    void *allocate_raw(size_t size, [[maybe_unused]] size_t align = alignof(std::max_align_t))
    {
#ifdef PYLANG_USE_Boehm_GC
        if constexpr (ContainsPointers) {
            return GC_MALLOC(size);
        } else {
            return GC_MALLOC_ATOMIC(size);
        }
#else
        return bump_allocate(size, align);
#endif
    }

    /// 批量释放
    void reset()
    {
#ifdef PYLANG_USE_Boehm_GC
        // GC 模式下，Arena 不控制生命周期，直接返回（保留为空）。
#else
        // 逆序析构
        for (auto it = m_destructors.rbegin(); it != m_destructors.rend(); ++it) {
            it->dtor(it->ptr);
        }
        m_destructors.clear();

        for (auto &block : m_blocks) { ::operator delete(block.memory); }
        m_blocks.clear();
        m_total_allocated = 0;
#endif
    }

    // ---- thread_local 当前 Arena ----
    static Arena &current()
    {
        assert(t_current_arena && "No active Arena on this thread");
        return *t_current_arena;
    }

    static void set_current(Arena *arena) { t_current_arena = arena; }
    static bool has_current() { return t_current_arena != nullptr; }

    // ---- 统计信息 ----
    size_t bytes_allocated() const { return m_total_allocated; }
    size_t block_count() const { return m_blocks.size(); }
    size_t destructor_count() const { return m_destructors.size(); }

    struct SavePoint
    {
        size_t block_count;
        size_t block_offset;
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

    void restore(const SavePoint &sp)
    {
#ifdef PYLANG_USE_Boehm_GC
        // GC 模式下，忽略回退
        (void)sp;
#else
        while (m_destructors.size() > sp.dtor_count) {
            auto &entry = m_destructors.back();
            entry.dtor(entry.ptr);
            m_destructors.pop_back();
        }
        while (m_blocks.size() > sp.block_count) {
            ::operator delete(m_blocks.back().memory);
            m_blocks.pop_back();
        }
        if (!m_blocks.empty() && m_blocks.size() == sp.block_count) {
            m_blocks.back().offset = sp.block_offset;
        }
        m_total_allocated = sp.total_allocated;
#endif
    }

  private:
    struct Block
    {
        uint8_t *memory;
        size_t capacity;
        size_t offset;
    };

    struct DtorEntry
    {
        void *ptr;
        void (*dtor)(void *);
    };

    void *bump_allocate(size_t size, size_t align)
    {
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

        add_block(size);
        auto &current = m_blocks.back();
        size_t aligned_offset = align_up(current.offset, align);

        void *ptr = current.memory + aligned_offset;
        current.offset = aligned_offset + size;
        m_total_allocated += size;
        return ptr;
    }

    void add_block(size_t min_size)
    {
        size_t block_size = m_default_block_size;
        if (!m_blocks.empty()) {
            block_size = std::min(m_blocks.back().capacity * 2, kMaxBlockSize);
        }
        block_size = std::max(block_size, min_size + alignof(std::max_align_t));
        auto *memory = static_cast<uint8_t *>(::operator new(block_size));
        m_blocks.push_back(Block{ memory, block_size, 0 });
    }

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