# Pylang 重构计划 (Refactor Plan)

本文档描述了将现有 `Python C++` 项目重构为 `Pylang` 的技术路线图。`Pylang` 是一个高性能、兼容 Python 3.10+ 语法的编译型语言实现，利用 LLVM 生态系统进行静态分析与优化。

## 1. 核心愿景 (Vision)

*   **生态兼容**: 保持 Python 语法与语义的兼容性（不支持 `eval` 等运行时动态特性）。
*   **高性能**:
    *   **无 GIL (No Global Interpreter Lock)**: 无全局锁，利用 Arena 的线程局部性实现真正的并行。
    *   **LLVM 优化**: 利用 LLVM 后端进行死代码消除 (DCE)、内联 (Inlining) 和机器码生成。
    *   **零引用计数开销**: 不使用 `shared_ptr`，消除原子引用计数的性能损耗。
*   **架构革新**:
    *   `解释器 (VM)` → `编译器 (Compiler Frontend)`
    *   `手写 Mark-Sweep GC + Slab 分配器` → `Arena/Region-based 内存管理`

## 2. 架构概览 (Architecture)

整体架构转变为"编译器前端 + 静态运行时库"的编译流程。前端负责将 Python 源码转换为 LLVM IR，并与预编译的 Runtime Bitcode 进行链接。

```mermaid
graph TD
    subgraph "Development Time"
    R_SRC[Runtime C++ Source] -->|Clang| R_BC[Runtime Bitcode (.bc)]
    end

    subgraph "Compile Time (Pylang Compiler)"
    A[Python Source Code] -->|Parser| B(AST)
    B -->|Codegen| C(LLVM IR Module)
    R_BC -.->|Load & Link| C
    end
    
    subgraph "Native Compilation"
    C -->|LLVM/Clang Backend| D[Object File (.o)]
    D -->|Linker| E[Native Executable]
    end
```

### 2.1 编译执行流程
1.  **Runtime 预编译**: `src/runtime` 下的 C++ 代码被 Clang 编译为 LLVM Bitcode (`runtime.bc`)。这些代码中包含带有特殊 Attribute 标记的导出函数。
2.  **前端解析**: 用户输入的 Python 源码经过 Parser 生成 AST。
3.  **代码生成 (Lowering)**: 
    *   编译器前端加载 `runtime.bc`。
    *   遍历 AST，生成对应的 LLVM IR。
    *   当需要调用 Runtime 功能（如 `PyInteger::add`）时，前端根据 Attribute 在 `runtime.bc` 中查找对应的函数原型并生成调用指令。
4.  **优化与链接**: 生成的用户模块 IR 与 Runtime IR 进行模块级链接 (Link Modules)，此时大量 Runtime 函数（如小整数加法）可被 LLVM 内联优化。
5.  **后端输出**: 链接后的模块交给 LLVM Backend 生成目标机器码或可执行文件。

## 3. 内存管理重构 (Memory Management Refactor)

### 3.1 设计决策与理由

**彻底否决 `shared_ptr` 方案**。原因链：

| 方案 | 问题 | 结论 |
|:---|:---|:---|
| `shared_ptr` 替代 GC | 模块互引、闭包自引用等场景产生引用环，内存永久泄漏 | ❌ 不可行 |
| `shared_ptr` + `weak_ptr` | Python 语义中无法区分"拥有"和"引用"，编译器无法自动判断 | ❌ 不可行 |
| `shared_ptr` + Cycle Collector | 本质上重新引入了 GC，违背"去 GC"目标 | ❌ 违背愿景 |
| **Arena/Region-based** | **无引用计数、无环问题、零 GC 开销、与 C/C++ 程序行为一致** | ✅ 采用 |

### 3.2 核心机制：Arena/Region-based 内存管理

所有 Python 对象在 C++ Runtime 中通过 **裸指针 `PyObject*`** 引用，由 **Arena 分配器** 管理生命周期。

**关键原则**：
*   对象的生命周期由其所属的 Arena 决定，而非引用关系。
*   Arena 销毁时批量释放所有对象，无需逐个析构（除非对象持有外部资源）。
*   不存在引用计数，因此不存在循环引用问题。

#### Arena 层次结构

```
ProgramArena (全局，程序生命周期)
├── TypeArena
│   └── 所有 PyType 对象 (int, str, list, 用户自定义类...)
│
├── ModuleArena[N] (每个模块一个)
│   ├── 模块级全局变量
│   ├── 模块级函数对象
│   ├── 模块级常量
│   └── 模块 __dict__
│
└── ThreadArena[M] (每个线程一个，线程局部)
    └── ScopedRegion (每次函数调用一个，bump allocator)
        ├── 函数局部变量
        └── 临时对象
```

