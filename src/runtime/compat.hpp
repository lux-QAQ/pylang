#pragma once

#include "../utilities.hpp"

// =============================================================================
// Pylang 内存分配兼容层
// =============================================================================

#if defined(PYLANG_USE_ARENA)

#include "memory/Arena.hpp"
#include "memory/ArenaManager.hpp"
#include "modules/weakref/WeakRefRegistry.hpp"

// Arena 模式（内部会自动接管 PYLANG_USE_Boehm_GC 的判定）
#define PYLANG_ALLOC(Type, ...) ::py::Arena::current().allocate<Type>(__VA_ARGS__)

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
    ::py::Arena::current().allocate_with_extra<Type>(Extra, __VA_ARGS__)

#define PYLANG_GC_PAUSE_SCOPE()

#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
    ::py::Arena::current().allocate<Type>(Obj, Callback)

#define PYLANG_WEAKREF_ALIVE(ObjPtr) ::py::weakref::is_alive(ObjPtr)
#define PYLANG_WEAKREF_COUNT(ObjPtr) ::py::weakref::ref_count(ObjPtr)
#define PYLANG_WEAKREF_LIST(ObjPtr) ::py::weakref::get_refs(ObjPtr)

#define PYLANG_ALLOC_IMMORTAL(Type, ...) \
    ::py::ArenaManager::program_arena().allocate<Type>(__VA_ARGS__)

#else

// 旧的 Heap + MarkSweep GC 模式
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

#define PYLANG_ALLOC_IMMORTAL(Type, ...) VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__)

#endif

// =============================================================================
// 通用辅助宏
// =============================================================================
#define PYLANG_CHECK_ALLOC(ptr, Type)                           \
    do {                                                        \
        if (!(ptr)) { return Err(memory_error(sizeof(Type))); } \
    } while (0)