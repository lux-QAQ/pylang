#pragma once

#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyType.hpp"
#include <atomic>

// 增加可配置的宏控制多态内联缓存 (PIC) 的槽位数量
#ifndef PYLANG_METHOD_CACHE_SLOTS
#define PYLANG_METHOD_CACHE_SLOTS 4
#endif

namespace py {
namespace cache {

	// 单个多态内联缓存槽位
	struct MethodCacheSlot
	{
		std::atomic<py::PyType *> expected_type{ nullptr };
		std::atomic<uint64_t> type_version{ 0 };
		std::atomic<py::PyString *> attr_name{ nullptr };
		std::atomic<py::PyObject *> resolved_func{ nullptr };

		// 0: Invalid
		// 1: Function / Native Function (Needs 'self' prepended)
		// 2: Non-Descriptor Callable (No 'self' prepended)
		std::atomic<uint8_t> tag{ 0 };
	};

	// 驻留在 LLVM 数据段的连续结构体
	struct MethodCache
	{
		MethodCacheSlot slots[PYLANG_METHOD_CACHE_SLOTS];
		std::atomic<uint8_t> next_evict_slot{ 0 };// 极简循环淘汰计数
	};

}// namespace cache
}// namespace py