#### 对象生命周期分类

| 类别 | 分配位置 | 释放时机 | 示例 |
|:---|:---|:---|:---|
| **Immortal** | ProgramArena | 程序退出 | PyType, builtin 函数, None/True/False |
| **Module-scoped** | ModuleArena | 程序退出（模块不卸载） | 模块全局变量, 模块级函数 |
| **Function-scoped** | ScopedRegion 或栈 | 函数返回 | 局部变量, 临时计算结果 |
| **Escaped** | 提升到父级 Arena | 随父级 Arena 释放 | 从函数返回的对象 |

#### 示例代码对比

**旧代码 (GC + Slab Style):**
```cpp
// 需要 Heap 分配，GC 追踪
auto* list = heap.allocate<PyList>();
list->append(value); // value 必须在 GC 根集中
// 需要实现 visit_graph() 供 GC 遍历
```

**新代码 (Arena Style):**
```cpp
// 从当前 Arena 分配，裸指针，无引用计数
PyObject* runtime_add(PyObject* a, PyObject* b) {
    auto& arena = Arena::current();  // thread_local
    int64_t result = a->as<PyInteger>()->value() + b->as<PyInteger>()->value();
    return arena.allocate<PyInteger>(result);
}
```

### 3.3 Arena 分配器实现

```cpp
class Arena {
    struct Block {
        uint8_t* memory;
        size_t capacity;
        size_t offset;  // bump pointer
    };
    
    std::vector<Block> m_blocks;
    
public:
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        void* mem = bump_allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }
    
    // Bump allocation — O(1)，无锁（线程局部 Arena）
    void* bump_allocate(size_t size, size_t align);
    
    // 批量释放 — 调用所有对象的析构函数，然后释放内存块
    void reset();
    ~Arena();
};
```

**线程局部访问**：
```cpp
// 每个线程持有自己的 Arena，分配无需加锁
thread_local Arena* t_current_arena = nullptr;

struct ScopedRegion {
    Arena region;
    Arena* previous;
    
    ScopedRegion() : previous(t_current_arena) {
        t_current_arena = &region;
    }
    ~ScopedRegion() {
        t_current_arena = previous;
        // region 析构时自动释放所有对象
    }
};
```

### 3.4 逃逸分析 (Escape Analysis)

编译器负责判断对象的生命周期类别：

```python
def process():
    temp = [1, 2, 3]    # 不逃逸 → ScopedRegion
    result = sum(temp)   # 不逃逸 → 栈分配或 ScopedRegion
    return result         # 逃逸 → 提升到 ModuleArena
```

编译器生成的 IR：
```
; temp 分配在 ScopedRegion (函数返回时自动释放)
%temp = call %PyList* @arena_scoped_alloc_PyList()

; result 逃逸，分配在 ModuleArena
%result = call %PyInteger* @arena_module_alloc_PyInteger(i64 %sum_val)
ret %PyObject* %result
```

**保守策略**：逃逸分析不确定时，默认分配到 ModuleArena（安全但可能浪费内存）。这是可接受的，因为：
1. AOT 编译的程序行为类似 C/C++ 程序
2. 模块级内存在程序退出时统一释放
3. 未来可通过更精确的逃逸分析逐步优化

### 3.5 与现有代码的关系

**关键发现**：现有代码已经使用裸指针 `PyObject*`，`shared_ptr` 迁移尚未发生。因此 Arena 方案的改动量远小于原计划：

| 组件 | 现状 | Arena 方案改动 |
|:---|:---|:---|
| `PyObject*` 裸指针 | ✅ 已在使用 | **保持不变** |
| `Value` 类型 | 包含 `PyObject*` | **保持不变** |
| `Slab` 分配器 | 按大小分桶 | **替换为 Arena bump allocator** |
| `GarbageCollected` header | 2-bit 颜色标记 | **移除** |
| `Cell::visit_graph()` | GC 遍历接口 | **移除** |
| `MarkSweepGC` | 标记-清除 GC | **移除** |
| `Heap` | 管理 Slab + GC | **替换为 Arena 管理器** |

### 3.6 无 GIL 的线程安全

Arena 方案天然支持无 GIL 并行：

