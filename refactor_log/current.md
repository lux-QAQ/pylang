# Pylang Runtime 与编译器重构实施文档

本文档描述从当前状态（`compat.hpp` 宏已部署，目标为 `shared_ptr`）到最终状态（Arena/Region-based 内存管理）的具体迁移路径。

**前置文档**: `refactor.md`（总体愿景）、`Multi-file-compilation.md`（Import/Module 架构）

## 1. 现状评估

### 1.1 已完成的宏替换

`compat.hpp` 引入了一层编译期间接层，将分配调用从硬编码的 `VirtualMachine::the().heap()` 抽离为宏。这一步工作**与最终方案无关地正确**——无论目标是 `shared_ptr` 还是 Arena，宏间接层都是必需的。

| 宏 | 替换状态 | 当前展开目标 (旧路径) |
|:---|:---|:---|
| `PYLANG_ALLOC(Type, ...)` | ✅ 全局替换完成 | `VirtualMachine::the().heap().allocate<Type>(...)` |
| `PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...)` | ✅ 全局替换完成 | `heap().allocate_with_extra_bytes<Type>(Extra, ...)` |
| `PYLANG_GC_PAUSE_SCOPE()` | ✅ 全局替换完成 | `heap().scoped_gc_pause()` |
| `PYLANG_CHECK_ALLOC(ptr, Type)` | ⚠️ 部分替换 | `if (!ptr) return Err(memory_error(...))` |
| `PYLANG_ALLOC_WEAKREF(Type, Obj, Cb)` | ✅ weakref 模块内使用 | `heap().allocate_weakref<Type>(Obj, Cb)` |
| `PYLANG_WEAKREF_ALIVE(ObjPtr)` | ✅ weakref 模块内使用 | `heap().has_weakref_object(...)` |
| `PYLANG_WEAKREF_COUNT(ObjPtr)` | ✅ weakref/module.cpp 使用 | `heap().weakref_count(...)` |
| `PYLANG_WEAKREF_LIST(ObjPtr)` | ✅ weakref/module.cpp 使用 | `heap().get_weakrefs(...)` |
| `PYLANG_VISIT_GRAPH_BODY` | ❌ 未使用 | — |
| `PYLANG_CREATE` | ❌ 未使用 | — |
| `PYLANG_USE_SHARED_PTR` | ❌ 从未定义 | — |

### 1.2 关键认识

1. **`shared_ptr` 路径从未激活**：`PYLANG_USE_SHARED_PTR` 从未被定义，`compat.hpp` 中 `#ifdef` 分支内的所有代码（`s_legacy_registry`、`NullVisitor`、`CellCompat` 等）从未编译过。
2. **宏间接层是正资产**：所有分配点已经通过宏调用，切换后端只需改宏定义，不需要再次全局搜索替换。
3. **weakref 是特殊挑战**：Arena 模型下对象不被单独释放，weakref 的"目标已死"语义需要重新设计。

### 1.3 需要清理的代码

`compat.hpp` 中以下内容将被移除：

- `#ifdef PYLANG_USE_SHARED_PTR` 分支的全部内容
- `py::compat::s_legacy_registry` 及相关函数
- `py::compat::NullVisitor`
- `py::compat::CellCompat`
- `py::compat::ScopedNoOp`
- `#include "memory.hpp"` (shared_ptr 相关的头文件)

---

## 2. compat.hpp 重写：从 shared_ptr 桥接到 Arena 桥接

### 2.1 新版 compat.hpp 设计

