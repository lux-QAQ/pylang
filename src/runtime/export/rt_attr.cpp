#include "rt_common.hpp"

#include "runtime/AttributeCache.hpp"
#include "runtime/ModuleRegistry.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyString.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"


// =============================================================================
// Tier 0: 属性访问
// =============================================================================

// PYLANG_EXPORT_ATTR("getattr", "obj", "obj,str")
// py::PyObject *rt_getattr(py::PyObject *obj, const char *name)
// {
// 	// intern：同一个属性名永远复用同一个 PyString，零分配
// 	return rt_unwrap(obj->getattribute(py::PyString::intern(name)));
// }

PYLANG_EXPORT_ATTR("getattr", "obj", "obj,str")
py::PyObject *rt_getattr(py::PyObject *obj, const char *name)
{
	auto *b_obj = py::ensure_box(obj);
	auto *type = b_obj->type();
	uint64_t gv = py::PyType::global_version();

	// 1. 快速路径：哈希表命中 (包含 PYLANG_ATTR_ABSENT)
	py::PyObject *cached = py::AttrCache::lookup(type, name, gv);
	if (cached) {
		if (cached == PYLANG_ATTR_ABSENT) goto try_instance_dict;
		return rt_unwrap(cached->get(b_obj, type));
	}

	// 2. 慢速路径：安静地搜索 MRO (lookup 不分配异常对象)
	{
		auto *interned_name = py::PyString::intern(name);
		auto res_opt = type->lookup(interned_name);

		if (res_opt.has_value()) {
			auto res = res_opt.value();
			if (res.is_ok()) {
				auto *attr = res.unwrap();
				py::AttrCache::update(type, name, attr, gv);
				return rt_unwrap(attr->get(b_obj, type));
			}
		}
	}

	// 3. 记录负缓存：标记该类 MRO 确定无此属性
	py::AttrCache::update(type, name, PYLANG_ATTR_ABSENT, gv);

try_instance_dict:
	// 4. [极致优化] 直接查询实例属性，消灭 99% 的 getattribute 异常分配
	if (auto *dict = b_obj->attributes()) {
		// 使用 interned string 作为 key 直接在 unordered_map 中查找
		auto it = dict->map().find(py::Value(py::PyString::intern(name)));
		if (it != dict->map().end()) {
			return py::RtValue::from_value(it->second).as_pyobject_raw();
		}
	}

	// 5. 慢速路径：真正抛出异常 (只有这一步会分配 AttributeError)
	return rt_unwrap(b_obj->getattribute(py::PyString::intern(name)));
}

// PYLANG_EXPORT_ATTR("load_global", "obj", "obj,str")
// py::PyObject *rt_load_global(py::PyObject *module, const char *name)
// {
// 	auto *mod = static_cast<py::PyModule *>(py::ensure_box(module));
// 	auto result = mod->find_symbol_cstr(name);
// 	if (result.is_ok()) { return result.unwrap(); }

// 	auto &reg = py::ModuleRegistry::instance();
// 	if (auto *builtins_mod = reg.find("builtins")) {
// 		auto builtin_result = builtins_mod->find_symbol_cstr(name);
// 		if (builtin_result.is_ok()) { return builtin_result.unwrap(); }
// 	}

// 	rt_raise(py::name_error("name '{}' is not defined", name));
// 	return nullptr;
// }

PYLANG_EXPORT_ATTR("load_global", "obj", "obj,str")
py::PyObject *rt_load_global(py::PyObject *module, const char *name)
{
	auto *mod = static_cast<py::PyModule *>(py::ensure_box(module));
	auto *globals_dict = mod->dict();
	auto *builtins_mod = py::ModuleRegistry::instance().find("builtins");
	auto *builtins_dict = builtins_mod ? builtins_mod->dict() : nullptr;

	// 1. 快速路径：语义安全缓存
	if (auto *cached = py::GlobalCache::lookup(mod, name, globals_dict, builtins_dict)) {
		return cached;
	}

	// 2. 慢速路径：标准查找
    py::PyObject *found = nullptr;
    auto result = mod->find_symbol_cstr(name);
    if (result.is_ok()) {
        found = result.unwrap();
    } else {
        auto &reg = py::ModuleRegistry::instance();
        if (auto *builtins_mod = reg.find("builtins")) {
            auto builtin_result = builtins_mod->find_symbol_cstr(name);
            if (builtin_result.is_ok()) found = builtin_result.unwrap();
        }
    }

    if (found) {
        py::GlobalCache::update(mod, name, found, globals_dict->version(), builtins_dict ? builtins_dict->version() : 0);
        return found;
    }

    // 只有最后确定找不到，才分配一次异常
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