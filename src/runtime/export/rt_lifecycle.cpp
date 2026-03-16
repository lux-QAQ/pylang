#include "rt_common.hpp"

#include "runtime/BaseException.hpp"
#include "runtime/Import.hpp"
#include "runtime/KeyError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/NotImplemented.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyCode.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyEllipsis.hpp"
#include "runtime/PyFrame.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyNumber.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyType.hpp"
#include "runtime/Value.hpp"
#include "runtime/builtinTypeInit.hpp"
#include "runtime/modules/Modules.hpp"
#include "runtime/modules/config.hpp"
#include "runtime/types/builtin.hpp"

#include "runtime/RuntimeContext.hpp"
#include "runtime/modules/config.hpp"

#ifdef PYLANG_USE_ARENA
#include "memory/ArenaManager.hpp"
#endif

#ifdef PYLANG_USE_Boehm_GC
#include <gc.h>
#endif

// =============================================================================
// Tier 0: 运行时初始化 / 销毁
// =============================================================================

PYLANG_EXPORT_LIFECYCLE("init", "void", "")
void rt_init()
{
#ifdef PYLANG_USE_Boehm_GC
	GC_INIT();
	GC_allow_register_threads();
#endif

#ifdef PYLANG_USE_ARENA
	py::ArenaManager::initialize();
#endif

	// // 1. 为 AOT 编译器路径提供兜底的 RuntimeContext
	// if (!py::RuntimeContext::has_current()) {
	// 	static py::RuntimeContext s_compiler_ctx;
	// 	py::RuntimeContext::set_current(&s_compiler_ctx);
	// }
	// AOT 模式下提供最小化 RuntimeContext
	// 使用 thread_local 而非 static，避免跨 Arena shutdown 边界的析构问题
	if (!py::RuntimeContext::has_current()) {
		static thread_local py::RuntimeContext s_compiler_ctx;
		py::RuntimeContext::set_current(&s_compiler_ctx);
	}

	// 2. 调用统一的 Runtime 层初始化
	py::initialize_types();

	// 3. 注册所有内置 C++ 模块 (builtins, sys, os 等)
	py::register_all_builtins();
}

PYLANG_EXPORT_LIFECYCLE("shutdown", "void", "")
void rt_shutdown()
{
	// Phase 2: 空实现
	// Phase 3+: Arena 清理、模块卸载等
}