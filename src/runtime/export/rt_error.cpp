#include "rt_common.hpp"

#include "runtime/AssertionError.hpp"
#include "runtime/BaseException.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyType.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/TypeError.hpp"
#include "runtime/types/builtin.hpp"

#include <cstdio>
#include <cstdlib>

// =============================================================================
// rt_raise —— 所有导出函数的错误出口
//
// Phase 2 实现: 打印异常类型和消息后 abort()
// Phase 4+ 将改为设置线程异常状态 + 返回 null 传播
// =============================================================================

[[noreturn]] void rt_raise(py::BaseException *exc)
{
	if (exc) {
		// 尝试获取异常类型名
		auto *obj = static_cast<py::PyObject *>(exc);
		const std::string &type_name_str =
			obj->type() ? obj->type()->name() : std::string("Exception");

		// 尝试获取异常消息
		auto msg = obj->str();
		if (msg.is_ok()) {
			std::fprintf(stderr, "%s: %s\n", type_name_str.c_str(), msg.unwrap()->value().c_str());
		} else {
			std::fprintf(stderr, "%s\n", type_name_str.c_str());
		}
	} else {
		std::fprintf(stderr, "Unknown exception (null)\n");
	}

	std::abort();
}

// =============================================================================
// Tier 0: raise
// =============================================================================

PYLANG_EXPORT_ERROR("raise", "void", "obj")
void rt_raise_obj(py::PyObject *exc) { rt_raise(static_cast<py::BaseException *>(exc)); }

// =============================================================================
// Tier 6: 更多异常操作
// =============================================================================

PYLANG_EXPORT_ERROR("load_assertion_error", "obj", "")
py::PyObject *rt_load_assertion_error() { return py::types::assertion_error(); }

// =============================================================================
// Tier 6: 异常匹配（Phase 3.3）
// =============================================================================

PYLANG_EXPORT_ERROR("check_exception_match", "bool", "obj,obj")
bool rt_check_exception_match(py::PyObject *exc, py::PyObject *exc_type)
{
	// 纯委托给 runtime 的 check_exception_match
	return py::check_exception_match(exc, exc_type);
}

PYLANG_EXPORT_ERROR("reraise", "void", "obj")
void rt_reraise(py::PyObject *exc)
{
	// raise（无参数）重新抛出当前异常
	// 委托给 rt_raise
	if (!exc) {
		// 没有当前异常时的错误
		rt_raise(py::runtime_error("No active exception to re-raise"));
	}
	rt_raise(static_cast<py::BaseException *>(exc));
}