#include "Import.hpp"
#include "ImportError.hpp"
#include "PyBool.hpp"
#include "PyCode.hpp"
#include "PyDict.hpp"
#include "PyFrame.hpp"
#include "PyModule.hpp"
#include "PyNone.hpp"
#include "PyType.hpp"
#include "TypeError.hpp"
#include "ValueError.hpp"

#ifndef PYLANG_AOT_MODE
#include "executable/bytecode/BytecodeProgram.hpp"
#endif

#include "frozen/importlib.h"
#include "frozen/importlib_external.h"
#include "interpreter/InterpreterCore.hpp"
#include "modules/config.hpp"
#include "runtime/KeyError.hpp"
#include "runtime/ModuleRegistry.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/Value.hpp"
#include "runtime/compat.hpp"


namespace py {



	
namespace {

#ifndef PYLANG_AOT_MODE
	// ---- 辅助: 获取当前 Interpreter ----
	Interpreter &current_interpreter()
	{
		ASSERT(RuntimeContext::has_current() && RuntimeContext::current().has_interpreter());
		return *RuntimeContext::current().interpreter();
	}
#endif

	// adapted from CPython import.c resolve_name
	PyResult<PyString *> resolve_name(PyString *name, PyDict *globals, uint32_t level)
	{
		if (!globals) { return Err(value_error("'__name__' not in globals")); }

		if (!globals->map().contains(String{ "__package__" })) {
			return Err(value_error("'__package__' not in globals"));
		}
		if (!globals->map().contains(String{ "__spec__" })) {
			return Err(value_error("'__spec__' not in globals"));
		}
		auto package_ = PyObject::from(globals->map().at(String{ "__package__" }));
		ASSERT(package_.is_ok());
		auto *package = package_.unwrap();
		auto spec_ = PyObject::from(globals->map().at(String{ "__spec__" }));
		ASSERT(spec_.is_ok());
		auto *spec = spec_.unwrap();

		if (package != py_none()) {
			if (!as<PyString>(package)) {
				return Err(type_error("package must be a string"));
			} else if (spec != py_none()) {
				auto parent = spec->get_attribute(PyString::create("parent").unwrap());
				if (parent.is_err()) return Err(parent.unwrap_err());
				auto are_equal = package->eq(parent.unwrap());
				if (are_equal.is_err()) return Err(are_equal.unwrap_err());
				if (are_equal.unwrap() == py_false()) {
					// TODO: emit import warning
				}
			}
		} else if (spec != py_none()) {
			package_ = spec->get_attribute(PyString::create("parent").unwrap());
			if (package_.is_err()) return Err(package_.unwrap_err());
			if (!as<PyString>(package_.unwrap())) {
				return Err(type_error("__spec__.parent must be a string"));
			}
			package = package_.unwrap();
		} else {
			if (!globals->map().contains(String{ "__name__" })) {
				return Err(value_error("'__name__' not in globals"));
			}
			package_ = PyObject::from(globals->map().at(String{ "__name__" }));
			if (package_.is_err()) return Err(package_.unwrap_err());
			if (!as<PyString>(package)) { return Err(type_error("__name__ must be a string")); }
			package = package_.unwrap();

			const bool haspath = globals->map().contains(String{ "__path__" });

			if (!haspath) {
				const auto &package_str = as<PyString>(package)->value();
				auto pos = package_str.find_last_of('.');
				if (pos == std::string::npos) {
					return Err(
						import_error("attempted relative import with no known parent package"));
				}
				package_ = PyString::create(package_str.substr(pos));
				if (package_.is_err()) return Err(package_.unwrap_err());
				package = package_.unwrap();
			}
		}

		ASSERT(as<PyString>(package));

		size_t last_dot = as<PyString>(package)->size();

		if (last_dot == 0) {
			return Err(import_error("attempted relative import with no known parent package"));
		}

		const auto &package_str = as<PyString>(package)->value();

		for (size_t level_up = 1; level_up < level; level_up++) {
			const auto pos = package_str.find_last_of('.', last_dot);
			if (pos == std::string::npos) {
				return Err(import_error("attempted relative import beyond top-level package"));
			}
			last_dot = pos;
		}

		auto base_ = PyString::create(package_str.substr(0, last_dot));
		if (base_.is_err()) return base_;
		auto *base = base_.unwrap();
		if (base->size() == 0) { return Ok(base); }

		if (name->to_string().empty()) { return PyString::create(base); }

		auto abs_name_str = fmt::format("{}.{}", base->to_string(), name->to_string());
		auto abs_name = PyString::create(abs_name_str);

		return abs_name;
	}

	// ========================================================================
	// import_get_module — 双路径查找 (RuntimeContext 版)
	// ========================================================================
    std::optional<PyModule *> import_get_module(PyString *name)
    {
        const auto &name_str = name->value();

        // 新路径: 先查 ModuleRegistry
        if (auto *mod = ModuleRegistry::instance().find(name_str)) { return mod; }
#ifndef PYLANG_AOT_MODE
        // 旧路径: 从 Interpreter.modules() 查找
        if (RuntimeContext::has_current() && RuntimeContext::current().has_interpreter()) {
            auto *available_modules = current_interpreter().modules();
            if (available_modules) {
                if (auto it = available_modules->map().find(name);
                    it != available_modules->map().end()) {
                    auto *mod = as<PyModule>(PyObject::from(it->second).unwrap());
                    if (mod) { ModuleRegistry::instance().register_module(name_str, mod); }
                    return mod;
                }
            }
        }
#endif

        return {};
    }