```cpp
// filepath: src/runtime/compat.hpp
#pragma once

// =============================================================================
// Pylang 内存分配兼容层 (Arena Migration)
// =============================================================================
//
// 迁移策略:
//   Phase A (当前): PYLANG_USE_ARENA 未定义
//     → 所有宏展开为旧的 Heap + GC 路径，行为不变
//   Phase B: 在 CMake 中定义 PYLANG_USE_ARENA
//     → 所有宏展开为 Arena 路径
//   Phase C: 全局启用 Arena 后，删除旧路径代码
//     → compat.hpp 简化为直接调用 Arena API
//
// =============================================================================

#ifdef PYLANG_USE_ARENA

// ---- 新路径: Arena ----
#include "memory/Arena.hpp"
#include "memory/ArenaManager.hpp"

#define PYLANG_ALLOC(Type, ...) \
    ::py::Arena::current().allocate<Type>(__VA_ARGS__)

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
    ::py::Arena::current().allocate_with_extra<Type>(Extra, __VA_ARGS__)

// Arena 模式下无 GC，暂停操作为 no-op
#define PYLANG_GC_PAUSE_SCOPE() ((void)0)

// WeakRef: Arena 模式下的语义重定义 (详见 §3)
#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
    ::py::Arena::current().allocate<Type>(Obj, Callback)

#define PYLANG_WEAKREF_ALIVE(ObjPtr) \
    ::py::weakref::is_alive(ObjPtr)

#define PYLANG_WEAKREF_COUNT(ObjPtr) \
    ::py::weakref::ref_count(ObjPtr)

#define PYLANG_WEAKREF_LIST(ObjPtr) \
    ::py::weakref::get_refs(ObjPtr)

#else
// ---- 旧路径: Heap + GC (不变) ----

#include "vm/VM.hpp"

#define PYLANG_ALLOC(Type, ...) \
    VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__)

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
    VirtualMachine::the().heap().allocate_with_extra_bytes<Type>(Extra, __VA_ARGS__)

#define PYLANG_GC_PAUSE_SCOPE() \
    [[maybe_unused]] auto _pylang_gc_pause_##__LINE__ = \
        VirtualMachine::the().heap().scoped_gc_pause()

#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
    VirtualMachine::the().heap().allocate_weakref<Type>(Obj, Callback)

#define PYLANG_WEAKREF_ALIVE(ObjPtr) \
    VirtualMachine::the().heap().has_weakref_object(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_WEAKREF_COUNT(ObjPtr) \
    VirtualMachine::the().heap().weakref_count(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_WEAKREF_LIST(ObjPtr) \
    VirtualMachine::the().heap().get_weakrefs(std::bit_cast<uint8_t *>(ObjPtr))

#endif // PYLANG_USE_ARENA


// =============================================================================
// 通用辅助宏（两种模式下都可用）
// =============================================================================

#define PYLANG_CHECK_ALLOC(ptr, Type)                           \
    do {                                                        \
        if (!(ptr)) { return Err(memory_error(sizeof(Type))); } \
    } while (0)
```

### 2.2 对比旧版 compat.hpp 的变化

| 项目 | 旧版 (shared_ptr 桥接) | 新版 (Arena 桥接) |
|:---|:---|:---|
| 编译开关 | `PYLANG_USE_SHARED_PTR` | `PYLANG_USE_ARENA` |
| 新路径目标 | `std::make_shared` + 全局注册表 | `Arena::current().allocate` |
| 注册表 | `s_legacy_registry` (mutex 保护) | 不需要 |
| GC 暂停 | `ScopedNoOp` | `((void)0)` |
| WeakRef | "暂时假定总是存活" (hack) | 显式 tombstone 机制 (§3) |
| `CellCompat` | 空壳 Cell 替代品 | 不需要 (直接移除 Cell) |
| 代码量 | ~120 行 | ~50 行 |

---

## 3. WeakRef 在 Arena 模型下的重设计

### 3.1 核心矛盾

Arena 模型的核心是：**对象不被单独释放，随 Arena 批量释放**。但 `weakref` 的语义要求：**当目标对象"死亡"时，弱引用应返回 `None`**。

在 GC 模型中，"死亡"由 GC 的 sweep 阶段确定。在 Arena 模型中，对象在 Arena 存活期间始终有效——不存在"单个对象死亡"的概念。

### 3.2 解决方案：显式 Tombstone

在 Arena 模型下，"死亡"不再由内存系统自动判定，而是由**显式标记**决定：

