#pragma once

#include "../utilities.hpp"

#ifndef NDEBUG
#include <atomic>
#include <cpptrace/cpptrace.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>

// =============================================================================
// Pylang 内存分配调试辅助 (动态频率控制)
// =============================================================================
// 支持通过环境变量设置频率：
// export PYLANG_ALLOC_LOG_FREQ=5000     (设置简略日志打印频率，0为关闭)
// export PYLANG_ALLOC_TRACE_FREQ=100000 (设置堆栈打印频率，0为关闭)
inline void _pylang_debug_log_alloc(const char *type_name, std::atomic<size_t> &count)
{
	// 静态变量保证只会在程序启动第一次触发时读取一次环境变量
	static size_t log_freq = []() -> size_t {
		const char *env = std::getenv("PYLANG_ALLOC_LOG_FREQ");
		return env ? std::strtoull(env, nullptr, 10) : 5000;
	}();

	static size_t trace_freq = []() -> size_t {
		const char *env = std::getenv("PYLANG_ALLOC_TRACE_FREQ");
		return env ? std::strtoull(env, nullptr, 10) : 100000;
	}();

	// 先增加计数（返回的是增加前的值，因此我们+1得到当前值）
	size_t c = count.fetch_add(1, std::memory_order_relaxed) + 1;

	// 优先判断堆栈打印（因为它的阈值通常更大，能覆盖日志打印）
	if (trace_freq > 0 && (c % trace_freq) == 0) {
		auto trace = cpptrace::generate_trace();
		spdlog::error("[DEBUG_ALLOC_TRACE] Type: {:<20} | Total: {:>10}\n{}",
			type_name,
			c,
			trace.to_string());
	}
	// 否则判断普通日志打印
	else if (log_freq > 0 && (c % log_freq) == 0) {
		spdlog::error("[DEBUG_ALLOC] Type: {:<20} | Total: {:>10}", type_name, c);
	}
}

#define PYLANG_DEBUG_LOG_ALLOC(Type)                                                     \
	([&]() {                                                                             \
		/* 每个宏展开点 (分配代码行) 拥有独立的计数器，有助于精确定位哪一行在狂暴分配 */ \
		static std::atomic<size_t> _alloc_count{ 0 };                                    \
		_pylang_debug_log_alloc(#Type, _alloc_count);                                    \
	}())

#else
#define PYLANG_DEBUG_LOG_ALLOC(Type) ((void)0)
#endif
// =============================================================================
// Pylang 内存分配兼容层
// =============================================================================

#if defined(PYLANG_USE_ARENA)

#include "memory/Arena.hpp"
#include "memory/ArenaManager.hpp"
#include "modules/weakref/WeakRefRegistry.hpp"

// Arena 模式（内部会自动接管 PYLANG_USE_Boehm_GC 的判定）
#define PYLANG_ALLOC(Type, ...) \
	(PYLANG_DEBUG_LOG_ALLOC(Type), ::py::Arena::current().allocate<Type>(__VA_ARGS__))

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
	(PYLANG_DEBUG_LOG_ALLOC(Type),                \
		::py::Arena::current().allocate_with_extra<Type>(Extra, __VA_ARGS__))

#define PYLANG_GC_PAUSE_SCOPE()

#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
	(PYLANG_DEBUG_LOG_ALLOC(Type), ::py::Arena::current().allocate<Type>(Obj, Callback))

#define PYLANG_WEAKREF_ALIVE(ObjPtr) ::py::weakref::is_alive(ObjPtr)
#define PYLANG_WEAKREF_COUNT(ObjPtr) ::py::weakref::ref_count(ObjPtr)
#define PYLANG_WEAKREF_LIST(ObjPtr) ::py::weakref::get_refs(ObjPtr)

#define PYLANG_ALLOC_IMMORTAL(Type, ...) \
	(PYLANG_DEBUG_LOG_ALLOC(Type), ::py::ArenaManager::program_arena().allocate<Type>(__VA_ARGS__))

// 手动分配原子内存（GC 绝对不会扫描这块内存内部的指针）
// 用于手动构造对象
#define PYLANG_ALLOC_ATOMIC(Type, ...)                                                \
	(PYLANG_DEBUG_LOG_ALLOC(Type),                                                    \
		new (::py::Arena::current().allocate_raw<false>(sizeof(Type), alignof(Type))) \
			Type(__VA_ARGS__))

// 手动分配纯原子裸内存 (类似 malloc，不调构造)
#define PYLANG_ALLOC_ATOMIC_RAW(Size) ::py::Arena::current().allocate_raw<false>(Size)

#else

// 旧的 Heap + MarkSweep GC 模式
#define PYLANG_ALLOC(Type, ...) \
	(PYLANG_DEBUG_LOG_ALLOC(Type), VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__))

#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
	(PYLANG_DEBUG_LOG_ALLOC(Type),                \
		VirtualMachine::the().heap().allocate_with_extra_bytes<Type>(Extra, __VA_ARGS__))

#define PYLANG_GC_PAUSE_SCOPE()                         \
	[[maybe_unused]] auto _pylang_gc_pause_##__LINE__ = \
		VirtualMachine::the().heap().scoped_gc_pause();

#define PYLANG_ALLOC_WEAKREF(Type, Obj, Callback) \
	(PYLANG_DEBUG_LOG_ALLOC(Type),                \
		VirtualMachine::the().heap().allocate_weakref<Type>(Obj, Callback))

#define PYLANG_WEAKREF_ALIVE(ObjPtr) \
	VirtualMachine::the().heap().has_weakref_object(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_WEAKREF_COUNT(ObjPtr) \
	VirtualMachine::the().heap().weakref_count(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_WEAKREF_LIST(ObjPtr) \
	VirtualMachine::the().heap().get_weakrefs(std::bit_cast<uint8_t *>(ObjPtr))

#define PYLANG_ALLOC_IMMORTAL(Type, ...) VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__)

#define PYLANG_ALLOC_ATOMIC(Type, ...) \
	(PYLANG_DEBUG_LOG_ALLOC(Type), VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__))

#define PYLANG_ALLOC_ATOMIC_RAW(Size) VirtualMachine::the().heap().allocate_raw(Size)

#endif

// =============================================================================
// 通用辅助宏
// =============================================================================
#define PYLANG_CHECK_ALLOC(ptr, Type)                           \
	do {                                                        \
		if (!(ptr)) { return Err(memory_error(sizeof(Type))); } \
	} while (0)
