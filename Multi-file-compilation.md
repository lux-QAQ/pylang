# Pylang 多文件编译与 Import 系统架构 (Multi-file Compilation & Import Architecture)

本文档详细描述了在 Pylang (AOT 编译器架构) 中如何处理多文件项目、Import 机制以及标准库模块。

**内存模型前提**: 本文档基于 `refactor.md` 确定的 Arena/Region-based 内存管理。所有 `PyObject` 通过裸指针引用，由 Arena 管理生命周期。不使用 `shared_ptr`，不存在循环引用问题。

## 1. 标准库重构 (`src/runtime/modules`)

### 1.1 核心策略：裸指针 ABI + Attribute 导出

由于采用 Arena 内存模型，所有 Runtime 函数的参数和返回值均为 `PyObject*` 裸指针。ABI 极其简洁，同时保留 C++ name mangling 以获得类型安全。

```cpp
// src/runtime/common.hpp
#define PYLANG_EXPORT __attribute__((annotate("pylang_runtime_export")))
#define PYLANG_INIT(name) __attribute__((annotate("pylang_init_func=" name)))
```

**对比旧方案 (`shared_ptr`) 的优势**：

| 维度 | `shared_ptr` ABI | 裸指针 ABI |
|:---|:---|:---|
| 函数签名 | `shared_ptr<PyObject>` (复杂结构体) | `PyObject*` (单个指针) |
| 调用开销 | 原子引用计数 inc/dec | 零开销 |
| LLVM 内联 | 受限 (需穿透 shared_ptr 层) | 激进内联 |
| 循环引用 | 致命缺陷 | 不存在 |





### 1.1.5 统一初始化流程

为了避免解释器路径和 AOT 路径的重复代码，我们将类型初始化和模块注册统一到以下两个函数：

#### `py::initialize_runtime()` — 类型系统初始化

定义在 `src/runtime/types/builtin.cpp`，负责：
- 调用所有 `types::xxx()` 触发类型注册
- 创建全局单例（`py_none()`, `py_true()`, `py_false()` 等）
- 线程安全（内部有 `static bool` 守卫，多次调用无副作用）

```cpp
// src/runtime/types/builtin.hpp
namespace py {
    void initialize_runtime();
}
```

#### `py::register_all_builtins()` — 模块注册

定义在 `src/runtime/modules/config.hpp`，负责：
- 遍历 `builtin_modules` 数组
- 调用每个模块的初始化函数（如 `builtins_module()`, `sys_module()` 等）
- 将模块注册到 `ModuleRegistry`

```cpp
// src/runtime/modules/config.hpp
namespace py {
    void register_all_builtins();
}
```

#### 调用顺序

**解释器路径** (`Interpreter::internal_setup`):
```cpp
py::initialize_runtime();        // 1. 类型系统
py::register_all_builtins();     // 2. 模块注册
// 3. 从 ModuleRegistry 获取 builtins/sys
m_builtins = ModuleRegistry::instance().find("builtins");
m_modules->insert("builtins", m_builtins);  // 4. 同步到 m_modules dict
```

**AOT 路径** (`rt_init`):
```cpp
py::initialize_runtime();        // 1. 类型系统
py::register_all_builtins();     // 2. 模块注册
// 3. 编译器生成的代码通过 rt_load_global 访问 builtins
```

**测试路径** (`main.cpp`):
```cpp
py::initialize_runtime();        // 只需类型系统，模块按需加载
```

#### 关键修正

1. **删除重复代码**：
   - ❌ `Interpreter.cpp` 中的 `void initialize_types()` 函数
   - ❌ `rt_lifecycle.cpp` 中内联的 80+ 行 `types::xxx()` 调用
   - ✅ 统一使用 `py::initialize_runtime()`

2. **统一模块注册**：
   - ❌ `Interpreter.cpp` 中手动遍历 `builtin_modules` 并调用
   - ✅ 统一使用 `py::register_all_builtins()`
   - ✅ 解释器路径额外同步到 `m_modules` dict（兼容旧代码）

3. **ModuleRegistry 的作用**：
   - 解决并发 import 的竞态条件
   - 解决循环依赖（同线程递归 import 返回部分初始化模块）
   - 作为 AOT 和解释器的统一模块存储

#### 为什么解释器路径还需要 `m_modules` dict？

