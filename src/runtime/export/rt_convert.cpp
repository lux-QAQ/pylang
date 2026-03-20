#include "rt_common.hpp"

#include "runtime/BaseException.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyTraceback.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"

// =============================================================================
// 编译器原语（不是 Python 函数，无对应的 builtins 名称）
// =============================================================================

PYLANG_EXPORT_CONVERT("is_true", "bool", "obj")
bool rt_is_true(py::PyObject *obj) { return py::RtValue::flatten(obj).is_truthy(); }


/// list → tuple 转换（用于 *args 展开后的 list→tuple）
PYLANG_EXPORT_CONVERT("list_to_tuple", "obj", "obj")
py::PyObject *rt_list_to_tuple(py::PyObject *list)
{
	auto *l = static_cast<py::PyList *>(py::ensure_box(list));
	return rt_unwrap(py::PyTuple::create(l->elements()));
}

// =============================================================================
// 以下函数已移除，改为通过 builtins 模块访问:
//
//   rt_len        → builtins.len     (语义差异: 协议优先级不同, 返回类型不同)
//   rt_isinstance → builtins.isinstance
//   rt_type_of    → builtins.type
//   rt_to_int     → builtins.int     (类型构造器, 可能被子类化/shadow)
//   rt_to_str     → builtins.str
//   rt_to_float   → builtins.float
//   rt_to_bool    → builtins.bool
//
// 原因:
//   1. Python 允许 shadowing: len = lambda x: 42
//   2. 返回类型不同: rt_len 返回 int64_t, builtins.len 返回 PyInteger
//   3. 参数验证不同: rt_len 不检查参数数量
//   4. int/str/float/bool 是类型构造器, 可能被继承重写
//
// 正确调用路径:
//   %len_fn = call ptr @rt_load_global(%mod, "len")
//   %args   = call ptr @rt_build_tuple(i32 1, ptr %arr)
//   %result = call ptr @rt_call(%len_fn, %args, null)
//
// Phase 4+ 可添加 speculative optimization:
//   如果编译器能证明 len 未被 shadow, 可以内联快速路径
// =============================================================================


/// type(obj) — 用于 with.__exit__(type(exc), exc, tb)
PYLANG_EXPORT_CONVERT("type_of", "obj", "obj")
py::PyObject *rt_type_of(py::PyObject *obj)
{
	// obj->type() 返回原生 PyType* 指针，不要 rt_unwrap
	return static_cast<py::PyObject *>(py::ensure_box(obj)->type());
}

/// exc.__traceback__ — 用于 with.__exit__
PYLANG_EXPORT_CONVERT("get_traceback", "obj", "obj")
py::PyObject *rt_get_traceback(py::PyObject *exc)
{
	auto *base_exc = static_cast<py::BaseException *>(py::ensure_box(exc));
	auto *tb = base_exc->traceback();
	return tb ? static_cast<py::PyObject *>(tb) : py::py_none();
}