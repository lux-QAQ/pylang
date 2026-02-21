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