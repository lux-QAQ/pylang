#include "rt_common.hpp"
#include "runtime/Import.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

PYLANG_EXPORT_MODULE("import", "obj", "str,obj,obj,obj,i32")
py::PyObject *rt_import(const char *module_name,
	py::PyObject *globals,
	py::PyObject *locals,
	py::PyObject *from_list,
	int32_t level)
{
	auto *name_str = rt_unwrap(py::PyString::create(std::string(module_name)));
	auto *b_globals = py::ensure_box(globals);
	auto *b_from_list = py::ensure_box(from_list);

	if (!b_from_list) { b_from_list = rt_unwrap(py::PyTuple::create()); }

	return rt_unwrap(py::import_module_level_object(name_str,
		b_globals ? py::as<py::PyDict>(b_globals) : nullptr,
		py::ensure_box(locals),
		b_from_list,
		static_cast<uint32_t>(level)));
}

PYLANG_EXPORT_MODULE("add_module", "obj", "str")
py::PyObject *rt_add_module(const char *module_name)
{
	auto *name_str = rt_unwrap(py::PyString::create(std::string(module_name)));
	return rt_unwrap(import_add_module(name_str));
}