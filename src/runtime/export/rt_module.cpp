#include "rt_common.hpp"

#include "runtime/ImportError.hpp"
#include "runtime/PyString.hpp"
#include "utilities.hpp"

#include <cstdio>
#include <cstdlib>

// =============================================================================
// Tier 0: 模块操作
// =============================================================================

PYLANG_EXPORT_MODULE("import", "obj", "str,str,i32")
py::PyObject *rt_import(const char *module_name, const char * /*from_list*/, int32_t /*level*/)
{
	// Phase 2: 简化实现 —— 打印错误后 abort
	// 完整的 import 机制需要 ModuleRegistry 实例，
	// 在 Phase 2.5 中通过 rt_init 注册全局实例后再完善
	std::fprintf(stderr, "rt_import: import not yet implemented (module: %s)\n", module_name);
	TODO()  ;
}