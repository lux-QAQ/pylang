#pragma once

// =============================================================================
// Pylang 内存分配兼容层 (Arena Migration)
// =============================================================================
//
// 迁移策略:
//   Phase A (当前): PYLANG_USE_ARENA 未定义
//     → 所有宏展开为旧的 Heap + GC 路径，行为不变
//   Phase B: 在 CMake 中定义 PYLANG_USE_ARENA
//     → 所有宏展开为 Arena 路径
//   Phase C: 全局启用 Arena 后，删除旧路径代码
//     → compat.hpp 简化为直接调用 Arena API
//
// 已完成的宏替换:
//   PYLANG_ALLOC           ✅ 全局替换完成
//   PYLANG_ALLOC_WITH_EXTRA ✅ 全局替换完成
//   PYLANG_GC_PAUSE_SCOPE  ✅ 全局替换完成
//   PYLANG_CHECK_ALLOC     ⚠️ 部分替换，但无需全部替换不影响正常使用
//   PYLANG_ALLOC_WEAKREF   ✅ weakref 模块内使用
//   PYLANG_WEAKREF_ALIVE   ✅ weakref 模块内使用
//   PYLANG_WEAKREF_COUNT   ✅ weakref/module.cpp 使用
//   PYLANG_WEAKREF_LIST    ✅ weakref/module.cpp 使用
//
// =============================================================================

#ifdef PYLANG_USE_ARENA

// ---- 新路径: Arena ----
#include "memory/Arena.hpp"
#include "memory/ArenaManager.hpp"

#define PYLANG_ALLOC(Type, ...) ::py::Arena::current().allocate<Type>(__VA_ARGS__)

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
	::py::Arena::current().allocate_with_extra<Type>(Extra, __VA_ARGS__)

// Arena 模式下无 GC，暂停操作为 no-op（空展开兼容有无分号两种调用方式）
#define PYLANG_GC_PAUSE_SCOPE()

// WeakRef: Arena 模式下通过 WeakRefRegistry 实现
#include "modules/weakref/WeakRefRegistry.hpp"

#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
	::py::Arena::current().allocate<Type>(Obj, Callback)

#define PYLANG_WEAKREF_ALIVE(ObjPtr) ::py::weakref::is_alive(ObjPtr)

#define PYLANG_WEAKREF_COUNT(ObjPtr) ::py::weakref::ref_count(ObjPtr)

#define PYLANG_WEAKREF_LIST(ObjPtr) ::py::weakref::get_refs(ObjPtr)

#else
// ---- 旧路径: Heap + GC (不变) ----

// #include "vm/VM.hpp"

#define PYLANG_ALLOC(Type, ...) VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__)

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
	VirtualMachine::the().heap().allocate_with_extra_bytes<Type>(Extra, __VA_ARGS__)

#define PYLANG_GC_PAUSE_SCOPE()                         \
	[[maybe_unused]] auto _pylang_gc_pause_##__LINE__ = \
		VirtualMachine::the().heap().scoped_gc_pause();

#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
	VirtualMachine::the().heap().allocate_weakref<Type>(Obj, Callback)

#define PYLANG_WEAKREF_ALIVE(ObjPtr) \
	VirtualMachine::the().heap().has_weakref_object(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_WEAKREF_COUNT(ObjPtr) \
	VirtualMachine::the().heap().weakref_count(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_WEAKREF_LIST(ObjPtr) \
	VirtualMachine::the().heap().get_weakrefs(std::bit_cast<uint8_t *>(ObjPtr))

#endif// PYLANG_USE_ARENA


// =============================================================================
// 通用辅助宏（两种模式下都可用）
// =============================================================================

// 检查分配结果并返回 MemoryError
#define PYLANG_CHECK_ALLOC(ptr, Type)                           \
	do {                                                        \
		if (!(ptr)) { return Err(memory_error(sizeof(Type))); } \
	} while (0)