| 操作 | 线程安全机制 |
|:---|:---|
| 对象分配 | ThreadArena 是 thread_local，无需加锁 |
| 模块全局变量读取 | Inline Cache + 原子版本号（见 §4） |
| 模块全局变量写入 | `std::atomic` 版本号 + 必要时加细粒度锁 |
| 模块初始化 | ModuleRegistry 的 `condition_variable`（见 Multi-file-compilation.md） |
| 跨线程对象传递 | 对象在 ModuleArena/ProgramArena 中分配（长生命周期），裸指针传递安全 |

## 4. 关键模块重构清单

### 4.1 目录结构调整
```
pylang/
├── src/
│   ├── compiler/          # (New) 编译器前端
│   │   ├── codegen/       # AST -> LLVM IR 生成器
│   │   ├── analysis/      # 逃逸分析、类型推导
│   │   └── driver/        # main 入口，调用 LLVM 工具链
│   ├── runtime/           # (Refactor) 纯净的 Runtime 库
│   │   ├── core/          # PyObject (无 GC header)
│   │   ├── types/         # 使用裸指针的容器
│   │   ├── builtins/      # print, len 等内建函数
│   │   └── modules/       # 标准库模块 (sys, math, etc.)
│   ├── memory/            # (Refactor) Arena 分配器
│   │   ├── Arena.hpp      # 核心 Arena 实现
│   │   ├── ArenaManager.hpp # 全局 Arena 层次管理
│   │   └── ScopedRegion.hpp # 函数级作用域 Region
│   └── parser/            # (Keep) 现有的 Parser 实现
├── third_party/
│   └── llvm/              # LLVM 依赖
```

### 4.2 API 导出与 ABI

由于使用裸指针 `PyObject*`，ABI 大幅简化：

```cpp
// Runtime 代码示例
#define PYLANG_EXPORT __attribute__((annotate("pylang_runtime_export")))

PYLANG_EXPORT
PyObject* runtime_add(PyObject* a, PyObject* b) {
    return a->add(b);  // 内部从 Arena::current() 分配结果
}
```

**对比 shared_ptr 方案的优势**：
*   函数签名是简单的指针传递，LLVM IR 中无需处理复杂的 `%class.std::shared_ptr` 结构体。
*   函数调用无引用计数的原子操作开销。
*   LLVM 可以更激进地内联和优化。

前端 Codegen 阶段：
```cpp
// 伪代码 — 比 shared_ptr 方案简单得多
Function* add_func = module->getFunctionWithAttribute("pylang_runtime_export", "runtime_add");
// 参数就是简单的 PyObject* 指针
builder.CreateCall(add_func, {val_a, val_b});
```

## 5. 实施步骤 (Roadmap)

*详细的执行步骤请参见 `step.md`*

### 第一阶段：内存子系统替换 (Arena Migration)
**策略**: 增量式替换，确保每一步测试通过。

1.  **Arena 基础设施**: 实现 `Arena`, `ArenaManager`, `ScopedRegion`。
2.  **移除 GC 依赖**:
    *   从 `PyBaseObject` 移除 `GarbageCollected` 基类。
    *   从所有 `PyObject` 子类移除 `visit_graph()` 实现。
    *   将 `Cell` 基类简化为不含 GC 接口的纯虚基类。
3.  **替换分配路径**: 将 `heap.allocate<T>(...)` 替换为 `arena.allocate<T>(...)`。
4.  **测试驱动**: 利用现有测试用例，修改一个模块即修复对应测试。

### 第二阶段：编译器原型 (Compiler Prototype)
1.  **AST Lowering**: 创建 `src/compiler/codegen`，实现 `AST -> LLVM IR`。
2.  **Hello World**: 
    *   在 Runtime 中实现 `builtin_print`。
    *   生成 `main` 函数，构造字符串对象，调用 `print`。
3.  **逃逸分析**: 实现基础的逃逸分析 Pass，区分 ScopedRegion 和 ModuleArena 分配。

### 第三阶段：功能补全
1.  **控制流**: `if`, `while`, `for` 等。
2.  **类与函数**: Python 函数调用约定（参数传递即裸指针拷贝，零开销）。
3.  **多文件编译**: Import 系统、ModuleRegistry、Name Mangling。

## 6. 与原项目的兼容性说明
*   **Parser**: 100% 复用。
*   **裸指针 `PyObject*`**: 现有代码已在使用，**无需改动引用方式**。
*   **算法逻辑**: `src/runtime/types` 中的 Python 语义逻辑直接复用，仅替换分配器调用。
*   **测试资产**: 完整保留 `integration/` 和 `src/*/tests` 下的测试用例。
*   **移除项**: `GarbageCollected`, `Cell::visit_graph()`, `MarkSweepGC`, `Heap` (Slab)。