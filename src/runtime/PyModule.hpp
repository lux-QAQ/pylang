#pragma once

#include "KeyVersionTracker.hpp"
#include "PyObject.hpp"
#include "PyString.hpp"

#include <atomic>
#include <cstdint>

namespace py {

class PyModule : public PyBaseObject
{
  public:
	PyString *m_module_name{ nullptr };
	PyObject *m_doc{ nullptr };
	PyObject *m_package{ nullptr };
	PyObject *m_loader{ nullptr };
	PyObject *m_spec{ nullptr };
	PyDict *m_dict{ nullptr };

  private:
	PyObject *m_module_context{ nullptr };

	/// 全局 dict 版本号 — 任意属性变更时递增
	/// 用于粗粒度 inline cache 失效检测
	uint64_t m_dict_version{ 0 };

	/// Per-key 版本号 — 精细粒度 cache 失效
	/// 仅 key X 被修改时，X 的 version 才递增
	KeyVersionTracker m_key_versions;

	/// 全局版本计数器（所有模块共享，单调递增）
	static std::atomic<uint64_t> s_global_version;

#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	friend class VM;

	std::shared_ptr<Program> m_program;

	PyModule(PyType *);

  public:
	PyModule(PyDict *symbol_table, PyString *module_name, PyObject *doc);

	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);
	PyResult<int32_t> __init__(PyTuple *args, PyDict *kwargs);
	static PyResult<PyModule *> create(PyDict *symbol_table, PyString *module_name, PyObject *doc);

	void visit_graph(Visitor &visitor) override;

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __getattribute__(PyObject *attribute) const;

	std::string to_string() const override;

	PyDict *dict() const { return m_dict; }
	PyDict *symbol_table() const { return m_dict; }

	/// 添加/更新符号 — 同时递增 dict_version 和 key_version
	void add_symbol(PyString *key, const Value &value);

	PyString *name() const { return m_module_name; }

	// ---- Inline Cache 支持 ----

	/// 全局 dict 版本（粗粒度）
	uint64_t dict_version() const { return m_dict_version; }

	/// 单个 key 的版本号（细粒度，避免缓存震荡）
	uint64_t get_key_version(const std::string &key) const { return m_key_versions.get(key); }

	/// 手动使所有 cache 失效
	void invalidate_cache() { m_dict_version = s_global_version.fetch_add(1); }

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	PyType *static_type() const override;

	void set_program(std::shared_ptr<Program> program);
	const std::shared_ptr<Program> &program() const;

	void set_context(PyObject *obj) { m_module_context = obj; }
	PyObject *get_context() const { return m_module_context; }

	/// 直接用 C 字符串查找符号 — 避免 PyString 分配开销
	/// 用于 AOT 编译器的快速路径
	PyResult<PyObject *> find_symbol_cstr(const char *name) const;
};

}// namespace py
