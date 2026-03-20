#include "rt_common.hpp"

#include "runtime/ClassBuilder.hpp"
#include "runtime/ModuleRegistry.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

// =============================================================================
// Tier 5: 类创建 (Phase 3.3)
// =============================================================================
//
// __build_class__ 是 Python 类创建的核心机制。
//
// Python 代码:
//   class Foo(Bar):
//       x = 1
//       def method(self): ...
//
// 编译器生成的调用序列:
//   1. %bc = call ptr @rt_load_build_class()
//   2. %body_fn = ... (编译器生成的类体函数, 通过 rt_make_function 包装)
//   3. %name = call ptr @rt_string_from_cstr("Foo")
//   4. %base = call ptr @rt_load_global(%mod, "Bar")
//   5. %args = call ptr @rt_build_tuple(3, [%body_fn, %name, %base])
//   6. %cls = call ptr @rt_call(%bc, %args, null)
//   7. call void @rt_store_global(%mod, "Foo", %cls)
//
// __build_class__(body_fn, name, *bases, **kwargs) 内部:
//   1. 确定 metaclass (默认 type)
//   2. 调用 metaclass.__prepare__(name, bases) 创建命名空间 ns
//   3. 调用 body_fn(ns) 执行类体，填充 ns
//   4. 调用 metaclass(name, bases, ns) 创建类对象
//
// =============================================================================

PYLANG_EXPORT_CLASS("load_build_class", "obj", "")
py::PyObject *rt_load_build_class()
{
	// 从 builtins 模块中查找 __build_class__
	// 它在 BuiltinsModule.cpp 中注册为 PyNativeFunction
	auto &reg = py::ModuleRegistry::instance();
	auto *builtins = reg.find("builtins");
	if (!builtins) { rt_raise(py::runtime_error("builtins module not initialized")); }

	auto result = builtins->find_symbol_cstr("__build_class__");
	if (result.is_ok()) { return result.unwrap(); }

	rt_raise(py::name_error("__build_class__ not found in builtins"));
	return nullptr;// unreachable
}

PYLANG_EXPORT_CLASS("build_class_aot", "obj", "obj,str,obj,obj")
py::PyObject *rt_build_class_aot(py::PyObject *body_fn,
	const char *name,
	py::PyObject *bases,
	py::PyObject *kwargs)
{
	// name 字符串来自 LLVM IR 常量，可安全 intern
	auto *interned = py::PyString::intern(name);
	return rt_unwrap(py::build_class_aot(py::ensure_box(body_fn),
		interned->value(),
		static_cast<py::PyTuple *>(py::ensure_box(bases)),
		kwargs ? static_cast<py::PyDict *>(py::ensure_box(kwargs)) : nullptr));
}