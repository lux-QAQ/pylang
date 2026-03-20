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
#include <gmp.h>
// 声明在 Boehm_GCSetup.cpp 中定义的警告过滤函数
extern void pylang_gc_warn_proc(char *msg, GC_word arg);

extern "C" {
// 1. 无指针节点全部使用 ATOMIC 免标记分配，断崖式降低 `GC_mark_from` 时间
static void *pylang_gmp_alloc(size_t alloc_size) { return GC_MALLOC_ATOMIC(alloc_size); }
static void *pylang_gmp_realloc(void *ptr, [[maybe_unused]] size_t old_size, size_t new_size)
{
	return GC_REALLOC(ptr, new_size);
}
// 无需干预释放，让 GC 自动完成
static void pylang_gmp_free([[maybe_unused]]void *ptr, [[maybe_unused]] size_t size) {}
}
#endif

// =============================================================================
// Tier 0: 运行时初始化 / 销毁
// =============================================================================

PYLANG_EXPORT_LIFECYCLE("init", "void", "")
void rt_init()
{
#ifdef PYLANG_USE_Boehm_GC
	GC_INIT();
	// 提高初始堆大小，减少 GC 触发频率
	GC_expand_hp(256 * 1024 * 1024 * 7);// 256MB

	// [修复] 将 divisor 从极端的
	// 1（引发无限GC的根源）改回默认机制或更大的放宽比例。数值越大约不容易触发GC。
	GC_set_free_space_divisor(20000);

	// 关闭并行标记（当前 steal_mark_stack 开销太大）
	// GC_set_markers_count(2);

	// GC_enable_incremental();
	GC_allow_register_threads();
	GC_set_warn_proc(pylang_gc_warn_proc);
	GC_set_finalize_on_demand(0);

	// [新增] 预膨胀 Finalizer Table
	// 我们一次性申请并注销50w个对象，从而将终结对象表在系统启动初直接拔到极高上限，
	// 此后在跑纯数学运算时绝对不会出现 "Grew fo table" 而强制挂起进程。
	const int K_DUMMY_ENTRIES = 512 * 1024 * 10;
	void **dummy = (void **)GC_MALLOC(sizeof(void *) * K_DUMMY_ENTRIES);
	for (int i = 0; i < K_DUMMY_ENTRIES; ++i) {
		dummy[i] = GC_MALLOC(8);
		GC_register_finalizer_no_order(dummy[i], [](void *, void *) {}, nullptr, nullptr, nullptr);
	}
	for (int i = 0; i < K_DUMMY_ENTRIES; ++i) {
		GC_register_finalizer_no_order(dummy[i], nullptr, nullptr, nullptr, nullptr);
	}

	mp_set_memory_functions(pylang_gmp_alloc, pylang_gmp_realloc, pylang_gmp_free);
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

	py::initialize_types();
	py::register_all_builtins();
}

PYLANG_EXPORT_LIFECYCLE("shutdown", "void", "")
void rt_shutdown()
{
	// Phase 2: 空实现
	// Phase 3+: Arena 清理、模块卸载等
}