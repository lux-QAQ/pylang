#include "ModuleRegistry.hpp"

namespace py {

ModuleRegistry &ModuleRegistry::instance()
{
    static ModuleRegistry reg;
    return reg;
}

ModuleRegistry::GetResult ModuleRegistry::get_or_register(const std::string &name)
{
    std::unique_lock lock(m_mutex);

    auto &entry_ptr = m_modules[name];
    if (!entry_ptr) entry_ptr = std::make_unique<Entry>();
    auto &entry = *entry_ptr;

    if (entry.module != nullptr) {
        if (entry.initialized) {
            return { entry.module, false };
        }

        if (entry.owner_thread == std::this_thread::get_id()) {
            // 同线程递归 import — Python 语义: 返回部分初始化模块
            return { entry.module, false };
        }

        // 其他线程正在初始化 → 阻塞等待
        entry.cv.wait(lock, [&entry] { return entry.initialized || entry.failed; });

        if (entry.failed) {
            m_modules.erase(name);
            return { nullptr, false };
        }
        return { entry.module, false };
    }

    // 全新模块 → 占位（module 指针由调用方设置）
    entry.owner_thread = std::this_thread::get_id();
    return { nullptr, true };
}

void ModuleRegistry::mark_initialized(const std::string &name)
{
    std::lock_guard lock(m_mutex);
    auto it = m_modules.find(name);
    if (it == m_modules.end()) return;
    it->second->initialized = true;
    it->second->cv.notify_all();
}

void ModuleRegistry::mark_failed(const std::string &name)
{
    std::lock_guard lock(m_mutex);
    auto it = m_modules.find(name);
    if (it == m_modules.end()) return;
    it->second->failed = true;
    it->second->cv.notify_all();
}

PyModule *ModuleRegistry::find(const std::string &name) const
{
    std::lock_guard lock(m_mutex);
    auto it = m_modules.find(name);
    if (it == m_modules.end()) return nullptr;
    return it->second->module;
}

void ModuleRegistry::register_module(const std::string &name, PyModule *mod)
{
    std::lock_guard lock(m_mutex);
    auto &entry_ptr = m_modules[name];
    if (!entry_ptr) entry_ptr = std::make_unique<Entry>();
    entry_ptr->module = mod;
    entry_ptr->initialized = true;
    entry_ptr->owner_thread = std::this_thread::get_id();
    entry_ptr->cv.notify_all();
}

bool ModuleRegistry::is_initialized(const std::string &name) const
{
    std::lock_guard lock(m_mutex);
    auto it = m_modules.find(name);
    if (it == m_modules.end()) return false;
    return it->second->initialized;
}

void ModuleRegistry::clear()
{
    std::lock_guard lock(m_mutex);
    m_modules.clear();
}

}// namespace py