    void remove_module(PyString *name)
    {
#ifndef PYLANG_AOT_MODE
        if (RuntimeContext::has_current() && RuntimeContext::current().has_interpreter()) {
            auto *available_modules = current_interpreter().modules();
            if (available_modules) { available_modules->remove(name); }
        }
#endif
    }

	PyResult<std::monostate> import_ensure_initialized(PyModule *module, PyString *name)
	{
		auto spec_str = PyString::create("__spec__").unwrap();
		if (module->symbol_table()->map().contains(spec_str)) {
			auto spec = PyObject::from(module->symbol_table()->map().at(spec_str));
			if (spec.is_err()) return Err(spec.unwrap_err());

			auto value = spec.unwrap()->get_attribute(PyString::create("_initializing").unwrap());
			if (value.is_err()) return Ok(std::monostate{});

			bool initializing = false;
			if (RuntimeContext::has_current()) {
				initializing = RuntimeContext::current().is_true(value.unwrap());
			} else {
				auto result = value.unwrap()->true_();
				if (result.is_err()) return Err(result.unwrap_err());
				initializing = result.unwrap();
			}

			if (initializing) {
#ifdef PYLANG_AOT_MODE
				return Err(
					import_error("concurrent import initializing state not supported in AOT mode"));
#else
				auto _lock_unlock_module = PyString::create("_lock_unlock_module");
				if (_lock_unlock_module.is_err()) return Err(_lock_unlock_module.unwrap_err());
				auto args = PyTuple::create(name);
				if (args.is_err()) return Err(args.unwrap_err());
				auto *importlib = current_interpreter().importlib();
				if (!importlib) { return Err(import_error("importlib not imported")); }
				value = importlib->get_method(_lock_unlock_module.unwrap())
							.and_then([args](PyObject *lock_unlock_module) {
								return lock_unlock_module->call(args.unwrap(), nullptr);
							});
				if (value.is_err()) return Err(value.unwrap_err());
				return Ok(std::monostate{});
#endif
			}
		}
		return Ok(std::monostate{});
	}

	PyResult<PyModule *> import_find_and_load(PyString *name)
	{
#ifdef PYLANG_AOT_MODE
		return Err(import_error(
			"No module named '{}' (dynamic import_find_and_load not supported in AOT mode)",
			name->value()));
#else
		auto find_and_load = PyString::create("_find_and_load");
		if (find_and_load.is_err()) return Err(find_and_load.unwrap_err());
		auto *importlib = current_interpreter().importlib();
		if (!importlib) { return Err(import_error("importlib not imported")); }
		auto import_find_and_load = importlib->get_method(find_and_load.unwrap());
		if (import_find_and_load.is_err()) return Err(import_find_and_load.unwrap_err());

		auto args = PyTuple::create(name, current_interpreter().importfunc());
		if (args.is_err()) return Err(args.unwrap_err());

		auto module = import_find_and_load.unwrap()->call(args.unwrap(), nullptr);
		if (module.is_err()) { return Err(module.unwrap_err()); }
		if (!as<PyModule>(module.unwrap())) {
			return Err(type_error("expected module to be of type module"));
		}
		return Ok(as<PyModule>(module.unwrap()));
#endif
	}

