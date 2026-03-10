#include "rt_common.hpp"


#include "runtime/PyCell.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/IndexError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/RuntimeError.hpp"

// =============================================================================
// Tier 0: 函数调用
// =============================================================================

PYLANG_EXPORT_FUNC("call", "obj", "obj,obj,obj")
py::PyObject *rt_call(py::PyObject *callable, py::PyObject *args, py::PyObject *kwargs)
{
	auto *tuple_args = static_cast<py::PyTuple *>(args);
	auto *dict_kwargs = kwargs ? static_cast<py::PyDict *>(kwargs) : nullptr;
	return rt_unwrap(callable->call(tuple_args, dict_kwargs));
}

// =============================================================================
// Tier 4: 方法调用
// =============================================================================

PYLANG_EXPORT_FUNC("load_method", "obj", "obj,str")
py::PyObject *rt_load_method(py::PyObject *obj, const char *name)
{
	auto *method_name = rt_unwrap(py::PyString::create(std::string(name)));
	return rt_unwrap(obj->get_method(method_name));
}

// =============================================================================
// Tier 4: tuple 特化访问（闭包实现需要）
// =============================================================================

PYLANG_EXPORT_SUBSCR("tuple_getitem", "obj", "obj,i32")
py::PyObject *rt_tuple_getitem(py::PyObject *tuple, int32_t index)
{
	auto *t = static_cast<py::PyTuple *>(tuple);

	// 使用 PyTuple 的公开接口，自动处理负索引和边界检查
	auto result = t->__getitem__(static_cast<int64_t>(index));

	// rt_unwrap 会在失败时调用 rt_raise
	return rt_unwrap(result);
}

// =============================================================================
// Tier 4: 闭包操作 (Phase 3.2)
// =============================================================================

PYLANG_EXPORT_FUNC("create_cell", "obj", "obj")
py::PyObject *rt_create_cell(py::PyObject *value)
{
	// 捕获闭包变量：为 value 创建一个 cell 对象
	// 委托给 PyCell::create(value)
	if (value) { return rt_unwrap(py::PyCell::create(py::Value{ value })); }
	// value 为 null 表示创建空 cell（变量尚未赋值）
	return rt_unwrap(py::PyCell::create());
}

PYLANG_EXPORT_FUNC("cell_get", "obj", "obj")
py::PyObject *rt_cell_get(py::PyObject *cell)
{
	// 读取闭包变量：从 cell 中取出存储的对象
	// 委托给 PyCell::content()
	auto *cell_obj = static_cast<py::PyCell *>(cell);
	if (cell_obj->empty()) {
		rt_raise(py::name_error("free variable referenced before assignment in enclosing scope"));
	}
	return rt_unwrap(py::PyObject::from(cell_obj->content()));
}

PYLANG_EXPORT_FUNC("cell_set", "void", "obj,obj")
void rt_cell_set(py::PyObject *cell, py::PyObject *value)
{
	// 写入闭包变量：将 value 存入 cell
	// 委托给 PyCell::set_cell(value)
	auto *cell_obj = static_cast<py::PyCell *>(cell);
	cell_obj->set_cell(py::Value{ value });
}

// =============================================================================
// Phase 4+: 快速调用路径（避免 PyTuple 分配）
// =============================================================================

/// 直接传递 C 数组，跳过 PyTuple 打包
/// 对应 CPython 的 vectorcall 协议
PYLANG_EXPORT_FUNC("call_fast", "obj", "obj,i32,ptr")
py::PyObject *rt_call_fast(py::PyObject *callable, int32_t argc, py::PyObject **argv)
{
    // 从 C 数组构造 PyTuple，然后走标准调用协议
    // Phase 4+ 可优化：让 PyNativeFunction 直接接受 (argc, argv)
    std::vector<py::Value> elements;
    elements.reserve(argc);
    for (int32_t i = 0; i < argc; ++i) { elements.push_back(argv[i]); }
    auto *args = rt_unwrap(py::PyTuple::create(elements));
    return rt_unwrap(callable->call(args, nullptr));
}

// =============================================================================
// Tier 4: make_function — 薄包装，逻辑在 PyNativeFunction::create_aot
// =============================================================================

PYLANG_EXPORT_FUNC("make_function", "obj", "str,ptr,obj,obj,obj,obj")
py::PyObject *rt_make_function(
    const char *name,
    void *code_ptr,
    py::PyObject *module,
    py::PyObject *defaults,
    py::PyObject *kwdefaults,
    py::PyObject *closure)
{
    return rt_unwrap(py::PyNativeFunction::create_aot(
        std::string(name), code_ptr, module, defaults, kwdefaults, closure));
}

// =============================================================================
// Tier 4: get_closure — 薄包装，逻辑在 runtime 层
// =============================================================================

PYLANG_EXPORT_FUNC("get_closure", "obj", "obj")
py::PyObject *rt_get_closure(py::PyObject *func)
{
    // PyFunction（字节码解释器创建的函数）
    if (auto *pyfunc = py::as<py::PyFunction>(func)) {
        auto *closure = pyfunc->closure();
        return closure ? static_cast<py::PyObject *>(closure)
                       : rt_unwrap(py::PyTuple::create());
    }

    // PyNativeFunction（AOT 编译器创建的函数）
    if (auto *native = py::as<py::PyNativeFunction>(func)) {
        auto *closure = native->closure();
        return closure ? static_cast<py::PyObject *>(closure)
                       : rt_unwrap(py::PyTuple::create());
    }

    // 其他可调用对象
    return rt_unwrap(py::PyTuple::create());
}