#pragma once

#include "Modules.hpp"
#include "runtime/ModuleRegistry.hpp"
#include <array>

namespace py {
static constexpr std::array builtin_modules{
	std::tuple<std::string_view, PyModule *(*)()>{ "builtins", builtins_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "sys", sys_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_codecs", codecs_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_imp", imp_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_io", io_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "math", math_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "marshal", marshal_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "posix", posix_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_thread", thread_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_weakref", weakref_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_warnings", warnings_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "itertools", itertools_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_sre", sre_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_collections", collections_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "time", time_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_signal", signal_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "errno", errno_module },
	std::tuple<std::string_view, PyModule *(*)()>{ "_struct", struct_module },
};

inline bool is_builtin(std::string_view name)
{
	for (const auto &[module_name, _] : builtin_modules) {
		if (name == module_name) { return true; }
	}
	return false;
}

/// 查找 builtin 模块的初始化函数（不执行）
inline PyModule *(*lookup_builtin_init(std::string_view name))()
{
	for (const auto &[module_name, init_func] : builtin_modules) {
		if (name == module_name) { return init_func; }
	}
	return nullptr;
}

/// 将所有 builtin 模块注册到 ModuleRegistry
inline void register_all_builtins()
{
	auto &reg = ModuleRegistry::instance();
	for (const auto &[name, init_func] : builtin_modules) {
		if (!init_func) continue;
		if (reg.find(std::string(name))) continue;
		auto *mod = init_func();
		if (mod) { reg.register_module(std::string(name), mod); }
	}
}
}// namespace py