```cpp
// filepath: src/runtime/weakref/WeakRefRegistry.hpp
#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace py {

class PyObject;

namespace weakref {

/// 全局弱引用注册表
/// 当对象被逻辑销毁时 (如 del 语句, 作用域退出),
/// 调用 invalidate() 标记其 tombstone, 所有弱引用随之失效
class WeakRefRegistry {
public:
    static WeakRefRegistry& instance() {
        static WeakRefRegistry reg;
        return reg;
    }

    /// 注册一个弱引用关系: ref_obj 弱引用 target
    void register_ref(PyObject* target, PyObject* ref_obj) {
        std::lock_guard lock(m_mutex);
        m_refs[target].insert(ref_obj);
        m_alive.insert(target);
    }

    /// 标记 target 为"逻辑死亡"
    /// 所有引用它的 weakref 将在下次 is_alive() 时返回 false
    void invalidate(PyObject* target) {
        std::lock_guard lock(m_mutex);
        m_alive.erase(target);
        // 注意: 不从 m_refs 中移除，weakref 对象的 is_alive() 会检查 m_alive
    }

    /// 检查 target 是否仍然"逻辑存活"
    bool is_alive(PyObject* target) const {
        std::lock_guard lock(m_mutex);
        return m_alive.contains(target);
    }

    /// 获取 target 的弱引用计数
    size_t ref_count(PyObject* target) const {
        std::lock_guard lock(m_mutex);
        auto it = m_refs.find(target);
        if (it == m_refs.end()) return 0;
        return it->second.size();
    }

    /// 获取 target 的所有弱引用对象
    std::vector<PyObject*> get_refs(PyObject* target) const {
        std::lock_guard lock(m_mutex);
        auto it = m_refs.find(target);
        if (it == m_refs.end()) return {};
        return {it->second.begin(), it->second.end()};
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<PyObject*, std::unordered_set<PyObject*>> m_refs;
    std::unordered_set<PyObject*> m_alive;

    WeakRefRegistry() = default;
};

// compat.hpp 宏的后端函数
inline bool is_alive(PyObject* obj) {
    return WeakRefRegistry::instance().is_alive(obj);
}

inline size_t ref_count(PyObject* obj) {
    return WeakRefRegistry::instance().ref_count(obj);
}

inline std::vector<PyObject*> get_refs(PyObject* obj) {
    return WeakRefRegistry::instance().get_refs(obj);
}

} // namespace weakref
} // namespace py
```

### 3.3 weakref 模块文件的改动

现有的 `PyWeakRef`/`PyWeakProxy`/`PyCallableProxyType` 的 `create()` 函数已经使用 `PYLANG_ALLOC_WEAKREF` 宏。在 Arena 模式下只需在分配后向 `WeakRefRegistry` 注册：

```cpp
// PyWeakRef::create() — Arena 模式下
PyResult<PyWeakRef*> PyWeakRef::create(PyObject* object, PyObject* callback) {
    auto* result = PYLANG_ALLOC_WEAKREF(PyWeakRef, object, callback);
    PYLANG_CHECK_ALLOC(result, PyWeakRef);

#ifdef PYLANG_USE_ARENA
    weakref::WeakRefRegistry::instance().register_ref(object, result);
#endif

    return Ok(result);
}
```

`is_alive()` 已经使用 `PYLANG_WEAKREF_ALIVE` 宏，自动切换到 `WeakRefRegistry::is_alive()`。

### 3.4 谁调用 invalidate？

| 场景 | 触发时机 | 调用方 |
|:---|:---|:---|
| `del obj` | 编译器生成的 IR | `weakref::WeakRefRegistry::instance().invalidate(obj)` |
| ScopedRegion 析构 | 函数返回时 RAII | Arena 的析构回调遍历已注册对象 |
| 模块卸载 | 程序退出时 | ArenaManager::shutdown() |

对于 ScopedRegion，Arena 需要追踪哪些对象有弱引用注册：

```cpp
// Arena 析构时的回调
void Arena::reset() {
    // 通知 WeakRefRegistry 标记所有本 Arena 内的对象为"死亡"
    for (auto* obj : m_weak_tracked_objects) {
        weakref::WeakRefRegistry::instance().invalidate(obj);
    }
    // 然后批量释放内存...
}
```

### 3.5 visit_graph 的处理

现有的 `PyWeakRef::visit_graph()`、`PyWeakProxy::visit_graph()`、`PyCallableProxyType::visit_graph()` 在 Arena 模式下不再需要。处理方式：

```cpp
void PyWeakRef::visit_graph(Visitor& visitor) {
#ifndef PYLANG_USE_ARENA
    PyObject::visit_graph(visitor);
    if (m_callback) visitor.visit(*m_callback);
#else
    (void)visitor;
#endif
}
```

