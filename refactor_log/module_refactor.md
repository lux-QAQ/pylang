# Module 系统重构分析

## 一、当前耦合关系图

```
VirtualMachine::the()
├── .interpreter().modules()          ← Import.cpp (4处), PyModule.cpp (1处)
├── .interpreter().importlib()        ← Import.cpp (3处)
├── .interpreter().importfunc()       ← Import.cpp (2处)
├── .interpreter().builtins()         ← Import.cpp (1处)
├── .interpreter().execution_frame()  ← BuiltinsModule.cpp, SysModule.cpp
├── .interpreter().codec_*()          ← CodecsModule.cpp (4处)
├── .interpreter() [equals/truthy]    ← Deque.cpp (2处)
└── .stack_local()                    ← RuntimeContext.cpp
```

所有 modules 下的模块都通过 `VirtualMachine::the()` 间接访问 `Interpreter`，这是最大的耦合点。

## 二、新建文件清单

### 2.1 `src/runtime/ModuleRegistry.hpp` + `.cpp`（核心，第一优先级）

````cpp
#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <memory>

namespace py {

class PyModule;

/// ModuleRegistry — 全局模块注册表
///
/// 解决三个问题:
/// 1. 循环依赖: Check-Insert-Execute 三阶段守卫
/// 2. 并发安全: condition_variable 阻塞等待
/// 3. 递归 import: 同线程返回部分初始化模块
class ModuleRegistry
{
  public:
    static ModuleRegistry &instance();

    struct GetResult
    {
        PyModule *module;
        bool is_owner;// true → 调用者负责初始化
    };

    /// 获取或注册模块
    GetResult get_or_register(const std::string &name);

    /// 标记模块初始化完成
    void mark_initialized(const std::string &name);

    /// 标记模块初始化失败
    void mark_failed(const std::string &name);

    /// 查找已注册模块（不创建）
    PyModule *find(const std::string &name) const;

    /// 直接注册一个已初始化的模块（用于旧路径兼容）
    void register_module(const std::string &name, PyModule *mod);

    /// 模块是否已初始化
    bool is_initialized(const std::string &name) const;

    /// 清空（测试用）
    void clear();

  private:
    struct Entry
    {
        PyModule *module{ nullptr };
        bool initialized{ false };
        bool failed{ false };
        std::thread::id owner_thread{};
        std::condition_variable cv;

        Entry() = default;
        Entry(const Entry &) = delete;
        Entry &operator=(const Entry &) = delete;
    };

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::unique_ptr<Entry>> m_modules;

    ModuleRegistry() = default;
};

/// RAII 初始化守卫 — 异常/提前返回时自动 mark_failed
struct InitGuard
{
    std::string name;
    bool committed{ false };

    explicit InitGuard(const std::string &n) : name(n) {}
    ~InitGuard()
    {
        if (!committed) { ModuleRegistry::instance().mark_failed(name); }
    }
    void commit()
    {
        committed = true;
        ModuleRegistry::instance().mark_initialized(name);
    }
};

}// namespace py
````

### 2.2 `src/runtime/ModuleMangler.hpp`（第二优先级）

````cpp
#pragma once

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace py {

class ModuleMangler
{
  public:
    /// 模块逻辑名 → 链接器符号名
    /// "foo.bar" → "PyInit_foo_2E_bar"
    static std::string mangle(const std::string &module_name)
    {
        std::ostringstream os;
        os << "PyInit_";
        for (char c : module_name) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                os << c;
            } else {
                os << '_' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(static_cast<unsigned char>(c)) << '_';
            }
        }
        return os.str();
    }

    /// 文件路径 → 模块逻辑名
    /// "foo/bar/baz.py" (相对于 root) → "foo.bar.baz"
    static std::string path_to_module_name(const std::string &file_path,
        const std::string &root_dir);
};

}// namespace py
````

### 2.3 `src/runtime/KeyVersionTracker.hpp`（第三优先级，为 inline cache 准备）

````cpp
#pragma once

#include <atomic>
#include <string>
#include <unordered_map>

namespace py {

class KeyVersionTracker
{
  public:
    uint64_t get(const std::string &key) const
    {
        auto it = m_versions.find(key);
        return it != m_versions.end() ? it->second.load(std::memory_order_acquire) : 0;
    }

    void bump(const std::string &key)
    {
        m_versions[key].fetch_add(1, std::memory_order_release);
    }

  private:
    std::unordered_map<std::string, std::atomic<uint64_t>> m_versions;
};

}// namespace py
````

### 2.4 `src/runtime/DynamicImport.hpp` + `.cpp`（第四优先级，编译器需要时再实现）

暂时只创建头文件骨架，内部实现推迟。

## 三、需要修改的现有文件

### 3.1 修改优先级排序

