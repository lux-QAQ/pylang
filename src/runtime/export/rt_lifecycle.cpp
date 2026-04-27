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

#include <spdlog/cfg/env.h>

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
static void pylang_gmp_free([[maybe_unused]] void *ptr, [[maybe_unused]] size_t size) {}
}

// =============================================================================
// [核心] 将所有内置 PyType 单例钉为 GC 永久根
// =============================================================================
// 安全性论证:
//   叶子对象 (PyInteger, PyFloat, PyString 等) 使用 GC_MALLOC_ATOMIC 分配后,
//   GC 不会扫描其内部的 m_type 指针。如果 PyType 单例只被 ATOMIC 对象引用,
//   GC 会认为它不可达并回收之 → 段错误。
//
//   通过 GC_add_roots 将所有 PyType* 注册为永久根, 使其永远不被回收。
//   必须在 initialize_types() 之后调用, 因为类型单例在那里才被创建。
// =============================================================================
static void pylang_pin_builtin_type_roots()
{
	// 使用 GC_MALLOC_UNCOLLECTABLE 分配的数组本身也不会被回收
	// 且其内部的指针会被 GC 当作根来扫描
	static constexpr size_t MAX_TYPES = 128;
	static py::PyType **pinned =
		static_cast<py::PyType **>(GC_MALLOC_UNCOLLECTABLE(sizeof(py::PyType *) * MAX_TYPES));
	size_t n = 0;

	auto pin = [&](py::PyType *t) {
		if (t && n < MAX_TYPES) { pinned[n++] = t; }
	};

	// ── 核心类型 ──
	pin(py::types::object());
	pin(py::types::type());
	pin(py::types::super());

	// ── 标量/单例 ──
	pin(py::types::bool_());
	pin(py::types::none());
	pin(py::types::ellipsis());
	pin(py::types::not_implemented());

	// ── 数值 ──
	pin(py::types::integer());
	pin(py::types::float_());

	// ── 字符串/字节 ──
	pin(py::types::str());
	pin(py::types::str_iterator());
	pin(py::types::bytes());
	pin(py::types::bytes_iterator());
	pin(py::types::bytearray());
	pin(py::types::bytearray_iterator());

	// ── 容器 ──
	pin(py::types::list());
	pin(py::types::list_iterator());
	pin(py::types::list_reverseiterator());
	pin(py::types::tuple());
	pin(py::types::tuple_iterator());
	pin(py::types::dict());
	pin(py::types::dict_items());
	pin(py::types::dict_items_iterator());
	pin(py::types::dict_keys());
	pin(py::types::dict_key_iterator());
	pin(py::types::dict_values());
	pin(py::types::dict_value_iterator());
	pin(py::types::set());
	pin(py::types::frozenset());
	pin(py::types::set_iterator());
	pin(py::types::mappingproxy());

	// ── 迭代器/生成器 ──
	pin(py::types::range());
	pin(py::types::range_iterator());
	pin(py::types::reversed());
	pin(py::types::zip());
	pin(py::types::map());
	pin(py::types::enumerate());
	pin(py::types::iterator());
	pin(py::types::generator());
	pin(py::types::coroutine());
	pin(py::types::async_generator());

	// ── 函数/方法 ──
	pin(py::types::function());
	pin(py::types::native_function());
	pin(py::types::llvm_function());
	pin(py::types::code());
	pin(py::types::cell());
	pin(py::types::builtin_method());
	pin(py::types::slot_wrapper());
	pin(py::types::bound_method());
	pin(py::types::method_wrapper());
	pin(py::types::classmethod_descriptor());
	pin(py::types::getset_descriptor());
	pin(py::types::static_method());
	pin(py::types::property());
	pin(py::types::classmethod());
	pin(py::types::member_descriptor());

	// ── 杂项 ──
	pin(py::types::slice());
	pin(py::types::traceback());
	pin(py::types::frame());
	pin(py::types::namespace_());
	pin(py::types::generic_alias());
	pin(py::types::memoryview());
	pin(py::types::module());

	// ── 异常体系 ──
	pin(py::types::base_exception());
	pin(py::types::exception());
	pin(py::types::type_error());
	pin(py::types::assertion_error());
	pin(py::types::attribute_error());
	pin(py::types::value_error());
	pin(py::types::name_error());
	pin(py::types::runtime_error());
	pin(py::types::import_error());
	pin(py::types::key_error());
	pin(py::types::not_implemented_error());
	pin(py::types::module_not_found_error());
	pin(py::types::os_error());
	pin(py::types::lookup_error());
	pin(py::types::index_error());
	pin(py::types::warning());
	pin(py::types::import_warning());
	pin(py::types::syntax_error());
	pin(py::types::memory_error());
	pin(py::types::stop_iteration());
	pin(py::types::unbound_local_error());

	// GC_MALLOC_UNCOLLECTABLE 分配的内存自动被 GC 视为根并扫描其内容,
	// 所以 pinned[0..n-1] 中的每个 PyType* 都会被 GC 当作活跃引用,
	// 无需额外调用 GC_add_roots。
	(void)n;// suppress unused warning in release builds
}
#endif

// =============================================================================
// Tier 0: 运行时初始化 / 销毁
// =============================================================================

PYLANG_EXPORT_LIFECYCLE("init", "void", "")
void rt_init()
{
	spdlog::cfg::load_env_levels();
#ifdef PYLANG_USE_Boehm_GC
	GC_INIT();
	// 提高初始堆大小，减少 GC 触发频率
	// GC_expand_hp(256 * 1024 * 1024 * 7);// 256MB

	// [修复] 将 divisor 从极端的
	// 1（引发无限GC的根源）改回默认机制或更大的放宽比例。数值越大约不容易触发GC。
	GC_set_free_space_divisor(1);

	GC_allow_register_threads();
	GC_set_warn_proc(pylang_gc_warn_proc);
	GC_set_finalize_on_demand(0);

	// 预膨胀 Finalizer Table
	// const int K_DUMMY_ENTRIES = 512 * 1024 * 10;
	// void **dummy = (void **)GC_MALLOC(sizeof(void *) * K_DUMMY_ENTRIES);
	// for (int i = 0; i < K_DUMMY_ENTRIES; ++i) {
	// 	dummy[i] = GC_MALLOC(8);
	// 	GC_register_finalizer_no_order(dummy[i], [](void *, void *) {}, nullptr, nullptr, nullptr);
	// }
	// for (int i = 0; i < K_DUMMY_ENTRIES; ++i) {
	// 	GC_register_finalizer_no_order(dummy[i], nullptr, nullptr, nullptr, nullptr);
	// }

	mp_set_memory_functions(pylang_gmp_alloc, pylang_gmp_realloc, pylang_gmp_free);
#endif

#ifdef PYLANG_USE_ARENA
	py::ArenaManager::initialize();
#endif

	if (!py::RuntimeContext::has_current()) {
		static thread_local py::RuntimeContext s_compiler_ctx;
		py::RuntimeContext::set_current(&s_compiler_ctx);
	}

	py::initialize_types();
	py::register_all_builtins();

#ifdef PYLANG_USE_Boehm_GC
	// [关键] 必须在 initialize_types() 之后调用！
	// 此时所有 PyType 单例已创建，可以安全钉根。
	pylang_pin_builtin_type_roots();
#endif
}

PYLANG_EXPORT_LIFECYCLE("shutdown", "void", "")
void rt_shutdown()
{
	// Phase 2: 空实现
	// Phase 3+: Arena 清理、模块卸载等
}