#pragma once

// =============================================================================
// 导出层公共头文件
// 所有 rt_*.cpp 文件都应包含此头文件
// =============================================================================

#include "export.hpp"

#include "runtime/PyObject.hpp"
#include "runtime/Value.hpp"

// 前向声明 rt_raise —— 定义在 rt_error.cpp 中
// Phase 2: 打印异常信息后 abort()
// Phase 4+: 设置线程异常状态，由调用者检查 null 传播
[[noreturn]] void rt_raise(py::BaseException *exc);

// =============================================================================
// rt_unwrap —— PyResult 解包模板函数
//
// 用法:
//   auto *result = rt_unwrap(some_pyresult_expr);
//
// 检查 is_err()，如果是错误则调用 rt_raise 并中止
// 否则返回 unwrap() 的值
//
// 注意: 由于 rt_raise 是 [[noreturn]]，编译器知道错误路径不会返回，
//       不会产生"未初始化变量"之类的警告。
// =============================================================================

template<typename T> inline T rt_unwrap(py::PyResult<T> result)
{
	if (result.is_err()) { rt_raise(result.unwrap_err()); }
	return result.unwrap();
}

// rt_unwrap_void —— 用于返回 PyResult<std::monostate> 的情况
template<typename T> inline void rt_unwrap_void(py::PyResult<T> result)
{
	if (result.is_err()) { rt_raise(result.unwrap_err()); }
}