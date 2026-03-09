#include "rt_common.hpp"

#include "runtime/ModuleRegistry.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyString.hpp"

// =============================================================================
// Tier 0: 属性访问
// =============================================================================

PYLANG_EXPORT_ATTR("getattr", "obj", "obj,str")
py::PyObject *rt_getattr(py::PyObject *obj, const char *name)
{
	auto *attr_name = rt_unwrap(py::PyString::create(std::string(name)));
	return rt_unwrap(obj->getattribute(attr_name));
}

PYLANG_EXPORT_ATTR("load_global", "obj", "obj,str")
py::PyObject *rt_load_global(py::PyObject *module, const char *name)
{
	// 快速路径：用 C 字符串直接查找，避免 PyString 分配
	auto *mod = static_cast<py::PyModule *>(module);
	auto result = mod->find_symbol_cstr(name);
	if (result.is_ok()) { return result.unwrap(); }

	// 退避到 builtins 模块查找 (标准 Python 语义)
	auto &reg = py::ModuleRegistry::instance();
	if (auto *builtins_mod = reg.find("builtins")) {
		auto builtin_result = builtins_mod->find_symbol_cstr(name);
		if (builtin_result.is_ok()) { return builtin_result.unwrap(); }
	}

	rt_raise(py::name_error("name '{}' is not defined", name));
}

// =============================================================================
// Tier 1: 存储操作
// =============================================================================

PYLANG_EXPORT_ATTR("store_global", "void", "obj,str,obj")
void rt_store_global(py::PyObject *module, const char *name, py::PyObject *value)
{
	auto *attr_name = rt_unwrap(py::PyString::create(std::string(name)));
	rt_unwrap_void(module->setattribute(attr_name, value));
}

// =============================================================================
// Tier 5: setattr / delattr
// =============================================================================

PYLANG_EXPORT_ATTR("setattr", "void", "obj,str,obj")
void rt_setattr(py::PyObject *obj, const char *name, py::PyObject *value)
{
	auto *attr_name = rt_unwrap(py::PyString::create(std::string(name)));
	rt_unwrap_void(obj->setattribute(attr_name, value));
}

PYLANG_EXPORT_ATTR("delattr", "void", "obj,str")
void rt_delattr(py::PyObject *obj, const char *name)
{
	auto *attr_name = rt_unwrap(py::PyString::create(std::string(name)));
	rt_unwrap_void(obj->delattribute(attr_name));
}