`ModuleRegistry` 是全局单例，而 `Interpreter::m_modules` 是每个解释器实例的私有字典。在多解释器场景下（如嵌入式 Python），每个解释器可能有不同的模块集合。因此：

- **AOT 路径**：只使用 `ModuleRegistry`（单一全局命名空间）
- **解释器路径**：使用 `ModuleRegistry` + `m_modules`（支持多解释器隔离）

未来如果需要支持多解释器，可以将 `ModuleRegistry` 改为 per-interpreter 实例。

### 1.2 改造示例 (SysModule)

```cpp
// src/runtime/modules/SysModule.cpp
#include "runtime/api.hpp"
#include "memory/Arena.hpp"
#include "runtime/ModuleRegistry.hpp"

namespace py {

static PyModule* create_sys_module(Arena& arena) {
    auto* mod = arena.allocate<PyModule>("sys");
    mod->set_attr("path", arena.allocate<PyList>());
    mod->set_attr("modules", arena.allocate<PyDict>());
    mod->set_attr("platform", arena.allocate<PyString>("linux"));
    // ... 其他 sys 模块属性
    return mod;
}

PYLANG_INIT("sys")
PyModule* PyInit_sys() {
    auto& registry = ModuleRegistry::instance();
    auto [mod, is_owner] = registry.get_or_register("sys");

    if (!is_owner) return mod;  // 已初始化或同线程递归 import

    InitGuard guard("sys");  // RAII: 异常时自动 mark_failed

    auto& arena = ArenaManager::module_arena("sys");
    PyModule* result = create_sys_module(arena);

    // 将内容填充到已注册的空模块中
    mod->merge_from(result);

    guard.commit();
    return mod;
}

} // namespace py
```

### 1.3 现有模块改造清单

现有 `src/runtime/modules/` 下的模块需要统一改造：

| 模块文件 | 导出函数 | 分配 Arena |
|:---|:---|:---|
| `BuiltinsModule.cpp` | `PyInit_builtins()` | ProgramArena (Immortal) |
| `SysModule.cpp` | `PyInit_sys()` | ModuleArena("sys") |
| `MarshalModule.cpp` | `PyInit_marshal()` | ModuleArena("marshal") |
| `IOModule.cpp` | `PyInit__io()` | ModuleArena("_io") |
| `CodecsModule.cpp` | `PyInit__codecs()` | ModuleArena("_codecs") |
| `MathModule.cpp` | `PyInit_math()` | ModuleArena("math") |
| `PosixModule.cpp` | `PyInit_posix()` | ModuleArena("posix") |
| `WarningsModule.cpp` | `PyInit__warnings()` | ModuleArena("_warnings") |
| `ImpModule.cpp` | `PyInit__imp()` | ModuleArena("_imp") |
| `collections/module.cpp` | `PyInit_collections()` | ModuleArena("collections") |
| `struct/module.cpp` | `PyInit_struct()` | ModuleArena("struct") |

改造模式统一：
1. 移除对 `Interpreter` / `VirtualMachine` 的依赖
2. 通过 `ModuleRegistry` 注册
3. 从对应 `ModuleArena` 分配对象
4. 添加 `PYLANG_INIT` 属性标记

---

## 2. 模块注册表 (ModuleRegistry)

### 2.1 设计要点

ModuleRegistry 是 Import 系统的核心，解决三个问题：
- **循环依赖**: Check-Insert-Execute 三阶段守卫
- **并发安全**: `condition_variable` 阻塞等待，非 owner 线程不会拿到半成品模块
- **递归 import**: 同线程递归 import 返回部分初始化模块（Python 语义）