	std::array frozen_modules = {
		FrozenModule{ .name = "_frozen_importlib", .code = _bootstrap, .is_package = false },
		FrozenModule{ .name = "_frozen_importlib_external",
			.code = _bootstrap_external,
			.is_package = false },
	};

}// namespace

std::optional<std::reference_wrapper<const FrozenModule>> find_frozen(PyString *name)
{
	for (const auto &m : frozen_modules) {
		if (m.name == name->value()) { return m; }
	}
	return {};
}

// adapted from CPython import.c PyImport_ImportModuleLevelObject
PyResult<PyObject *> import_module_level_object(PyString *name,
	PyDict *globals,
	PyObject *locals,
	PyObject *fromlist,
	uint32_t level)
{
	(void)locals;
	(void)fromlist;
	if (!name) { return Err(value_error("Empty module name")); }

	auto absolute_name = [&]() -> PyResult<PyString *> {
		if (level > 0) {
			return resolve_name(name, globals, level);
		} else {
			if (name->size() == 0) { return Err(value_error("Empty module name")); }
			return Ok(name);
		}
	}();

	if (absolute_name.is_err()) return absolute_name;

	auto module = [&]() -> PyResult<PyModule *> {
		auto module = import_get_module(absolute_name.unwrap());
		if (module.has_value()) {
			auto is_initialized = import_ensure_initialized(*module, absolute_name.unwrap());
			if (is_initialized.is_err()) { return Err(is_initialized.unwrap_err()); }
			return Ok(*module);
		}

		return import_find_and_load(absolute_name.unwrap());
	}();

	if (module.is_err()) { return module; }

	const auto has_from = [fromlist]() -> PyResult<bool> {
		if (!fromlist) { return Ok(false); }
		if (RuntimeContext::has_current()) {
			return Ok(RuntimeContext::current().is_true(fromlist));
		}
		return fromlist->true_();
	}();

	if (has_from.is_err()) { return Err(has_from.unwrap_err()); }

	if (!has_from.unwrap()) {
		if (name->value().empty()) { return module; }
		auto it = name->value().find('.');
		if (it == std::string::npos) { return module; }
		if (level == 0) {
			return import_module_level_object(
				PyString::create(name->value().substr(0, it)).unwrap(),
				nullptr,
				nullptr,
				nullptr,
				0);
		}
		auto to_return = absolute_name.unwrap()->value().substr(0, name->value().size() - it);
		auto final_mod = import_get_module(PyString::create(to_return).unwrap());
		if (!final_mod.has_value()) {
			return Err(key_error("{} not in sys.modules as expected", to_return));
		}
		return Ok(*final_mod);
	} else {
		auto path = module.unwrap()->symbol_table()->map().find(String{ "__path__" });
		if (path != module.unwrap()->symbol_table()->map().end()) {
#ifdef PYLANG_AOT_MODE
			return module;
#else
			auto &interp = current_interpreter();
			return interp.importlib()
				->get_method(PyString::create("_handle_fromlist").unwrap())
				.and_then([&interp, module, fromlist](auto *_handle_fromlist) {
					return _handle_fromlist->call(
						PyTuple::create(module.unwrap(), fromlist, interp.importfunc()).unwrap(),
						nullptr);
				});
#endif
		} else {
			return module;
		}
	}

	return module;
}

// adapted from CPython import.c PyImport_ImportFrozenModuleObject
PyResult<PyModule *> import_frozen_module(PyString *name)
{
#ifdef PYLANG_AOT_MODE
	(void)name;
	return Err(import_error("frozen module import not available in AOT mode"));
#else
	const auto frozen_module = find_frozen(name);
	if (!frozen_module.has_value()) { return Err(import_error("")); }

	const auto &serialised_code = frozen_module->get().code;
	std::shared_ptr<Program> program = BytecodeProgram::deserialize(serialised_code);

	PYLANG_GC_PAUSE_SCOPE();

	if (!program) { return Err(import_error("TODO")); }

	if (frozen_module->get().is_package) { TODO(); }

	auto module = import_add_module(name);
	if (module.is_err()) return module;
	module.unwrap()->set_program(program);

	auto &interp = current_interpreter();

	module.unwrap()->symbol_table()->insert(String{ "__builtins__" }, interp.builtins());

	auto result = [&interp, &program, module]() {
		auto *code = as<PyCode>(static_cast<BytecodeProgram &>(*program).main_function());
		ASSERT(code);
		auto *function_frame = PyFrame::create(interp.execution_frame(),
			code->register_count(),
			0,
			code,
			module.unwrap()->symbol_table(),
			module.unwrap()->symbol_table(),
			code->consts(),
			code->names(),
			nullptr);
		[[maybe_unused]] auto scoped_stack =
			interp.setup_call_stack(code->function(), function_frame);
		return interp.call(code->function(), function_frame);
	}();

	if (result.is_err()) {
		remove_module(name);
		spdlog::error("{}", result.unwrap_err()->to_string());
		TODO();
		return Err(import_error("TODO"));
	}

	module.unwrap()->set_program(std::move(program));

	return module;
#endif
}

// ========================================================================
// create_builtin — Registry 查找
// ========================================================================
PyResult<PyObject *> create_builtin(PyObject *spec)
{
	PyObject *mod{ nullptr };

	auto name = spec->get_attribute(PyString::create("name").unwrap());
	if (name.is_err()) return name;

	auto namestr = as<PyString>(name.unwrap());
	if (!namestr) {
		return Err(type_error(
			"expected spec name to str, but got {}", name.unwrap()->type()->to_string()));
	}

	// 新路径: 先查 Registry
	if (auto *cached = ModuleRegistry::instance().find(namestr->value())) {
		return Ok(static_cast<PyObject *>(cached));
	}

	// 旧路径: 遍历 builtin_modules 表
	for (const auto &[bname, init_func] : builtin_modules) {
		if (bname == namestr->value()) {
			if (!init_func) { return import_add_module(namestr); }
			mod = init_func();
			if (!mod) { TODO(); }
			ModuleRegistry::instance().register_module(std::string(bname), as<PyModule>(mod));
			return Ok(mod);
		}
	}

	return Ok(py_none());
}

	PyResult<PyModule *> import_add_module(PyString *name)
	{
		auto m = import_get_module(name);
		if (m.has_value()) return Ok(*m);
		auto mod = PyModule::create(PyDict::create().unwrap(), name, PyString::create("").unwrap());
		if (mod.is_ok()) {
			ModuleRegistry::instance().register_module(name->value(), mod.unwrap());
		}
		return mod;
	}

}// namespace py