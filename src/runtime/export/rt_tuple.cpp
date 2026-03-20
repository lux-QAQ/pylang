#include "rt_common.hpp"

#include "runtime/PyTuple.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

PYLANG_EXPORT_SUBSCR("tuple_size", "i32", "obj")
int32_t rt_tuple_size(py::PyObject *obj)
{
	// C++ 获取 size 本身不返回 PyResult，故不不需要也不恩使用 rt_unwrap
	auto *tuple = static_cast<py::PyTuple *>(py::ensure_box(obj));
	return static_cast<int32_t>(tuple->size());
}