```cpp
// src/runtime/ModuleRegistry.hpp
#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace py {

class PyModule;

/// RAII 初始化守卫 — 异常时自动 mark_failed
struct InitGuard {
    std::string name;
    bool committed{false};

    explicit InitGuard(const std::string& n) : name(n) {}
    ~InitGuard() {
        if (!committed)
            ModuleRegistry::instance().mark_failed(name);
    }
    void commit() {
        committed = true;
        ModuleRegistry::instance().mark_initialized(name);
    }
};

class ModuleRegistry {
public:
    static ModuleRegistry& instance() {
        static ModuleRegistry reg;
        return reg;
    }

    struct GetResult {
        PyModule* module;
        bool is_owner;  // true → 调用者负责初始化
    };

    /// 获取或注册模块
    /// - 模块已初始化 → 直接返回, is_owner=false
    /// - 同线程递归 import → 返回部分初始化模块, is_owner=false
    /// - 其他线程正在初始化 → 阻塞等待直到完成
    /// - 全新模块 → 占位注册, is_owner=true
    GetResult get_or_register(const std::string& name);

    void mark_initialized(const std::string& name);
    void mark_failed(const std::string& name);

    PyModule* find(const std::string& name) const;
    bool is_initialized(const std::string& name) const;

private:
    struct Entry {
        PyModule* module{nullptr};
        bool initialized{false};
        bool failed{false};
        std::thread::id owner_thread{};
        std::condition_variable cv;

        Entry() = default;
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
    };

    mutable std::mutex m_mutex;
    // condition_variable 不可移动, 用 unique_ptr 包装
    std::unordered_map<std::string, std::unique_ptr<Entry>> m_modules;

    ModuleRegistry() = default;
};

} // namespace py
```

### 2.2 get_or_register 实现

```cpp
// src/runtime/ModuleRegistry.cpp
ModuleRegistry::GetResult ModuleRegistry::get_or_register(const std::string& name) {
    std::unique_lock lock(m_mutex);

    auto& entry_ptr = m_modules[name];
    if (!entry_ptr) entry_ptr = std::make_unique<Entry>();
    auto& entry = *entry_ptr;

    if (entry.module != nullptr) {
        if (entry.initialized) {
            return {entry.module, false};
        }

        if (entry.owner_thread == std::this_thread::get_id()) {
            // 同线程递归 import — Python 语义: 返回部分初始化模块
            return {entry.module, false};
        }

        // 其他线程正在初始化 → 阻塞等待
        entry.cv.wait(lock, [&entry] {
            return entry.initialized || entry.failed;
        });

        if (entry.failed) {
            m_modules.erase(name);
            return {nullptr, false};  // 调用方应检查 nullptr
        }
        return {entry.module, false};
    }

    // 全新模块 → 从 ModuleArena 分配空模块并占位
    auto& arena = ArenaManager::module_arena(name);
    entry.module = arena.allocate<PyModule>(name);
    entry.owner_thread = std::this_thread::get_id();
    return {entry.module, true};
}
```

### 2.3 循环依赖处理流程

```
线程 1: import a
  ├─ get_or_register("a") → {mod_a, is_owner=true}
  ├─ 执行 a.py 模块体...
  │   ├─ 遇到 import b
  │   │   ├─ get_or_register("b") → {mod_b, is_owner=true}
  │   │   ├─ 执行 b.py 模块体...
  │   │   │   ├─ 遇到 import a
  │   │   │   │   └─ get_or_register("a")
  │   │   │   │       → owner_thread == 当前线程
  │   │   │   │       → 返回 {mod_a, is_owner=false}  ← 部分初始化
  │   │   │   ├─ 使用 mod_a (可能缺少后续定义的属性)
  │   │   │   └─ b 初始化完成
  │   │   └─ mark_initialized("b")
  │   ├─ 继续执行 a.py 剩余代码
  │   └─ a 初始化完成
  └─ mark_initialized("a")

线程 2: import a (并发)
  ├─ get_or_register("a")
  │   → entry.module != nullptr && !initialized
  │   → owner_thread != 当前线程
  │   → cv.wait() ... 阻塞 ...
  │   → 线程 1 调用 mark_initialized("a"), notify_all()
  │   → 返回 {mod_a, is_owner=false}  ← 完全初始化
  └─ 安全使用 mod_a
```

---

## 3. 命名修饰 (Name Mangling)

### 3.1 基于模块逻辑名，而非文件路径

