#pragma once

// =============================================================================
// Pylang Runtime Export Macros
// =============================================================================
//
// 这些宏用于标记 Runtime 中需要暴露给编译器前端的函数。
// 编译器前端通过扫描 runtime.bc 中的 LLVM Annotation 来发现这些函数。
//
// Annotation 格式 (字符串，编码在 llvm.global.annotations 中):
//   "pylang_export:<category>:<symbolic_name>:<ret_type>:<param_types>"
//
// 其中:
//   category      = "op" | "builtin" | "attr" | "create" | "convert" | "module"
//   symbolic_name = 编译器前端使用的逻辑名 (如 "binary_add", "print", "getattr")
//   ret_type      = 返回值类型标记 (如 "obj", "bool", "i64", "void")
//   param_types   = 参数类型列表，逗号分隔 (如 "obj,obj" 或 "obj,str")
//
// 示例:
//   PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
//   -> annotation: "pylang_export:op:binary_add:obj:obj,obj"
//
// 编译器前端解析流程:
//   1. 加载 runtime.bc
//   2. 遍历所有 Function，检查 "annotate" 属性
//   3. 匹配 "pylang_export:" 前缀
//   4. 按 ':' 分割，提取 category / name / ret / params
//   5. 建立 symbolic_name -> llvm::Function* 映射表
//
// 这样前端不需要知道 C++ mangled name，只需要知道逻辑名即可。
// =============================================================================

// 内部拼接宏 —— 不要直接使用
#define PYLANG_EXPORT_IMPL_(cat, name, ret, params) \
	__attribute__((annotate("pylang_export:" cat ":" name ":" ret ":" params)))

// ---- 公开宏 ----

// 二元/一元运算: add, sub, mul, truediv, floordiv, mod, pow, lshift, rshift, ...
// 用法: PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
#define PYLANG_EXPORT_OP(name, ret, params) PYLANG_EXPORT_IMPL_("op", name, ret, params)

// 内置函数: print, len, isinstance, type, ...
// 用法: PYLANG_EXPORT_BUILTIN("print", "obj", "obj")
#define PYLANG_EXPORT_BUILTIN(name, ret, params) PYLANG_EXPORT_IMPL_("builtin", name, ret, params)

// 属性访问: getattr, setattr, delattr
// 用法: PYLANG_EXPORT_ATTR("getattr", "obj", "obj,str")
#define PYLANG_EXPORT_ATTR(name, ret, params) PYLANG_EXPORT_IMPL_("attr", name, ret, params)

// 对象创建: integer_create, string_create, list_create, ...
// 用法: PYLANG_EXPORT_CREATE("integer_from_i64", "obj", "i64")
#define PYLANG_EXPORT_CREATE(name, ret, params) PYLANG_EXPORT_IMPL_("create", name, ret, params)

// 类型转换 / 检查: to_bool, to_int, is_true, ...
// 用法: PYLANG_EXPORT_CONVERT("to_bool", "bool", "obj")
#define PYLANG_EXPORT_CONVERT(name, ret, params) PYLANG_EXPORT_IMPL_("convert", name, ret, params)

// 模块初始化函数
// 用法: PYLANG_EXPORT_MODULE("sys")  (无参数，返回 obj)
#define PYLANG_EXPORT_MODULE(name) PYLANG_EXPORT_IMPL_("module", name, "obj", "")

// 下标 / 容器操作: getitem, setitem, delitem, contains, iter, next
// 用法: PYLANG_EXPORT_SUBSCR("getitem", "obj", "obj,obj")
#define PYLANG_EXPORT_SUBSCR(name, ret, params) PYLANG_EXPORT_IMPL_("subscr", name, ret, params)

// 比较操作: eq, ne, lt, le, gt, ge
// 用法: PYLANG_EXPORT_CMP("eq", "obj", "obj,obj")
#define PYLANG_EXPORT_CMP(name, ret, params) PYLANG_EXPORT_IMPL_("cmp", name, ret, params)

// 通用导出 (不属于以上分类时的兜底)
// 用法: PYLANG_EXPORT("call", "obj", "obj,obj")
#define PYLANG_EXPORT(name, ret, params) PYLANG_EXPORT_IMPL_("general", name, ret, params)

// =============================================================================
// 类型标记约定 (用于 ret / params 字段):
//
//   "obj"    -> PyObjPtr  (std::shared_ptr<PyObject>)
//   "str"    -> std::string const&  或 PyObjPtr wrapping PyString
//   "i64"    -> int64_t
//   "u64"    -> uint64_t
//   "f64"    -> double
//   "bool"   -> bool
//   "void"   -> void
//   "tuple"  -> PyObjPtr wrapping PyTuple
//   "dict"   -> PyObjPtr wrapping PyDict
//   "list"   -> PyObjPtr wrapping PyList
//   "none"   -> 返回 None 单例
//
// 复合参数用逗号分隔: "obj,obj"  "obj,str,i64"
// 无参数用空串: ""
// =============================================================================
