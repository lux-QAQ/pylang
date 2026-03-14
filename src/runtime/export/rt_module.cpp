#include "rt_common.hpp"
#include "runtime/Import.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/Import.hpp"
#include "runtime/PyModule.hpp"

// =============================================================================
// Tier 0: 模块操作
// 修改签名：增加了 globals 和 locals (与 Python __import__ 规范对齐)
// =============================================================================

PYLANG_EXPORT_MODULE("import", "obj", "str,obj,obj,obj,i32")
py::PyObject *rt_import(const char *module_name,
	py::PyObject *globals,
	py::PyObject *locals,
	py::PyObject *from_list,
	int32_t level)
{
	auto *name_str = rt_unwrap(py::PyString::create(std::string(module_name)));

	// 如果 from_list 没有提供，使用空 tuple 默认值
	if (!from_list) { from_list = rt_unwrap(py::PyTuple::create()); }

	// 严谨语义：委托给运行时的默认 import 实现。
	// globals 参数不能丟，它是确定相对导入基础包路径（__package__）的核心。
	return rt_unwrap(py::import_module_level_object(name_str,
		globals ? py::as<py::PyDict>(globals) : nullptr,
		locals,
		from_list,
		static_cast<uint32_t>(level)));
}

PYLANG_EXPORT_MODULE("add_module", "obj", "str")
py::PyObject *rt_add_module(const char *module_name)
{
	auto *name_str = rt_unwrap(py::PyString::create(std::string(module_name)));
	return rt_unwrap(import_add_module(name_str));
}