最终 Phase C（全局 Arena 启用后）直接删除所有 `visit_graph` 实现。

---

## 4. Arena 基础设施实现

### 4.1 Arena 核心类

```cpp
// filepath: src/memory/Arena.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <vector>

namespace py {

class Arena {
public:
    static constexpr size_t kDefaultBlockSize = 64 * 1024;  // 64KB

    explicit Arena(size_t block_size = kDefaultBlockSize);
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) noexcept;
    Arena& operator=(Arena&&) noexcept;

    /// 主分配接口 — O(1) bump allocation
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        void* mem = bump_allocate(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        T* obj = new (mem) T(std::forward<Args>(args)...);
        // 记录析构函数以便 reset() 时调用
        m_destructors.push_back({mem, [](void* p) {
            static_cast<T*>(p)->~T();
        }});
        return obj;
    }

    /// 带额外字节的分配 (变长对象: PyBytes, PyTuple 等)
    template<typename T, typename... Args>
    T* allocate_with_extra(size_t extra_bytes, Args&&... args) {
        void* mem = bump_allocate(sizeof(T) + extra_bytes, alignof(T));
        if (!mem) return nullptr;
        T* obj = new (mem) T(std::forward<Args>(args)...);
        m_destructors.push_back({mem, [](void* p) {
            static_cast<T*>(p)->~T();
        }});
        return obj;
    }

    /// 批量释放：逆序调用析构函数，然后释放内存块
    void reset();

    /// 当前 thread_local Arena
    static Arena& current();

    /// 统计
    size_t bytes_allocated() const { return m_total_allocated; }
    size_t block_count() const { return m_blocks.size(); }

private:
    struct Block {
        uint8_t* memory;
        size_t capacity;
        size_t offset;
    };

    struct DtorEntry {
        void* ptr;
        void (*dtor)(void*);
    };

    void* bump_allocate(size_t size, size_t align);
    void add_block(size_t min_size);

    std::vector<Block> m_blocks;
    std::vector<DtorEntry> m_destructors;
    size_t m_default_block_size;
    size_t m_total_allocated{0};
};

} // namespace py
```

### 4.2 ArenaManager

```cpp
// filepath: src/memory/ArenaManager.hpp
#pragma once

#include "Arena.hpp"
#include <mutex>
#include <string>
#include <unordered_map>

namespace py {

class ArenaManager {
public:
    static void initialize();
    static void shutdown();

    /// 全局 ProgramArena — Immortal 对象 (PyType, None, True, False)
    static Arena& program_arena();

    /// Per-module Arena — 模块级对象
    static Arena& module_arena(const std::string& module_name);

    /// Per-thread Arena — thread_local, 无需加锁
    static Arena& thread_arena();

private:
    static std::mutex s_mutex;
    static Arena s_program_arena;
    static std::unordered_map<std::string, Arena> s_module_arenas;
};

} // namespace py
```

### 4.3 ScopedRegion

```cpp
// filepath: src/memory/ScopedRegion.hpp
#pragma once

#include "Arena.hpp"

namespace py {

/// 函数级作用域 Region
/// 构造时创建新 Arena 并设为当前 Arena
/// 析构时恢复之前的 Arena 并释放 Region 内所有对象
struct ScopedRegion {
    Arena region;
    Arena* previous;

    ScopedRegion()
        : previous(&Arena::current()) {
        // 将 thread_local 指针切换到 region
        set_current_arena(&region);
    }

    ~ScopedRegion() {
        region.reset();  // 析构所有对象 + 释放内存
        set_current_arena(previous);
    }

    ScopedRegion(const ScopedRegion&) = delete;
    ScopedRegion& operator=(const ScopedRegion&) = delete;

private:
    static void set_current_arena(Arena* a);
};

} // namespace py
```

---

## 5. Cell 基类与 GC 接口的移除

### 5.1 现有继承链

```
Cell (GC 遍历接口)
  └─ PyBaseObject (GarbageCollected header)
       └─ PyInteger, PyString, PyList, ...
```

### 5.2 目标继承链

```
PyBaseObject (纯对象基类, 无 GC)
  └─ PyInteger, PyString, PyList, ...
```

### 5.3 分阶段移除

**Phase B-1: 条件编译 `visit_graph`**

