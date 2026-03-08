#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/types/builtin.hpp"

#ifdef PYLANG_USE_ARENA
#include "memory/ArenaManager.hpp"
#endif

// =============================================================================
// Tier 0: 运行时初始化 / 销毁
// =============================================================================

PYLANG_EXPORT_LIFECYCLE("init", "void", "")
void rt_init()
{
#ifdef PYLANG_USE_ARENA
	// 初始化 Arena 内存管理器
	py::ArenaManager::initialize();
#endif

	// 初始化所有内置类型
	// BuiltinTypes::the() 是懒加载的，但 types::xxx() 函数会触发
	// PyType::initialize()，需要确保全部完成
	py::types::object();
	py::types::type();
	py::types::none();
	py::types::bool_();
	py::types::integer();
	py::types::float_();
	py::types::str();
	py::types::bytes();
	py::types::tuple();
	py::types::list();
	py::types::dict();
	py::types::set();
	py::types::frozenset();
	py::types::function();
	py::types::native_function();
	py::types::code();
	py::types::cell();
	py::types::module();
	py::types::slice();
	py::types::range();
	py::types::iterator();
	py::types::builtin_method();
	py::types::bound_method();
	py::types::slot_wrapper();
	py::types::method_wrapper();
	py::types::property();
	py::types::static_method();
	py::types::classmethod();
	py::types::getset_descriptor();
	py::types::member_descriptor();
	py::types::traceback();
	py::types::frame();
	py::types::not_implemented();
	py::types::super();
	py::types::generator();

	// 异常类型
	py::types::base_exception();
	py::types::exception();
	py::types::type_error();
	py::types::value_error();
	py::types::name_error();
	py::types::attribute_error();
	py::types::runtime_error();
	py::types::import_error();
	py::types::key_error();
	py::types::index_error();
	py::types::stop_iteration();
	py::types::assertion_error();
	py::types::memory_error();
	py::types::os_error();
	py::types::not_implemented_error();
	py::types::module_not_found_error();
	py::types::lookup_error();
	py::types::unbound_local_error();
	py::types::syntax_error();

	// 确保单例已创建
	(void)py::py_none();
	(void)py::py_true();
	(void)py::py_false();

	// TODO: 注册内置模块（builtins, sys 等）
	// 这需要与 ModuleRegistry 集成，Phase 2.5 具体实现
}

PYLANG_EXPORT_LIFECYCLE("shutdown", "void", "")
void rt_shutdown()
{
	// Phase 2: 空实现
	// Phase 3+: Arena 清理、模块卸载等
}