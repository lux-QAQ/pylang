#pragma once

#include "PyObject.hpp"

namespace py {

class PyTuple;
class PyDict;

/// AOT 模式类创建 — 不依赖解释器
///
/// 与 BuiltinsModule::build_class 的区别仅在类体执行方式:
///   build_class:     call_with_frame(ns, args, kwargs) — 帧 locals = ns
///   build_class_aot: call(PyTuple(ns), nullptr)        — ns 作为 args[0]
///
/// metaclass 协议、C3 MRO、__prepare__、__classcell__ 处理完全一致
///
/// @param body_fn  类体函数（PyNativeFunction，AOT 编译器通过 rt_make_function 创建）
/// @param name     类名
/// @param bases    基类元组
/// @param kwargs   关键字参数（metaclass=... 等），可为 nullptr
PyResult<PyObject *>
	build_class_aot(PyObject *body_fn, const std::string &name, PyTuple *bases, PyDict *kwargs);

}// namespace py