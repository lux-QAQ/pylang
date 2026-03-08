#include "rt_common.hpp"

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
	auto *attr_name = rt_unwrap(py::PyString::create(std::string(name)));
	return rt_unwrap(module->getattribute(attr_name));
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