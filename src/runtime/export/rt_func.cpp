#include "rt_common.hpp"

#include "runtime/IndexError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/PyCell.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"

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