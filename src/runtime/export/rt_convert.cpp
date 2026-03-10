#include "rt_common.hpp"

#include "runtime/PyBool.hpp"

// =============================================================================
// 编译器原语（不是 Python 函数，无对应的 builtins 名称）
// =============================================================================

PYLANG_EXPORT_CONVERT("is_true", "bool", "obj")
bool rt_is_true(py::PyObject *obj) { return rt_unwrap(obj->true_()); }

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