**关键修正** (来自 issue.md #3): Python 的 `import` 语义基于模块命名空间 (dot notation)，不是文件路径。同一个模块 `foo.bar` 无论从哪个路径加载，其符号名必须一致。

**Mangling 输入**: Python 点分模块名 (如 `foo.bar.baz`)
**Mangling 输出**: C 链接器安全的符号名 (如 `PyInit_foo_2E_bar_2E_baz`)

### 3.2 编码规则

1. 字母 (`a-z`, `A-Z`) 和数字 (`0-9`) 保持不变
2. 点号 `.` 替换为 `_2E_` (ASCII 0x2E)
3. 下划线 `_` 替换为 `_5F_` (ASCII 0x5F)
4. 其他非字母数字字符替换为 `_XX_` (XX 为 ASCII 十六进制大写)
5. 统一前缀 `PyInit_`

### 3.3 示例

| Python 模块名 | Mangled 符号名 |
|:---|:---|
| `main` | `PyInit_main` |
| `foo.bar` | `PyInit_foo_2E_bar` |
| `foo.bar.baz` | `PyInit_foo_2E_bar_2E_baz` |
| `foo_bar` | `PyInit_foo_5F_bar` |
| `_codecs` | `PyInit__5F_codecs` |
| `collections` | `PyInit_collections` |

### 3.4 实现

```cpp
// src/runtime/ModuleMangler.hpp
#pragma once
#include <string>
#include <sstream>
#include <iomanip>

namespace py {

class ModuleMangler {
public:
    /// 模块逻辑名 → 链接器符号名
    static std::string mangle(const std::string& module_name) {
        std::ostringstream os;
        os << "PyInit_";
        for (char c : module_name) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                os << c;
            } else {
                os << '_'
                   << std::uppercase << std::hex
                   << std::setw(2) << std::setfill('0')
                   << static_cast<int>(static_cast<unsigned char>(c))
                   << '_';
            }
        }
        return os.str();
    }

    /// 文件路径 → 模块逻辑名
    /// "foo/bar/baz.py" (root=".") → "foo.bar.baz"
    /// "__init__.py" 所在目录作为包名
    static std::string path_to_module_name(
        const std::string& file_path,
        const std::string& root_dir);
};

} // namespace py
```

### 3.5 Driver 中的路径解析流程

```
用户输入: import foo.bar

Driver:
  1. 在 PYTHONPATH 中查找:
     - foo/bar.py          → 普通模块
     - foo/bar/__init__.py → 包
  2. 确定模块逻辑名: "foo.bar"
  3. Mangle: "PyInit_foo_2E_bar"
  4. 检查是否已编译 → 未编译则加入编译队列
```

---

## 4. 全局变量与 Inline Cache

### 4.1 全局变量语义

Python 的全局变量是 Module `__dict__` 的条目，不是 C 的 `.data` 段变量：

```python
# a.py
x = 10          # → mod_a->set_attr("x", PyInteger(10))
print(x)        # → mod_a->get_attr("x")

# b.py
import a
print(a.x)      # → mod_a->get_attr("x")
```

直接编译为 `get_attr`/`set_attr` 会导致每次访问都经历哈希表查找，抹杀 AOT 优势。

### 4.2 Inline Cache 机制

编译器为每个 `LOAD_GLOBAL` 生成带缓存的访问代码：

```
; 编译器生成的 LLVM IR 伪代码

; 每个访问点有一个静态缓存槽
@cache_x = internal global { i64, ptr } zeroinitializer

define ptr @load_global_x(ptr %module) {
entry:
    ; 读取缓存的 key 版本号
    %cached_ver = load atomic i64, ptr getelementptr(@cache_x, 0, 0), acquire
    ; 读取模块中该 key 的当前版本号
    %cur_ver = call i64 @PyModule_get_key_version(ptr %module, ptr @str_x)
    %hit = icmp eq i64 %cached_ver, %cur_ver
    br i1 %hit, label %fast, label %slow

fast:
    %val = load atomic ptr, ptr getelementptr(@cache_x, 0, 1), acquire
    ret ptr %val

slow:
    %val2 = call ptr @runtime_load_global_slow(ptr %module, ptr @str_x, ptr @cache_x)
    ret ptr %val2
}
```

### 4.3 PyModule 版本号支持

```cpp
// src/runtime/PyModule.hpp (新增部分)
class PyModule : public PyBaseObject {
public:
    // ...existing interface...

    /// 原子版本号 — 多线程安全
    uint64_t dict_version() const {
        return m_dict_version.load(std::memory_order_acquire);
    }

    /// Per-key 版本号 — 避免缓存震荡
    uint64_t get_key_version(const std::string& key) const {
        return m_key_versions.get(key);
    }

    void set_attr(const std::string& key, PyObject* value) {
        m_attributes->insert(key, value);
        m_dict_version.fetch_add(1, std::memory_order_release);
        m_key_versions.bump(key);
    }

private:
    std::atomic<uint64_t> m_dict_version{0};
    KeyVersionTracker m_key_versions;
};
```

```cpp
// src/runtime/KeyVersionTracker.hpp
#pragma once
#include <atomic>
#include <string>
#include <unordered_map>

namespace py {

class KeyVersionTracker {
public:
    uint64_t get(const std::string& key) const {
        auto it = m_versions.find(key);
        return it != m_versions.end()
            ? it->second.load(std::memory_order_acquire) : 0;
    }

    void bump(const std::string& key) {
        m_versions[key].fetch_add(1, std::memory_order_release);
    }

private:
    std::unordered_map<std::string, std::atomic<uint64_t>> m_versions;
};

} // namespace py
```

### 4.4 缓存震荡问题的解决

旧方案 (soution.md) 使用单一 `m_dict_version`，任何变量写入都导致全模块缓存失效：

```python
# 热循环中
for i in range(1000000):
    global_counter += 1   # bump m_dict_version
    x = math.pi           # cache miss! (因为 version 变了)
```

新方案使用 per-key 版本号，`global_counter` 的写入只 bump `"global_counter"` 的 key_version，`"pi"` 的缓存不受影响。

### 4.5 静态推导优化

编译器可通过 SSA 分析识别"初始化后不再重绑定"的全局变量：

```python
import math       # math 模块引用在初始化后不变
PI = 3.14159      # 模块级常量

def compute(r):
    return PI * r * r   # PI 可以被静态化
```

对于这类变量，编译器可直接生成静态指针解引用，完全跳过 inline cache：

```
; PI 被证明不可变 → 直接内联为常量指针
%pi = load ptr, ptr @mod_main_PI_static   ; 无需版本号检查
```

---

## 5. 动态 Import 后备机制

### 5.1 问题

AOT 编译器的 Driver 通过静态 AST 扫描收集依赖。但以下场景无法静态分析：

```python
mod = importlib.import_module(user_input)   # 运行时才知道模块名
__import__(name)                             # 同上
```

### 5.2 解决方案

#### A. 编译器选项: `--hidden-import`

```bash
pylangc main.py --hidden-import=plugins.auth --hidden-import=plugins.db
```

指定的模块会被强制编译并链接，即使静态扫描未发现依赖。

#### B. Runtime 动态加载后备

```cpp
// src/runtime/DynamicImport.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace py {

class DynamicImportFallback {
public:
    /// 注册 --hidden-import 指定的模块
    static void register_hidden_import(
        const std::string& module_name,
        const std::string& so_path);

    /// 添加搜索路径
    static void add_search_path(const std::string& path);

    /// 尝试动态加载模块
    /// 1. 查找已注册的 hidden imports
    /// 2. 在搜索路径中查找 .so/.pylang 文件
    /// 3. dlopen + 查找 mangled PyInit_xxx 符号
    static PyModule* try_load(const std::string& name);

private:
    static std::vector<std::string>& search_paths();
    static std::unordered_map<std::string, std::string>& hidden_imports();
};

} // namespace py
```

#### C. Import 调用链

```cpp
// src/runtime/Import.cpp

PyModule* import_module(const std::string& name) {
    // 1. 查 ModuleRegistry (已加载的模块)
    if (auto* mod = ModuleRegistry::instance().find(name)) {
        return mod;
    }

    // 2. 查 builtin 模块表 (config.hpp 中注册的 C++ 模块)
    if (auto* init_fn = lookup_builtin_init(name)) {
        return init_fn();
    }

    // 3. 查编译期静态链接的用户模块 (通过 mangled symbol)
    std::string symbol = ModuleMangler::mangle(name);
    if (auto* init_fn = dlsym(RTLD_DEFAULT, symbol.c_str())) {
        auto fn = reinterpret_cast<PyModule*(*)()>(init_fn);
        return fn();
    }

    // 4. 动态加载后备
    if (auto* mod = DynamicImportFallback::try_load(name)) {
        return mod;
    }

    // 5. 失败
    throw ImportError("No module named '" + name + "'");
}
```

---

## 6. 编译器 Codegen: Init 函数生成

### 6.1 编译器为每个 .py 文件生成的 Init 函数结构

对于 `a.py`:

```python
# a.py
import b
x = 10
def foo():
    return b.y + x
```

编译器生成（C++ 伪代码表示 LLVM IR 逻辑）：

```cpp
// Mangled name: PyInit_a
PYLANG_INIT("a")
PyModule* PyInit_a() {
    // Phase 1: Check & Insert
    auto [mod, is_owner] = ModuleRegistry::instance().get_or_register("a");
    if (!is_owner) return mod;

    // Phase 2: RAII Guard
    InitGuard guard("a");

    // Phase 3: Execute module body
    auto& arena = ArenaManager::module_arena("a");

    // import b → 递归调用 (可能触发 b 的编译/初始化)
    PyModule* b_mod = PyInit_b();
    mod->set_attr("b", b_mod);

    // x = 10
    mod->set_attr("x", arena.allocate<PyInteger>(10));

    // def foo(): ...
    // 函数对象分配在 ModuleArena (模块级生命周期)
    auto* foo_code = arena.allocate<PyFunction>("foo", &compiled_foo);
    mod->set_attr("foo", foo_code);

    // Phase 4: Commit
    guard.commit();
    return mod;
}
```

### 6.2 Main 入口生成

```cpp
// 编译器生成的 main()
int main(int argc, char** argv) {
    // 1. 初始化 Arena 基础设施
    ArenaManager::initialize();

    // 2. 初始化 builtin 模块
    PyInit_builtins();
    PyInit_sys();

    // 3. 调用用户入口模块
    PyInit_main();

    // 4. ArenaManager 析构时批量释放所有 Arena
    ArenaManager::shutdown();
    return 0;
}
```

---

## 7. 编译器 Driver 设计

### 7.1 编译流程

```
用户运行: pylangc main.py -o main

Driver 流程:
  1. [Entry Scan]
     解析 main.py AST, 提取所有 import 语句

  2. [Path Resolution]
     对每个 import, 在 PYTHONPATH 中查找源文件
     确定模块逻辑名 (dot notation)

  3. [Dependency Graph]
     递归扫描所有依赖, 构建 DAG
     检测循环依赖 (标记, 不报错 — Python 允许循环 import)

  4. [Codegen Loop]
     a. 加载 runtime.bc 到 llvm::Module
     b. 按拓扑序 (叶子优先) 逐文件生成 IR
     c. 每个文件生成一个 PyInit_xxx 函数
     d. import 语句 → 生成对 PyInit_xxx 的 Call 指令

  5. [Main Entry]
     生成 main() 函数

  6. [Link & Optimize]
     合并所有 IR Module + runtime.bc
     运行 LLVM Pass (mem2reg, inline, DCE...)
     输出可执行文件
```

### 7.2 并行编译

由于 Arena 是 thread_local 的，编译器 Driver 可以并行编译无依赖关系的模块：

```
Dependency DAG:
    main → a → c
    main → b → c

编译顺序:
    Phase 1: c (叶子节点)
    Phase 2: a, b (并行)
    Phase 3: main
```

---

## 8. 与 `refactor.md` 的对应关系

| refactor.md 章节 | 本文档对应 |
|:---|:---|
| §3.2 Arena 层次结构 | §1.2 模块从 ModuleArena 分配 |
| §3.4 逃逸分析 | §6.1 模块级对象分配在 ModuleArena |
| §3.6 无 GIL 线程安全 | §2 ModuleRegistry 的 condition_variable |
| §4.2 API 导出 | §1.1 PYLANG_EXPORT / PYLANG_INIT |

## 9. 实施步骤

### Phase 1: Runtime 基础改造
- [ ] 实现 `ModuleRegistry` (含 `condition_variable` 并发支持)
- [ ] 实现 `ModuleMangler` (逻辑名 mangling)
- [ ] 实现 `InitGuard` RAII 守卫
- [ ] 改造现有 modules: 移除 VM/Interpreter 依赖, 添加 `PYLANG_INIT` 属性
- [ ] `PyModule` 增加 `std::atomic<uint64_t> m_dict_version` + `KeyVersionTracker`

### Phase 2: 编译器 Codegen
- [ ] 实现 `RuntimeLinker`: 加载 `runtime.bc`, 建立属性索引表
- [ ] `LLVMGenerator` 生成带守卫的 Init 函数
- [ ] 全局变量读写生成 inline cache 代码
- [ ] 生成 `main()` 入口

### Phase 3: Driver
- [ ] 实现 AST Visitor 提取 import 依赖
- [ ] 实现依赖图构建与拓扑排序
- [ ] 实现 `--hidden-import` 选项
- [ ] 实现 `DynamicImportFallback`