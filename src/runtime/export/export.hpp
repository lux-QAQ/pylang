#pragma once

// =============================================================================
// Pylang Runtime Export Macros
// =============================================================================
//
// 这些宏用于标记 Runtime 中需要暴露给编译器前端的函数。
// **所有导出函数统一放在 src/runtime/export/ 目录下**，即使只是
// 对已有函数的简单转发（如 py_true() → rt_true()），也在此目录中
// 创建包装函数并标注宏，保证导出层的单一管理入口。
//
// 编译器前端通过扫描 runtime.bc 中的 LLVM Annotation 来发现这些函数。
//
// ─── Annotation 格式 ───────────────────────────────────────────────────────
//
//   "pylang_export:<category>:<symbolic_name>:<ret_type>:<param_types>"
//
//   字段:
//     [0] 固定前缀     "pylang_export"
//     [1] category      分类标识符（见下方分类体系）
//     [2] symbolic_name 编译器前端使用的逻辑名（全局唯一，不含 rt_ 前缀）
//     [3] ret_type      返回值类型标记
//     [4] param_types   参数类型列表，逗号分隔；无参数用空串 ""
//
//   分隔符:
//     字段间: ':' (冒号)      — 恰好 4 个冒号，5 个字段
//     参数间: ',' (逗号)      — param_types 内部
//
//   示例:
//     PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
//     → 生成 annotation: "pylang_export:op:binary_add:obj:obj,obj"
//
// ─── 前端解析规范（RuntimeLinker）───────────────────────────────────────────
//
//   1. 加载 runtime.bc，找到全局变量 @llvm.global.annotations
//
//   2. 该变量是 ConstantArray，每个元素是 ConstantStruct，含 4 个字段:
//        element[0] = 被标注函数的指针 (可能经过 bitcast)
//        element[1] = annotation 字符串的 GlobalVariable 指针
//        element[2] = 源文件名字符串指针 (忽略)
//        element[3] = 行号 i32 (忽略)
//
//   3. 从 element[1] 指向的 GlobalVariable 的 initializer 中提取
//      ConstantDataArray 的字节内容，得到以 \0 结尾的 annotation 字符串
//
//   4. 过滤: 仅处理以 "pylang_export:" 开头的字符串
//
//   5. 按 ':' 分割为恰好 5 个字段:
//        fields[0] = "pylang_export"   (已在步骤 4 确认)
//        fields[1] = category
//        fields[2] = symbolic_name
//        fields[3] = ret_type
//        fields[4] = param_types       (可能为空串 "")
//
//   6. 恢复被标注的 llvm::Function*:
//        auto *func = dyn_cast<Function>(
//            element[0]->stripPointerCasts());
//
//   7. 存入索引:
//        symbolic_name → {
//            llvm::Function*     func,
//            llvm::FunctionType* type,     // 从 func->getFunctionType()
//            std::string         category,
//            std::string         ret,
//            vector<string>      params    // split(fields[4], ',')
//        }
//
//   8. 验证规则:
//      - symbolic_name 全局唯一，重复则编译期 error
//      - 字段数量必须为 5，否则 warning 并跳过
//      - params 中的参数数量应与 FunctionType 的参数数量一致
//
// =============================================================================

// 内部拼接宏 —— 不要直接使用
#define PYLANG_EXPORT_IMPL_(cat, name, ret, params) \
    __attribute__((annotate("pylang_export:" cat ":" name ":" ret ":" params)))

// =============================================================================
// 分类体系 (Category)
//
// 每个宏对应一个 category 字符串。所有宏签名统一为:
//   PYLANG_EXPORT_XXX(symbolic_name, ret_type, param_types)
// =============================================================================

/// 二元/一元运算
/// 逻辑名: binary_add, binary_sub, binary_mul, unary_neg, inplace_add, ...
#define PYLANG_EXPORT_OP(name, ret, params) \
    PYLANG_EXPORT_IMPL_("op", name, ret, params)

/// 比较操作
/// 逻辑名: compare_eq, compare_lt, compare_is, compare_in, ...
#define PYLANG_EXPORT_CMP(name, ret, params) \
    PYLANG_EXPORT_IMPL_("cmp", name, ret, params)

/// 对象创建
/// 逻辑名: integer_from_i64, string_from_cstr, float_from_f64,
///         build_list, build_tuple, build_dict, build_set, build_slice, ...
#define PYLANG_EXPORT_CREATE(name, ret, params) \
    PYLANG_EXPORT_IMPL_("create", name, ret, params)

/// 内置函数
/// 逻辑名: print, len, isinstance, type, hasattr, ...
#define PYLANG_EXPORT_BUILTIN(name, ret, params) \
    PYLANG_EXPORT_IMPL_("builtin", name, ret, params)

