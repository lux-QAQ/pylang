#include "rt_common.hpp"

#include "runtime/NotImplemented.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyEllipsis.hpp"
#include "runtime/PyNone.hpp"

// =============================================================================
// Tier 0: 全局单例
// =============================================================================

PYLANG_EXPORT_SINGLETON("none", "obj", "")
py::PyObject *rt_none() { return py::py_none(); }

PYLANG_EXPORT_SINGLETON("true", "obj", "")
py::PyObject *rt_true() { return py::py_true(); }

PYLANG_EXPORT_SINGLETON("false", "obj", "")
py::PyObject *rt_false() { return py::py_false(); }

PYLANG_EXPORT_SINGLETON("ellipsis", "obj", "")
py::PyObject *rt_ellipsis() { return py::py_ellipsis(); }

PYLANG_EXPORT_SINGLETON("not_implemented", "obj", "")
py::PyObject *rt_not_implemented() { return py::not_implemented(); }