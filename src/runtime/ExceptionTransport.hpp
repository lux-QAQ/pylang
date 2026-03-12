#pragma once

#include "forward.hpp"
#include <exception>

namespace py {

/// C++ 异常传输类型 — 唯一的 throw 入口是 rt_raise
/// 使用 C++ 异常确保：
///   1. RAII guard 正确析构（std::unique_ptr、lock_guard 等）
///   2. LLVM landingpad 能捕获
///   3. 零成本异常：正常路径无开销（Itanium ABI table-driven unwinding）
struct PylangException : std::exception
{
	BaseException *exc;
	explicit PylangException(BaseException *e) noexcept : exc(e) {}
	const char *what() const noexcept override { return "PylangException"; }
};

}// namespace py