不立即删除，而是用条件编译使其在 Arena 模式下变为空：

```cpp
// PyObject.hpp
class PyBaseObject
#ifndef PYLANG_USE_ARENA
    : public Cell
#endif
{
public:
    // ...existing interface...

#ifndef PYLANG_USE_ARENA
    void visit_graph(Visitor& visitor) override;
#endif
};
```

**Phase B-2: 移除 GarbageCollected header**

在 `PYLANG_USE_ARENA` 模式下，对象内存布局不再包含 2-bit 颜色标记：

```
旧布局: [GarbageCollected(2bit)] [Cell vtable] [PyBaseObject data]
新布局: [PyBaseObject vtable] [PyBaseObject data]
```

这会改变所有对象的内存偏移。由于使用宏分配，分配路径已经隔离，只需确保 Arena 的 `allocate<T>()` 不预留 header 空间。

**Phase C: 全局启用后清理**

- 删除 `src/memory/GarbageCollector.hpp` / `.cpp`
- 删除 `src/memory/Heap.hpp` / `.cpp`
- 删除 `Cell` 类
- 删除 `GarbageCollected` 类
- 从所有 `PyObject` 子类中删除 `visit_graph()` 方法
- 简化 `compat.hpp` 为直接 Arena 调用

---

## 6. VM 依赖解耦

### 6.1 问题

现有代码通过 `VirtualMachine::the()` 全局单例访问几乎所有基础设施：

```
VirtualMachine::the()
  ├── .heap()              → 内存分配 (被 PYLANG_ALLOC 替换)
  ├── .interpreter()       → 模块表、builtin 查找
  ├── .stack_objects()     → GC 根集
  └── .m_stack / m_stack_pointer → 栈帧管理
```

`PYLANG_ALLOC` 已经隔离了 `.heap()` 路径。但仍有大量代码直接依赖 `VirtualMachine::the()`：

| 依赖路径 | 使用位置 | Arena 方案替代 |
|:---|:---|:---|
| `heap().allocate` | 已替换为宏 | `Arena::current().allocate` |
| `heap().scoped_gc_pause()` | 已替换为宏 | no-op |
| `heap().allocate_weakref` | 已替换为宏 | `Arena + WeakRefRegistry` |
| `interpreter().modules()` | Import.cpp | `ModuleRegistry` |
| `interpreter().execution_frame()` | PyFrame 相关 | 编译器生成的栈帧 |
| `stack_objects()` | GC 根集 | 移除 (Arena 不需要根集) |

### 6.2 解耦步骤

Phase B 中，`compat.hpp` 的 `PYLANG_USE_ARENA` 分支不再 `#include "vm/VM.hpp"`。这意味着所有通过宏间接的路径自动解耦。

剩余的直接 `VirtualMachine::the()` 调用需要逐个处理：

```bash
# 查找剩余的直接 VM 依赖
grep -rn "VirtualMachine::the()" src/runtime/ --include="*.cpp" \
    | grep -v "compat.hpp"
```

---

## 7. 迁移执行计划

### Phase A: 准备工作 (当前 → 就绪)

- [x] 所有分配点已通过 `PYLANG_ALLOC` / `PYLANG_ALLOC_WITH_EXTRA` 宏化
- [x] GC 暂停已通过 `PYLANG_GC_PAUSE_SCOPE` 宏化
- [x] WeakRef 分配已通过 `PYLANG_ALLOC_WEAKREF` 宏化
- [ ] 完成剩余的 `PYLANG_CHECK_ALLOC` 替换
- [ ] 清理 `compat.hpp` 中未使用的代码 (`PYLANG_VISIT_GRAPH_BODY`, `PYLANG_CREATE`, 整个 `PYLANG_USE_SHARED_PTR` 分支)

### Phase B: Arena 基础设施 + 双路径共存

- [ ] 实现 `src/memory/Arena.hpp` (bump allocator)
- [ ] 实现 `src/memory/ArenaManager.hpp` (Program/Module/Thread Arena)
- [ ] 实现 `src/memory/ScopedRegion.hpp`
- [ ] 实现 `src/runtime/weakref/WeakRefRegistry.hpp`
- [ ] 重写 `compat.hpp`：添加 `PYLANG_USE_ARENA` 分支 (如 §2.1)
- [ ] 在 CMake 中为测试目标定义 `PYLANG_USE_ARENA`，逐个测试通过
- [ ] `PyBaseObject` 条件编译移除 `Cell` 继承
- [ ] `visit_graph()` 条件编译为空

