#include "rt_common.hpp"

#include "runtime/ModuleRegistry.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyString.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

// =============================================================================
// Tier 0: 属性访问
// =============================================================================

PYLANG_EXPORT_ATTR("getattr", "obj", "obj,str")
py::PyObject *rt_getattr(py::PyObject *obj, const char *name)
{
	// ✅ intern：同一个属性名永远复用同一个 PyString，零分配
	return rt_unwrap(obj->getattribute(py::PyString::intern(name)));
}

PYLANG_EXPORT_ATTR("load_global", "obj", "obj,str")
py::PyObject *rt_load_global(py::PyObject *module, const char *name)
{
	auto *mod = static_cast<py::PyModule *>(py::ensure_box(module));
	auto result = mod->find_symbol_cstr(name);
	if (result.is_ok()) { return result.unwrap(); }

	auto &reg = py::ModuleRegistry::instance();
	if (auto *builtins_mod = reg.find("builtins")) {
		auto builtin_result = builtins_mod->find_symbol_cstr(name);
		if (builtin_result.is_ok()) { return builtin_result.unwrap(); }
	}

	rt_raise(py::name_error("name '{}' is not defined", name));
	return nullptr;
}

// =============================================================================
// Tier 1: 存储操作
// =============================================================================

PYLANG_EXPORT_ATTR("store_global", "void", "obj,str,obj")
void rt_store_global(py::PyObject *module, const char *name, py::PyObject *value)
{
	rt_unwrap_void(
		py::ensure_box(module)->setattribute(py::PyString::intern(name), py::ensure_box(value)));
}

// =============================================================================
// Tier 5: setattr / delattr
// =============================================================================

PYLANG_EXPORT_ATTR("setattr", "void", "obj,str,obj")
void rt_setattr(py::PyObject *obj, const char *name, py::PyObject *value)
{
	rt_unwrap_void(
		py::ensure_box(obj)->setattribute(py::PyString::intern(name), py::ensure_box(value)));
}

PYLANG_EXPORT_ATTR("delattr", "void", "obj,str")
void rt_delattr(py::PyObject *obj, const char *name)
{
	rt_unwrap_void(py::ensure_box(obj)->delattribute(py::PyString::intern(name)));
}