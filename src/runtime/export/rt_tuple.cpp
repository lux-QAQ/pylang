#include "rt_common.hpp"

#include "runtime/PyTuple.hpp"

PYLANG_EXPORT_SUBSCR("tuple_size", "i32", "obj")
int32_t rt_tuple_size(py::PyObject *obj)
{
    auto *tuple = static_cast<py::PyTuple *>(obj);
    return static_cast<int32_t>(tuple->size());
}