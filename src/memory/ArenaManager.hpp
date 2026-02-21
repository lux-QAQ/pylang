#pragma once

// =============================================================================
// ArenaManager — 全局 Arena 层次管理
// =============================================================================
//
// 管理 Pylang 的三级 Arena 层次:
//   ProgramArena  → Immortal 对象 (PyType, None, True, False, builtin 函数)
//   ModuleArena   → 每个模块一个, 模块级全局变量/函数/常量
//   ThreadArena   → thread_local, 函数级临时对象
//
// =============================================================================

#include "Arena.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace py {

class ArenaManager
{
public:
	/// 初始化全局 Arena 基础设施
	/// 必须在程序启动时 (main 入口处) 调用
    static void initialize()
    {
        auto &inst = instance();
        std::lock_guard lock(inst.m_mutex);

        // 每次 initialize 都重新创建（支持 shutdown 后再次 initialize）
        if (inst.m_initialized) {
            // 已初始化: 确保 current 有效即可
            if (!Arena::has_current() && inst.m_program_arena) {
                Arena::set_current(inst.m_program_arena.get());
            }
            return;
        }

        inst.m_program_arena = std::make_unique<Arena>(Arena::kDefaultBlockSize);
        Arena::set_current(inst.m_program_arena.get());
        inst.m_initialized = true;
    }

	/// 关闭全局 Arena, 释放所有内存
	/// 在程序退出时调用 (或 main 返回前)
	static void shutdown()
	{
		auto &inst = instance();
		std::lock_guard lock(inst.m_mutex);

		// 先释放模块 Arena (模块可能引用 ProgramArena 中的 PyType)
		inst.m_module_arenas.clear();

		// 最后释放 ProgramArena
		Arena::set_current(nullptr);
		inst.m_program_arena.reset();

		inst.m_initialized = false;
	}

	/// 全局 ProgramArena — Immortal 对象
	static Arena &program_arena()
	{
		auto &inst = instance();
		assert(inst.m_program_arena && "ArenaManager not initialized");
		return *inst.m_program_arena;
	}

	/// Per-module Arena — 按模块名获取或创建
	/// 线程安全: 内部加锁
	static Arena &module_arena(const std::string &module_name)
	{
		auto &inst = instance();
		std::lock_guard lock(inst.m_mutex);

		auto it = inst.m_module_arenas.find(module_name);
		if (it != inst.m_module_arenas.end()) { return *it->second; }

		auto arena = std::make_unique<Arena>(Arena::kDefaultBlockSize);
		auto *raw = arena.get();
		inst.m_module_arenas[module_name] = std::move(arena);
		return *raw;
	}

	/// Per-thread Arena — thread_local, 无需加锁
	/// 如果当前线程还没有 ThreadArena, 创建一个
	static Arena &thread_arena()
	{
		if (!t_thread_arena) {
			t_thread_arena = std::make_unique<Arena>(Arena::kDefaultBlockSize);
			// 如果当前线程没有活跃 Arena, 将 ThreadArena 设为默认
			if (!Arena::has_current()) { Arena::set_current(t_thread_arena.get()); }
		}
		return *t_thread_arena;
	}

	/// 检查是否已初始化
	static bool is_initialized()
	{
		auto &inst = instance();
		return inst.m_initialized;
	}

private:
	static ArenaManager &instance()
	{
		static ArenaManager mgr;
		return mgr;
	}

	ArenaManager() = default;

	std::mutex m_mutex;
	bool m_initialized{ false };
	std::unique_ptr<Arena> m_program_arena;
	std::unordered_map<std::string, std::unique_ptr<Arena>> m_module_arenas;

	static thread_local std::unique_ptr<Arena> t_thread_arena;
};

}// namespace py