/// 属性访问
/// 逻辑名: getattr, setattr, delattr, load_global, store_global, load_name, store_name
#define PYLANG_EXPORT_ATTR(name, ret, params) \
    PYLANG_EXPORT_IMPL_("attr", name, ret, params)

/// 下标 / 容器操作
/// 逻辑名: getitem, setitem, delitem, get_iter, iter_next,
///         list_append, list_extend, dict_merge, set_add, unpack_sequence, ...
#define PYLANG_EXPORT_SUBSCR(name, ret, params) \
    PYLANG_EXPORT_IMPL_("subscr", name, ret, params)

/// 类型转换 / 检查
/// 逻辑名: is_true, to_int, to_float, list_to_tuple, ...
#define PYLANG_EXPORT_CONVERT(name, ret, params) \
    PYLANG_EXPORT_IMPL_("convert", name, ret, params)

/// 模块操作
/// 逻辑名: import, import_from, import_star
#define PYLANG_EXPORT_MODULE(name, ret, params) \
    PYLANG_EXPORT_IMPL_("module", name, ret, params)

/// 运行时生命周期
/// 逻辑名: init, shutdown
#define PYLANG_EXPORT_LIFECYCLE(name, ret, params) \
    PYLANG_EXPORT_IMPL_("lifecycle", name, ret, params)

/// 全局单例
/// 逻辑名: true, false, none, ellipsis, not_implemented
#define PYLANG_EXPORT_SINGLETON(name, ret, params) \
    PYLANG_EXPORT_IMPL_("singleton", name, ret, params)

/// 异常 / 错误处理
/// 逻辑名: raise, reraise, check_exception_match, setup_except, leave_except,
///         load_assertion_error
#define PYLANG_EXPORT_ERROR(name, ret, params) \
    PYLANG_EXPORT_IMPL_("error", name, ret, params)

/// 函数 / 闭包操作
/// 逻辑名: make_function, call, call_ex, call_with_kwargs,
///         load_closure, load_deref, store_deref, create_cell, cell_get, cell_set,
///         get_closure, load_method, method_call
#define PYLANG_EXPORT_FUNC(name, ret, params) \
    PYLANG_EXPORT_IMPL_("func", name, ret, params)

/// 类创建 / 继承
/// 逻辑名: load_build_class
#define PYLANG_EXPORT_CLASS(name, ret, params) \
    PYLANG_EXPORT_IMPL_("class", name, ret, params)

/// 通用导出 (不属于以上分类时的兜底)
#define PYLANG_EXPORT(name, ret, params) \
    PYLANG_EXPORT_IMPL_("general", name, ret, params)

// =============================================================================
// 类型标记约定 (用于 ret / params 字段)
//
//   标记      C++ 类型                    LLVM IR 类型
//   ------    --------                    ------------
//   "obj"     PyObject* (裸指针)           ptr (opaque pointer)
//   "str"     const char*                 ptr
//   "i64"     int64_t                     i64
//   "u64"     uint64_t / size_t           i64
//   "f64"     double                      double
//   "bool"    bool                        i1
//   "void"    void                        void
//   "i32"     int32_t                     i32
//   "ptr"     任意指针 (PyObject** 等)     ptr
//
// 语义区分:
//   "obj"  — 指向单个 PyObject 实例的指针（语义上是 Python 对象）
//   "str"  — 指向 C 字符串的指针（不是 PyString*，而是 const char*）
//   "ptr"  — 指向其他数据的指针（如 PyObject** 数组、bool* 输出参数）
//   三者在 LLVM IR 中都是 opaque ptr，但语义不同，前端可据此做验证
//
// 组合:
//   多参数用逗号分隔:  "obj,obj"  "obj,str,i64"  "i32,ptr"
//   无参数用空串:      ""
//
// =============================================================================



//=============================================================================
// PYLANG_INIT — 用户模块初始化函数标记
//
// 用途:
//   标记 AOT 编译器为用户 .py 文件生成的 PyInit_xxx 函数。
//   运行时通过 dlsym(RTLD_DEFAULT, "PyInit_xxx") 查找符号。
//
// 注意:
//   - C++ 内置模块 (builtins, sys, posix 等) 不需要此宏
//   - 它们通过 config.hpp 的 builtin_modules 数组静态注册
//
// 示例:
//   // 编译器为 foo.py 生成的代码
//   PYLANG_INIT("foo")
//   PyModule* PyInit_foo() {
//       auto [mod, is_owner] = ModuleRegistry::instance().get_or_register("foo");
//       if (!is_owner) return mod;
//       InitGuard guard("foo");
//       // ... 执行模块体 ...
//       guard.commit();
//       return mod;
//   }
// =============================================================================

#define PYLANG_INIT(name) \
    __attribute__((annotate("pylang_init_func:" name)))