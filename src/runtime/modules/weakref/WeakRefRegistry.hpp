#pragma once

// =============================================================================
// WeakRefRegistry — Arena 模式下的弱引用支持
// =============================================================================
//
// Arena 模型中对象不被单独释放，因此 weakref 的"目标已死"语义
// 需要通过显式 tombstone 标记实现。
//
// 调用 invalidate(obj) 将对象标记为"逻辑死亡"，
// 之后所有引用它的 weakref 的 is_alive() 返回 false。
//
// 谁调用 invalidate:
//   - del 语句 → 编译器生成的 IR
//   - ScopedRegion 析构 → Arena::reset() 回调
//   - 程序退出 → ArenaManager::shutdown()
//
// =============================================================================

#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace py {

class PyObject;

namespace weakref {

class WeakRefRegistry
{
public:
	static WeakRefRegistry &instance()
	{
		static WeakRefRegistry reg;
		return reg;
	}

	/// 注册一个弱引用关系: ref_obj 弱引用 target
	void register_ref(PyObject *target, PyObject *ref_obj)
	{
		std::lock_guard lock(m_mutex);
		m_refs[target].insert(ref_obj);
		m_alive.insert(target);
	}

	/// 标记 target 为"逻辑死亡"
	void invalidate(PyObject *target)
	{
		std::lock_guard lock(m_mutex);
		m_alive.erase(target);
	}

	/// 批量标记为"逻辑死亡" (供 Arena::reset 使用)
	void invalidate_batch(const std::vector<PyObject *> &targets)
	{
		std::lock_guard lock(m_mutex);
		for (auto *t : targets) { m_alive.erase(t); }
	}

	/// 检查 target 是否仍然"逻辑存活"
	bool is_alive(PyObject *target) const
	{
		std::lock_guard lock(m_mutex);
		return m_alive.contains(target);
	}

	/// 获取 target 的弱引用计数
	size_t ref_count(PyObject *target) const
	{
		std::lock_guard lock(m_mutex);
		auto it = m_refs.find(target);
		if (it == m_refs.end()) return 0;
		return it->second.size();
	}

	/// 获取 target 的所有弱引用对象
	std::vector<PyObject *> get_refs(PyObject *target) const
	{
		std::lock_guard lock(m_mutex);
		auto it = m_refs.find(target);
		if (it == m_refs.end()) return {};
		return { it->second.begin(), it->second.end() };
	}

	/// 完全移除一个 target 的所有记录 (target 内存被释放时调用)
	void remove_target(PyObject *target)
	{
		std::lock_guard lock(m_mutex);
		m_alive.erase(target);
		m_refs.erase(target);
	}

	/// 清空所有记录 (程序退出时调用)
	void clear()
	{
		std::lock_guard lock(m_mutex);
		m_alive.clear();
		m_refs.clear();
	}

private:
	mutable std::mutex m_mutex;
	std::unordered_map<PyObject *, std::unordered_set<PyObject *>> m_refs;
	std::unordered_set<PyObject *> m_alive;

	WeakRefRegistry() = default;
};

// ---- compat.hpp 宏的后端函数 ----

inline bool is_alive(PyObject *obj) { return WeakRefRegistry::instance().is_alive(obj); }

inline size_t ref_count(PyObject *obj) { return WeakRefRegistry::instance().ref_count(obj); }

inline std::vector<PyObject *> get_refs(PyObject *obj)
{
	return WeakRefRegistry::instance().get_refs(obj);
}

}// namespace weakref
}// namespace py
