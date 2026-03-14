#include "rt_common.hpp"

#include "runtime/AssertionError.hpp"
#include "runtime/BaseException.hpp"
#include "runtime/ExceptionTransport.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyType.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/TypeError.hpp"
#include "runtime/types/builtin.hpp"

#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>

// =============================================================================
// rt_raise — 所有异常的唯一出口
//
// 实现: throw PylangException{exc}
//   → C++ 栈回退 → RAII 安全
//   → LLVM landingpad 捕获
//   → 如无 landingpad: terminate → 打印异常后 abort
// =============================================================================

[[noreturn]] void rt_raise(py::BaseException *exc)
{
	if (!exc) {
		std::fprintf(stderr, "Fatal: rt_raise called with null exception\n");
		std::abort();
	}
	throw py::PylangException(exc);
}

// =============================================================================
// C++ EH ABI 包装 — 隐藏 __cxa_* 细节
//
// 这些函数由 codegen 在 landingpad 后调用。
// 编译器生成的 IR 不需要知道 Itanium ABI 细节。
// =============================================================================

PYLANG_EXPORT_ERROR("catch_begin", "obj", "ptr")
py::PyObject *rt_catch_begin(void *exception_ptr)
{
	// __cxa_begin_catch 注册当前活跃异常并返回异常对象指针
	auto *thrown = abi::__cxa_begin_catch(exception_ptr);
	auto *pe = static_cast<py::PylangException *>(thrown);
	return static_cast<py::PyObject *>(pe->exc);
}

PYLANG_EXPORT_ERROR("catch_end", "void", "")
void rt_catch_end()
{
	// __cxa_end_catch 递减异常引用计数
	// 必须与每个 __cxa_begin_catch 配对
	abi::__cxa_end_catch();
}

PYLANG_EXPORT_ERROR("catch_rethrow", "void", "")
[[noreturn]] void rt_catch_rethrow()
{
	// 重新抛出当前活跃异常（except handler 未匹配时）
	// 必须在 __cxa_begin_catch / __cxa_end_catch 之间调用
	abi::__cxa_rethrow();
}

// =============================================================================
// Tier 0: raise
// =============================================================================

PYLANG_EXPORT_ERROR("raise", "void", "obj")
void rt_raise_obj(py::PyObject *exc)
{
	// 如果 exc 是类型（如 ValueError），实例化它
	if (auto *type = py::as<py::PyType>(exc)) {
		auto args = py::PyTuple::create();
		if (args.is_err()) { rt_raise(args.unwrap_err()); }
		auto instance = type->__call__(args.unwrap(), nullptr);
		if (instance.is_err()) { rt_raise(instance.unwrap_err()); }
		rt_raise(static_cast<py::BaseException *>(instance.unwrap()));
	}

	// 已经是实例
	rt_raise(static_cast<py::BaseException *>(exc));
}

// =============================================================================
// Tier 6: 异常匹配
// =============================================================================

PYLANG_EXPORT_ERROR("load_assertion_error", "obj", "")
py::PyObject *rt_load_assertion_error() { return py::types::assertion_error(); }

PYLANG_EXPORT_ERROR("check_exception_match", "bool", "obj,obj")
bool rt_check_exception_match(py::PyObject *exc, py::PyObject *exc_type)
{
	return py::check_exception_match(exc, exc_type);
}

PYLANG_EXPORT_ERROR("reraise", "void", "obj")
void rt_reraise(py::PyObject *exc)
{
	if (!exc) { rt_raise(py::runtime_error("No active exception to re-raise")); }
	rt_raise(static_cast<py::BaseException *>(exc));
}

PYLANG_EXPORT_ATTR("print_unhandled_exception", "void", "obj")
void rt_print_unhandled_exception(py::PyObject *exc)
{
	if (exc) {
		spdlog::error("Unhandled exception: {}", exc->to_string());
	} else {
		spdlog::error("Unhandled unknown exception");
	}
}