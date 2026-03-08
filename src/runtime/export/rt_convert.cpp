#include "rt_common.hpp"

#include "runtime/PyBool.hpp"

// =============================================================================
// Tier 1: 类型转换
// =============================================================================

PYLANG_EXPORT_CONVERT("is_true", "bool", "obj")
bool rt_is_true(py::PyObject *obj) { return rt_unwrap(obj->true_()); }