| 优先级 | 文件 | 修改内容 | 风险 |
|:---:|:---|:---|:---:|
| P0 | Import.cpp | `import_get_module()` 先查 Registry，miss 降级旧路径 | 低 |
| P0 | config.hpp | 在 `initialize_builtin_modules()` 时注册到 Registry | 低 |
| P1 | Modules.hpp | 统一工厂函数签名，移除 `Interpreter&` 参数 | 中 |
| P1 | CodecsModule.cpp | `VirtualMachine::the().interpreter()` → `RuntimeContext::current().interpreter()` | 中 |
| P1 | Deque.cpp | `equals`/`truthy` → `RuntimeContext` | 中 |
| P1 | PyModule.cpp | `__repr__` 中的 VM 依赖 → RuntimeContext | 中 |
| P2 | SysModule.cpp | 工厂函数签名 `sys_module(Interpreter&)` → 无参数 + RuntimeContext | 中 |
| P2 | BuiltinsModule.cpp | 同上 | 高 |
| P2 | PyArgParser.hpp | 已部分迁移，清理残余旧代码 | 低 |
| P3 | `PyModule.hpp` | 增加 `dict_version` + `KeyVersionTracker` | 低 |
| P3 | CMakeLists.txt | 添加 `ModuleRegistry.cpp`, `ModuleMangler.cpp` | 低 |

### 3.2 核心修改详解

#### Import.cpp — 双路径渐进式迁移

```
import_get_module(name)
  ├── [新路径] ModuleRegistry::instance().find(name)
  │     └── 找到 → 返回
  └── [旧路径] VirtualMachine::the().interpreter().modules()
        └── 找到 → 注册到 Registry + 返回
```

#### config.hpp — 保留静态表，增加注册桥接

```
builtin_modules[] — 保持不变（编译期常量）

新增:
void register_all_builtins() {
    for (auto& [name, init_fn] : builtin_modules) {
        if (init_fn) {
            auto* mod = init_fn();
            ModuleRegistry::instance().register_module(name, mod);
        }
    }
}
```

#### 模块工厂函数 — 统一模式

当前模式（以 `warnings_module` 为例）：
```cpp
PyModule *warnings_module() {
    auto *mod = PyModule::create(...).unwrap();
    mod->add_symbol(...);
    return mod;  // 裸返回，无注册
}
```

目标模式：
```cpp
PyModule *warnings_module() {
    auto& reg = ModuleRegistry::instance();
    if (auto* existing = reg.find("_warnings")) return existing;

    auto *mod = PyModule::create(...).unwrap();
    mod->add_symbol(...);
    reg.register_module("_warnings", mod);
    return mod;
}
```

### 3.3 VirtualMachine 解耦路径

各模块对 `VirtualMachine::the()` 的依赖可分为三类：

| 依赖类型 | 调用点 | 替代方案 |
|:---|:---|:---|
| `interpreter().modules()` | Import.cpp | → `ModuleRegistry::instance()` |
| `interpreter().importlib()` | Import.cpp | → `RuntimeContext::current().interpreter()->importlib()` |
| `interpreter().execution_frame()` | BuiltinsModule, SysModule | → `RuntimeContext::current().interpreter()->execution_frame()` |
| `interpreter() [equals/truthy]` | Deque.cpp, PyArgParser | → `RuntimeContext::current().is_true()` / `.equals()` |
| `interpreter().codec_*()` | CodecsModule | → `RuntimeContext::current().interpreter()->codec_*()` |
| `heap()` | 已通过 `PYLANG_ALLOC` 解耦 | ✅ 已完成 |

## 四、推荐实施顺序

```
Phase C-1: ModuleRegistry 基础设施 (不破坏现有功能)
  ├── 新建 ModuleRegistry.hpp/cpp
  ├── 新建 ModuleMangler.hpp
  ├── CMakeLists.txt 添加新文件
  ├── git commit
  └── 构建验证

Phase C-2: Import.cpp 双路径 (向后兼容)
  ├── import_get_module() 增加 Registry 查询
  ├── 模块工厂函数增加 Registry 注册
  ├── git commit
  └── 全量测试

Phase C-3: 模块去 VM 耦合 (逐文件)
  ├── CodecsModule → RuntimeContext
  ├── Deque → RuntimeContext
  ├── PyModule.__repr__ → RuntimeContext
  ├── 每个文件单独 commit + 测试
  └── SysModule/BuiltinsModule (最后，风险最高)

Phase C-4: PyModule 增强
  ├── KeyVersionTracker
  ├── dict_version
  └── 为 inline cache 准备接口
```

**建议从 Phase C-1 开始**：创建 `ModuleRegistry.hpp/cpp` + `ModuleMangler.hpp`，注册到 CMake，确保编译通过。这是零风险的纯增量操作。要开始吗？