### Phase C: 全局启用 Arena + 清理

- [ ] CMake 全局定义 `PYLANG_USE_ARENA`
- [ ] 确认所有测试通过
- [ ] 删除 `src/memory/GarbageCollector.hpp` / `.cpp`
- [ ] 删除 `src/memory/Heap.hpp` / `.cpp`
- [ ] 删除 `Cell` 类、`GarbageCollected` 类
- [ ] 删除所有 `visit_graph()` 实现
- [ ] 简化 `compat.hpp`：移除旧路径分支，宏直接展开为 Arena 调用
- [ ] 移除 `VirtualMachine::the().heap()` 相关代码

### Phase D: Module/Import 重构

- [ ] 实现 `ModuleRegistry` (含并发支持, 详见 Multi-file-compilation.md §2)
- [ ] 实现 `ModuleMangler` (详见 Multi-file-compilation.md §3)
- [ ] 改造现有 modules 为 `PyInit_xxx` 模式
- [ ] `PyModule` 增加 `KeyVersionTracker` (详见 Multi-file-compilation.md §4)
- [ ] 实现 `DynamicImportFallback`

### Phase E: 编译器集成

- [ ] Runtime 预编译为 `runtime.bc`
- [ ] 编译器 Codegen 生成 Init 函数 + inline cache
- [ ] 编译器逃逸分析决定 ScopedRegion vs ModuleArena
- [ ] 生成 `main()` 入口

---

## 8. 风险与缓解

| 风险 | 影响 | 缓解措施 |
|:---|:---|:---|
| Arena 不释放单个对象，长运行程序内存增长 | 服务端场景内存压力 | 逃逸分析 + ScopedRegion 确保函数级对象及时释放；ModuleArena 是固定开销 |
| WeakRef tombstone 需要手动 `invalidate` | 忘记调用导致 weakref 永不失效 | 编译器在 `del` 语句和 ScopedRegion 析构时自动生成 invalidate 调用 |
| Phase B 双路径共存增加复杂度 | 编译矩阵翻倍 | CI 同时测试两个 define 配置；Phase C 尽快完成清理 |
| `allocate_with_extra` 的变长对象布局 | Arena bump allocator 需要处理对齐 | `bump_allocate(sizeof(T) + extra, alignof(T))` 已内置对齐 |
| 多线程下 `WeakRefRegistry` 的 mutex 争用 | 高频 weakref 场景性能下降 | 采用分片锁 (sharded lock) 或 per-thread 注册表 |

---

## 9. 文件变更索引

| 文件 | 操作 | Phase |
|:---|:---|:---|
| `src/runtime/compat.hpp` | **重写** (移除 shared_ptr, 添加 Arena 分支) | B |
| `src/memory/Arena.hpp` | **新建** | B |
| `src/memory/Arena.cpp` | **新建** | B |
| `src/memory/ArenaManager.hpp` | **新建** | B |
| `src/memory/ArenaManager.cpp` | **新建** | B |
| `src/memory/ScopedRegion.hpp` | **新建** | B |
| `src/runtime/weakref/WeakRefRegistry.hpp` | **新建** | B |
| `src/runtime/PyObject.hpp` | **修改** (条件编译移除 Cell 继承) | B |
| `src/runtime/modules/weakref/*.cpp` | **修改** (注册到 WeakRefRegistry) | B |
| `src/memory/GarbageCollector.hpp` | **删除** | C |
| `src/memory/GarbageCollector.cpp` | **删除** | C |
| `src/memory/Heap.hpp` | **删除** | C |
| `src/memory/Heap.cpp` | **删除** | C |
| `src/runtime/memory.hpp` | **删除** (shared_ptr 相关) | C |
| `src/runtime/ModuleRegistry.hpp` | **新建** | D |
| `src/runtime/ModuleMangler.hpp` | **新建** | D |
| `src/runtime/KeyVersionTracker.hpp` | **新建** | D |
| `src/runtime/DynamicImport.hpp` | **新建** | D |