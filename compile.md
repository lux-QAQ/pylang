# Phase 2: 编译器 Codegen 详细计划

## 0. 架构总览

```
  Python Source ─→ [Lexer] ─→ [Parser] ─→ [AST]
                                            │
                              ┌──────────────┴──────────────┐
                              │                             │
                        [BytecodeGen]                [PylangCodegen] ← NEW
                              │                             │
                        [VM eval_loop]              [LLVM IR Module]
                         (解释执行)                        │
                                                    [Link runtime.bc]
                                                           │
                                                    [LLVM Backend]
                                                           │
                                                    [Native Binary]
```

编译流程分为 **Development Time** 和 **Compile Time** 两个阶段：

- **Development Time**: `src/runtime/` 的 C++ 代码被 Clang 编译为 LLVM Bitcode（`runtime.bc`）。其中 `src/runtime/export/` 目录下的包装函数通过 `export.hpp` 的 `annotate` 属性标记导出，元数据嵌入 `llvm.global.annotations` section。
- **Compile Time**: 编译器前端加载 `runtime.bc`，通过 `RuntimeLinker` 扫描 annotation 建立逻辑名到 `llvm::Function*` 的映射表。`PylangCodegen` 遍历 AST，通过 `IREmitter` 生成对 runtime 函数的调用。最终用户 IR 与 runtime IR 链接，经 LLVM 优化后生成原生二进制。

## 1. 子任务分解

| Phase | 内容 | 状态 |
|:---:|:---|:---:|
| 2.1 | Runtime 导出层 + Bitcode 生成 | 待实施 |
| 2.2 | RuntimeLinker — annotation 驱动的服务发现 | 待实施 |
| 2.3 | PylangCodegen — AST → LLVM IR | 待实施 |
| 2.4 | AOT Driver — IR → 可执行文件 | 待实施 |
| 2.5 | 里程碑验证 — `print("hello world")` | 待实施 |

---

## 2. Phase 2.1: Runtime 导出与 Bitcode 生成

### 2.1.1 导出机制设计

Runtime 函数通过 `__attribute__((annotate(...)))` 标记导出，**不使用 `extern "C"`**。annotation 在编译为 bitcode 后保留在 `llvm.global.annotations` 中，RuntimeLinker 通过扫描此 section 发现导出函数。

**annotation 格式**：
```
pylang_export:<category>:<symbolic_name>:<ret_type>:<param_types>
```

前端永远不需要知道 C++ mangled name，只需要逻辑名（如 `"binary_add"`）即可。这保留了 C++ 的类型安全性，同时提供了稳定的 ABI 发现机制。

**分类体系**（`export.hpp` 中定义的宏，所有宏签名统一为 `(name, ret, params)`）：

| 宏 | category | 用途 | 示例 |
|:---|:---|:---|:---|
| `PYLANG_EXPORT_OP` | `op` | 二元/一元运算 | `binary_add`, `unary_neg`, `inplace_add` |
| `PYLANG_EXPORT_CMP` | `cmp` | 比较操作 | `compare_eq`, `compare_lt`, `compare_is` |
| `PYLANG_EXPORT_CREATE` | `create` | 对象创建 | `integer_from_i64`, `string_from_cstr`, `build_list` |
| `PYLANG_EXPORT_BUILTIN` | `builtin` | 内置函数 | `print`, `len`, `isinstance` |
| `PYLANG_EXPORT_ATTR` | `attr` | 属性访问 | `getattr`, `setattr`, `load_global` |
| `PYLANG_EXPORT_SUBSCR` | `subscr` | 下标/容器 | `getitem`, `setitem`, `get_iter`, `iter_next` |
| `PYLANG_EXPORT_CONVERT` | `convert` | 类型转换 | `is_true`, `to_int`, `list_to_tuple` |
| `PYLANG_EXPORT_MODULE` | `module` | 模块操作 | `import`, `import_from`, `import_star` |
| `PYLANG_EXPORT_LIFECYCLE` | `lifecycle` | 运行时生命周期 | `init`, `shutdown` |
| `PYLANG_EXPORT_SINGLETON` | `singleton` | 全局单例 | `true`, `false`, `none`, `ellipsis` |
| `PYLANG_EXPORT_ERROR` | `error` | 异常/错误处理 | `raise`, `reraise`, `check_exception_match` |
| `PYLANG_EXPORT_FUNC` | `func` | 函数/闭包操作 | `make_function`, `call`, `load_closure` |
| `PYLANG_EXPORT_CLASS` | `class` | 类创建/继承 | `load_build_class` |
| `PYLANG_EXPORT` | `general` | 兜底 | — |

**类型标记约定**：

| 标记 | C++ 类型 | LLVM IR 类型 | 语义 |
|:---|:---|:---|:---|
| `obj` | `PyObject*` | `ptr` | 指向单个 Python 对象 |
| `str` | `const char*` | `ptr` | C 字符串，不是 PyString* |
| `ptr` | 任意指针（`PyObject**` 等） | `ptr` | 数组指针、输出参数等 |
| `i64` | `int64_t` | `i64` | |
| `u64` | `uint64_t` / `size_t` | `i64` | |
| `f64` | `double` | `double` | |
| `bool` | `bool` | `i1` | |
| `void` | `void` | `void` | |
| `i32` | `int32_t` | `i32` | |

`obj`、`str`、`ptr` 三者在 LLVM IR 中都是 opaque `ptr`，但语义不同。前端可据此做参数验证（例如传入 `i64` 到一个期望 `obj` 的位置应该报错）。

### 2.1.2 export.hpp 现状与改动

当前 `export.hpp` 已有的基础宏：`PYLANG_EXPORT_OP`、`PYLANG_EXPORT_CMP`、`PYLANG_EXPORT_CREATE`、`PYLANG_EXPORT_BUILTIN`、`PYLANG_EXPORT_ATTR`、`PYLANG_EXPORT_SUBSCR`、`PYLANG_EXPORT_CONVERT`、`PYLANG_EXPORT_MODULE`、`PYLANG_EXPORT`。

**需要的改动**：

1. **新增 5 个分类宏**：`PYLANG_EXPORT_LIFECYCLE`、`PYLANG_EXPORT_SINGLETON`、`PYLANG_EXPORT_ERROR`、`PYLANG_EXPORT_FUNC`、`PYLANG_EXPORT_CLASS`。
2. **统一 `PYLANG_EXPORT_MODULE` 签名**：原版硬编码了 `ret="obj"` 和 `params=""`，改为与其他宏一致的 `(name, ret, params)` 三参数形式。
3. **修正类型标记文档**：`obj` 的描述从 `PyObjPtr` / `std::shared_ptr<PyObject>` 改为 `PyObject*`（裸指针）。新增 `ptr` 标记用于 `PyObject**` 等非对象指针。
4. **新增前端解析规范**：在注释中详细说明 `@llvm.global.annotations` 的 `ConstantStruct` 结构（4 个字段，element[2]/[3] 可忽略），以及字段分割和验证规则。

### 2.1.3 导出层策略：统一包装

**所有导出函数统一放在 `src/runtime/export/` 目录下**，无论是简单转发还是需要 PyResult 解包的包装。这意味着：

- 即使 `py_true()` 已经是自由函数且返回 `PyObject*`，也在 export/ 中创建 `rt_true()` 包装
- 所有导出函数使用 `rt_` 前缀命名
- 每个函数前放置对应的 `PYLANG_EXPORT_*` 宏

**理由**：

| 方面 | 统一包装 | 分层标注（旧方案） |
|:---|:---|:---|
| 管理 | 所有导出集中在一个目录，grep/audit 简单 | 分散在整个 runtime 代码库中 |
| 维护 | 修改导出签名只需改 export/，不影响 runtime 内部 | runtime 内部函数签名变化直接影响前端 |
| 隔离 | export/ 是 runtime 和前端之间的防火墙 | 前端直接依赖 runtime 内部函数 |
| 解耦 | RuntimeContext、PyResult 等内部机制对前端完全透明 | 前端需要理解哪些函数返回 PyResult |
| 一致性 | 所有函数统一模式，无需判断规则 | 需要判断"是否成员函数"、"是否返回 PyResult" |
| 开销 | 多一层函数调用（LTO 内联后消除） | 无额外调用 |

**包装函数的标准模式**：

```cpp
// 模式 1: 简单转发（无 PyResult）
PYLANG_EXPORT_SINGLETON("true", "obj", "")
PyObject *rt_true() { return py::py_true(); }

// 模式 2: PyResult 解包
PYLANG_EXPORT_CREATE("integer_from_i64", "obj", "i64")
PyObject *rt_integer_from_i64(int64_t value)
{
    auto result = py::PyInteger::create(value);
    if (result.is_err()) { rt_raise(result.unwrap_err()); }
    return result.unwrap();
}

// 模式 3: 成员函数展平 + PyResult 解包
PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
PyObject *rt_binary_add(PyObject *lhs, PyObject *rhs)
{
    auto result = lhs->add(rhs);
    if (result.is_err()) { rt_raise(result.unwrap_err()); }
    return result.unwrap();
}
```

### 2.1.4 导出文件组织

`src/runtime/export/` 目录按 category 分文件，每个文件包含同一分类的所有包装函数：

```
src/runtime/export/
├── export.hpp              ← 宏定义 + 约定文档
├── rt_lifecycle.cpp        ← init, shutdown
├── rt_singleton.cpp        ← true, false, none, ellipsis, not_implemented
├── rt_create.cpp           ← integer_from_i64, string_from_cstr, float_from_f64,
│                              build_tuple, build_list, build_dict, build_set, ...
├── rt_op.cpp               ← binary_add, binary_sub, ..., unary_neg, ..., inplace_add, ...
├── rt_cmp.cpp              ← compare_eq, compare_lt, ..., compare_is, compare_in, ...
├── rt_attr.cpp             ← getattr, setattr, delattr, load_global, store_global, ...
├── rt_subscr.cpp           ← getitem, setitem, delitem, get_iter, iter_next,
│                              list_append, list_extend, dict_merge, ...
├── rt_convert.cpp          ← is_true, to_int, list_to_tuple, ...
├── rt_func.cpp             ← make_function, call, call_ex, call_with_kwargs,
│                              load_closure, load_deref, store_deref, create_cell, ...
├── rt_class.cpp            ← load_build_class
├── rt_error.cpp            ← raise, reraise, check_exception_match, ...
├── rt_module.cpp           ← import, import_from, import_star
└── CMakeLists.txt          ← 编译规则
```

命名约定：
- 文件名 `rt_<category>.cpp`
- 函数名 `rt_<symbolic_name>`（与 annotation 中的 symbolic_name 一致，加 `rt_` 前缀）
- 一个文件内的函数按逻辑名字母序排列

### 2.1.5 完整导出函数清单

通过逐一分析 `src/executable/bytecode/instructions/` 中所有指令的 `execute()` 实现，提取完整的 runtime API 需求。按里程碑分级：

#### Tier 0: print("hello world") 最小集

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `init` | lifecycle | — | `ArenaManager::initialize` + `initialize_types` + builtins 注册 | `rt_lifecycle.cpp` |
| `none` | singleton | `LoadConst(None)` | `py_none()` | `rt_singleton.cpp` |
| `true` | singleton | `LoadConst(True)` | `py_true()` | `rt_singleton.cpp` |
| `false` | singleton | `LoadConst(False)` | `py_false()` | `rt_singleton.cpp` |
| `string_from_cstr` | create | `LoadConst("...")` | `PyString::create(str)` → unwrap | `rt_create.cpp` |
| `call` | func | `FunctionCall` | `obj->call(args, kwargs)` → unwrap | `rt_func.cpp` |
| `getattr` | attr | `LoadAttr` | `obj->getattribute(name)` → unwrap | `rt_attr.cpp` |
| `load_global` | attr | `LoadGlobal` | `module->getattribute(name)` → unwrap | `rt_attr.cpp` |
| `import` | module | `ImportName` | `ModuleRegistry::find` | `rt_module.cpp` |
| `build_tuple` | create | `BuildTuple` | `PyTuple::create(...)` → unwrap | `rt_create.cpp` |
| `raise` | error | `RaiseVarargs` | `abort` (Phase 2 简化) | `rt_error.cpp` |

#### Tier 1: 表达式与赋值

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `integer_from_i64` | create | `LoadConst(42)` | `PyInteger::create(int64_t)` → unwrap | `rt_create.cpp` |
| `float_from_f64` | create | `LoadConst(3.14)` | `PyFloat::create(double)` → unwrap | `rt_create.cpp` |
| `binary_add` | op | `BinaryOperation(PLUS)` | `obj->add(other)` → unwrap | `rt_op.cpp` |
| `binary_sub` | op | `BinaryOperation(MINUS)` | `obj->subtract(other)` → unwrap | `rt_op.cpp` |
| `binary_mul` | op | `BinaryOperation(MULTIPLY)` | `obj->multiply(other)` → unwrap | `rt_op.cpp` |
| `binary_truediv` | op | `BinaryOperation(SLASH)` | `obj->truediv(other)` → unwrap | `rt_op.cpp` |
| `binary_floordiv` | op | `BinaryOperation(FLOORDIV)` | `obj->floordiv(other)` → unwrap | `rt_op.cpp` |
| `binary_mod` | op | `BinaryOperation(MODULO)` | `obj->modulo(other)` → unwrap | `rt_op.cpp` |
| `binary_pow` | op | `BinaryOperation(EXP)` | `obj->pow(other, none)` → unwrap | `rt_op.cpp` |
| `binary_lshift` | op | `BinaryOperation(LEFTSHIFT)` | `obj->lshift(other)` → unwrap | `rt_op.cpp` |
| `binary_rshift` | op | `BinaryOperation(RIGHTSHIFT)` | `obj->rshift(other)` → unwrap | `rt_op.cpp` |
| `binary_and` | op | `BinaryOperation(AND)` | `obj->and_(other)` → unwrap | `rt_op.cpp` |
| `binary_or` | op | `BinaryOperation(OR)` | `obj->or_(other)` → unwrap | `rt_op.cpp` |
| `binary_xor` | op | `BinaryOperation(XOR)` | `obj->xor_(other)` → unwrap | `rt_op.cpp` |
| `unary_neg` | op | `Unary(SUB)` | `obj->neg()` → unwrap | `rt_op.cpp` |
| `unary_pos` | op | `Unary(ADD)` | `obj->pos()` → unwrap | `rt_op.cpp` |
| `unary_invert` | op | `Unary(INVERT)` | `obj->invert()` → unwrap | `rt_op.cpp` |
| `unary_not` | op | `Unary(NOT)` | `!obj->true_()` → PyBool | `rt_op.cpp` |
| `is_true` | convert | `JumpIfFalse` / `JumpIfTrue` | `obj->true_()` → unwrap | `rt_convert.cpp` |
| `store_global` | attr | `StoreGlobal` | `module->insert(name, value)` | `rt_attr.cpp` |
| `store_name` | attr | `StoreName` | `ns->insert(name, value)` | `rt_attr.cpp` |
| `load_name` | attr | `LoadName` | local→global→builtins 查找 | `rt_attr.cpp` |

#### Tier 2: 比较与控制流

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `compare_eq` | cmp | `CompareOperation(EQ)` | `obj->richcompare(other, Py_EQ)` → unwrap | `rt_cmp.cpp` |
| `compare_ne` | cmp | `CompareOperation(NE)` | `obj->richcompare(other, Py_NE)` → unwrap | `rt_cmp.cpp` |
| `compare_lt` | cmp | `CompareOperation(LT)` | `obj->richcompare(other, Py_LT)` → unwrap | `rt_cmp.cpp` |
| `compare_le` | cmp | `CompareOperation(LE)` | `obj->richcompare(other, Py_LE)` → unwrap | `rt_cmp.cpp` |
| `compare_gt` | cmp | `CompareOperation(GT)` | `obj->richcompare(other, Py_GT)` → unwrap | `rt_cmp.cpp` |
| `compare_ge` | cmp | `CompareOperation(GE)` | `obj->richcompare(other, Py_GE)` → unwrap | `rt_cmp.cpp` |
| `compare_is` | cmp | `CompareOperation(IS)` | 指针比较 → PyBool | `rt_cmp.cpp` |
| `compare_is_not` | cmp | `CompareOperation(IS_NOT)` | 指针比较 → PyBool | `rt_cmp.cpp` |
| `compare_in` | cmp | `CompareOperation(IN)` | `obj->contains(value)` → PyBool | `rt_cmp.cpp` |
| `compare_not_in` | cmp | `CompareOperation(NOT_IN)` | `!obj->contains(value)` → PyBool | `rt_cmp.cpp` |
| `get_iter` | subscr | `GetIter` | `obj->iter()` → unwrap | `rt_subscr.cpp` |
| `iter_next` | subscr | `ForIter` | `iter->next()` + StopIteration 检查 | `rt_subscr.cpp` |

#### Tier 3: 容器与下标

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `build_list` | create | `BuildList` | `PyList::create` + append | `rt_create.cpp` |
| `build_dict` | create | `BuildDict` | `PyDict::create` + insert | `rt_create.cpp` |
| `build_set` | create | `BuildSet` | `PySet::create` + add | `rt_create.cpp` |
| `build_string` | create | `BuildString` | 多段字符串拼接 | `rt_create.cpp` |
| `build_slice` | create | `BuildSlice` | `PySlice::create(start, stop, step)` | `rt_create.cpp` |
| `getitem` | subscr | `BinarySubscript(LOAD)` | `obj->getitem(key)` → unwrap | `rt_subscr.cpp` |
| `setitem` | subscr | `StoreSubscript` | `obj->setitem(key, value)` → unwrap | `rt_subscr.cpp` |
| `delitem` | subscr | `DeleteSubscript` | `obj->delitem(key)` → unwrap | `rt_subscr.cpp` |
| `list_append` | subscr | `ListAppend` | `list->append(value)` | `rt_subscr.cpp` |
| `list_extend` | subscr | `ListExtend` | `list->extend(other)` | `rt_subscr.cpp` |
| `dict_merge` | subscr | `DictMerge` | `dict->merge(other)` | `rt_subscr.cpp` |
| `dict_update` | subscr | `DictUpdate` | `dict->update(other)` | `rt_subscr.cpp` |
| `set_add` | subscr | `SetAdd` | `set->add(value)` | `rt_subscr.cpp` |
| `unpack_sequence` | subscr | `UnpackSequence` | iter + next × n | `rt_subscr.cpp` |

#### Tier 4: 函数与闭包

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `make_function` | func | `MakeFunction` | `PyFunction::create(...)` → unwrap | `rt_func.cpp` |
| `call_with_kwargs` | func | `FunctionCallWithKeywords` | `obj->call(args, kwargs)` → unwrap | `rt_func.cpp` |
| `call_ex` | func | `FunctionCallEx` | `obj->call(*args, **kwargs)` → unwrap | `rt_func.cpp` |
| `create_cell` | func | — | `PyCell::create()` → unwrap | `rt_func.cpp` |
| `cell_get` | func | `LoadDeref` | `cell->get()` | `rt_func.cpp` |
| `cell_set` | func | `StoreDeref` | `cell->set(value)` | `rt_func.cpp` |
| `get_closure` | func | — | `func->closure()` | `rt_func.cpp` |
| `load_method` | func | `LoadMethod` | `obj->get_method(name)` → unwrap | `rt_func.cpp` |
| `method_call` | func | `MethodCall` | 优化方法调用 | `rt_func.cpp` |

#### Tier 5: 类与继承

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `load_build_class` | class | `LoadBuildClass` | builtins `__build_class__` 查找 | `rt_class.cpp` |
| `setattr` | attr | `StoreAttr` | `obj->setattribute(name, value)` → unwrap | `rt_attr.cpp` |
| `delattr` | attr | `DeleteAttr` | `obj->delattribute(name)` → unwrap | `rt_attr.cpp` |

#### Tier 6: 异常处理

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `raise` | error | `RaiseVarargs` | abort (Phase 2) / longjmp (Phase 4+) | `rt_error.cpp` |
| `reraise` | error | `ReRaise` | 重新抛出当前异常 | `rt_error.cpp` |
| `check_exception_match` | error | `JumpIfExceptionMatch` | `isinstance(exc, type)` | `rt_error.cpp` |
| `load_assertion_error` | error | `LoadAssertionError` | 获取 AssertionError 类型 | `rt_error.cpp` |

#### Tier 7: 增强赋值与其他

| 逻辑名 | 分类 | 对应 bytecode 指令 | 包装来源 | 所在文件 |
|:---|:---|:---|:---|:---|
| `inplace_add` | op | `InplaceOp(PLUS)` | `a += b` | `rt_op.cpp` |
| `inplace_sub` | op | `InplaceOp(MINUS)` | `a -= b` | `rt_op.cpp` |
| `inplace_mul` | op | `InplaceOp(MULTIPLY)` | `a *= b` | `rt_op.cpp` |
| ... | op | `InplaceOp(...)` | 其余增强赋值同理 | `rt_op.cpp` |
| `format_value` | create | `FormatValue` | f-string 值格式化 | `rt_create.cpp` |
| `import_from` | module | `ImportFrom` | `from mod import name` | `rt_module.cpp` |
| `import_star` | module | `ImportStar` | `from mod import *` | `rt_module.cpp` |
| `list_to_tuple` | convert | `ListToTuple` | list → tuple 转换 | `rt_convert.cpp` |
| `yield_value` | func | `YieldValue` | 生成器 yield | `rt_func.cpp` |
| `yield_from` | func | `YieldFrom` | 生成器 yield from | `rt_func.cpp` |
| `get_awaitable` | func | `GetAwaitable` | async/await | `rt_func.cpp` |

### 2.1.6 RuntimeContext 与导出层的关系

当前 runtime 内部的 `RuntimeContext` 维护了 `Interpreter*`、`globals_provider`、`truthy_provider` 等回调。这些是 **VM 解释器的内部机制**，编译后的代码不使用 `RuntimeContext` 的 provider 链。

导出层的包装函数直接调用 `PyObject` 的公开 API，绕过 `RuntimeContext` 的间接层：

| 操作 | VM 路径 (通过 RuntimeContext) | 导出层路径 (直接调用) |
|:---|:---|:---|
| 判断真值 | `RuntimeContext::is_true()` → `truthy_provider` → `obj->true_()` | `obj->true_()` |
| 相等比较 | `RuntimeContext::equals()` → `equals_provider` → `obj->richcompare()` | `obj->richcompare(other, Py_EQ)` |
| 全局变量 | `RuntimeContext::current_globals()` → `globals_provider` → `PyFrame::globals()` | 编译器直接持有 `PyModule*` 指针 |

这意味着编译后的代码在运行时**不需要初始化 `RuntimeContext`**。`rt_init()` 只需初始化 Arena、类型系统和内置模块。

### 2.1.7 错误处理策略

Runtime 内部使用 `PyResult<T>` 表示可失败操作。导出层负责将其转换为前端可理解的约定：

**Phase 2 策略（简化版）**：包装函数内部对 `PyResult` 调用 `is_err()` 检查。如果是错误，调用 `rt_raise` 打印 traceback 后 `abort()`。返回值为 `nullptr` 表示错误（但实际上 abort 后不会返回）。

**Phase 4+ 策略**：引入异常传播机制。每个包装函数返回后，编译器插入 null 检查：
```
%result = call ptr @rt_binary_add(%a, %b)
%is_err = icmp eq ptr %result, null
br i1 %is_err, label %exception, label %continue
```
`%exception` 块跳转到当前活跃的异常处理器（对应 `try/except` 的 handler block），或向上传播（函数返回 null）。这等价于 bytecode VM 中 `SetupExceptionHandling` + cleanup stack 的静态展开。

### 2.1.8 Bitcode 生成

Runtime 的所有 `.cpp` 文件（包括 `src/runtime/export/*.cpp`）通过 Clang 编译为 LLVM Bitcode。CMake 中新增 `runtime_bitcode` 目标。

关键编译选项：
- `-emit-llvm -c` — 输出 bitcode 而非 object
- `-O2` — 启用优化，使内联更有效
- `-DPYLANG_USE_ARENA` — 保持 Arena 内存模型
- 需要包含所有 runtime 依赖的头文件路径

由于 runtime 包含大量 `.cpp` 文件，每个单独编译为 `.bc`，最后通过 `llvm-link` 合并为单个 `runtime.bc`。export/ 目录下的 `.cpp` 是其中的一部分，annotation 会自然嵌入到合并后的 bitcode 中。

---

## 3. Phase 2.2: RuntimeLinker — Annotation 驱动的服务发现

### 3.1 定位与职责

RuntimeLinker 不仅是"加载 .bc 的工具"，而是**编译器前端与 runtime 之间的唯一契约层**。它提供以下服务：

1. **加载 runtime.bc** — 解析 LLVM Bitcode 为 `llvm::Module`
2. **扫描 annotation** — 遍历 `llvm.global.annotations`，解析 `pylang_export:` 前缀的元数据
3. **建立索引** — 构建多级映射：`category → symbolic_name → FuncInfo`
4. **声明注入** — 按需在用户 Module 中创建 runtime 函数的声明（lazy）
5. **模块链接** — 将 runtime Module 链接到用户 Module（支持 LTO 内联）
6. **类型查询** — 提供 `PyObject*` 的 LLVM 类型（opaque pointer）

### 3.2 Annotation 扫描流程

LLVM Bitcode 中，`__attribute__((annotate("...")))` 标记的函数会在 `@llvm.global.annotations` 全局数组中生成条目。每个条目是 `ConstantStruct`，含 4 个字段：

| 字段 | 内容 | 用途 |
|:---|:---|:---|
| `element[0]` | 被标注函数的指针（可能经过 bitcast） | `stripPointerCasts()` 恢复 `llvm::Function*` |
| `element[1]` | annotation 字符串的 `GlobalVariable*` | 提取 initializer 得到字符串内容 |
| `element[2]` | 源文件名字符串指针 | 忽略 |
| `element[3]` | 行号 `i32` | 忽略 |

RuntimeLinker 的扫描流程：

1. 找到 `@llvm.global.annotations` 全局变量
2. 遍历其 `ConstantArray` 的每个元素
3. 从 `element[1]` 的 `GlobalVariable` initializer 中提取 `ConstantDataArray`，得到 annotation 字符串
4. 匹配 `pylang_export:` 前缀
5. 按 `:` 分割为恰好 5 个字段
6. 提取 `category`(field[1]), `symbolic_name`(field[2]), `ret_type`(field[3]), `param_types`(field[4])
7. 从 `element[0]` 恢复 `llvm::Function*`（`dyn_cast<Function>(element[0]->stripPointerCasts())`）
8. 存入索引：`symbolic_name` → `{ Function*, FunctionType*, category, ret, params }`

### 3.3 前端使用方式

IREmitter 通过 RuntimeLinker 按逻辑名获取函数。RuntimeLinker 的 `get` 方法是 lazy 的：首次对某个逻辑名的请求会在用户 Module 中创建对应的 `declare` 声明，后续请求复用已创建的声明。

### 3.4 按 category 的批量查询

RuntimeLinker 提供 `list_by_category(category)` 接口，用于：
- 编译器诊断：检查某个 `op` 是否有对应的 runtime 实现
- 验证：确保所有 bytecode 指令都有对应的 runtime 导出

### 3.5 与 export.hpp 的契约

- annotation 字符串必须以 `pylang_export:` 开头
- 按 `:` 分割后恰好 5 个字段
- 字段顺序固定为 `prefix:category:name:ret:params`
- `params` 中多个参数以 `,` 分隔；无参数为空串
- 逻辑名全局唯一（跨 category 不重复）
- RuntimeLinker 在扫描时验证格式，不合格的 annotation 产生编译期 warning
- params 数量应与 `FunctionType->getNumParams()` 一致，不一致时产生 warning

---

## 4. Phase 2.3: PylangCodegen — AST → LLVM IR

### 4.1 核心思路

**参考 bytecode 后端的指令实现**，将每个 AST 节点映射为对 runtime 导出函数的调用。

现有 `src/executable/bytecode/instructions/` 中 80+ 条指令的 `execute()` 方法精确展示了每个 Python 操作需要的 runtime 调用序列。PylangCodegen 做的事情等价于"将 VM 的 eval_loop 静态展开为 LLVM IR"。

### 4.2 指令映射对照

| Bytecode 指令 | Runtime 逻辑名 | 生成的 LLVM IR |
|:---|:---|:---|
| `LoadConst(42)` | `integer_from_i64` | `%v = call @rt_integer_from_i64(i64 42)` |
| `LoadConst("hello")` | `string_from_cstr` | `%v = call @rt_string_from_cstr(ptr @.str, i64 5)` |
| `BinaryOperation(+)` | `binary_add` | `%v = call @rt_binary_add(%a, %b)` |
| `CompareOperation(==)` | `compare_eq` | `%v = call @rt_compare_eq(%a, %b)` |
| `FunctionCall(f, args)` | `call` | `%v = call @rt_call(%f, %args, null)` |
| `LoadGlobal("x")` | `load_global` | `%v = call @rt_load_global(%mod, ptr @.str.x)` |
| `StoreGlobal("x")` | `store_global` | `call @rt_store_global(%mod, ptr @.str.x, %v)` |
| `JumpIfFalse(label)` | `is_true` | `%c = call @rt_is_true(%v); br i1 %c, ...` |
| `GetIter(obj)` | `get_iter` | `%it = call @rt_get_iter(%obj)` |
| `ForIter(iter)` | `iter_next` | `%v = call @rt_iter_next(%it, %flag)` |
| `BuildList(n)` | `build_list` | `%l = call @rt_build_list(i32 n, ptr %arr)` |
| `MakeFunction(name)` | `make_function` | `%f = call @rt_make_function(...)` |
| `LoadBuildClass` | `load_build_class` | `%bc = call @rt_load_build_class()` |
| `LoadDeref(idx)` | `cell_get` | `%v = call @rt_cell_get(%cell)` |
| `StoreDeref(idx)` | `cell_set` | `call @rt_cell_set(%cell, %v)` |
| `UnpackSequence(n)` | `unpack_sequence` | `call @rt_unpack_sequence(%obj, i32 n, ptr %out)` |
| `Return(value)` | — | `ret ptr %value` |

### 4.3 目录结构

```
src/compiler/
├── CMakeLists.txt
├── RuntimeLinker.hpp / .cpp       ← annotation 扫描 + 函数索引
├── codegen/
│   ├── PylangCodegen.hpp / .cpp   ← 核心: AST Visitor → LLVM IR
│   ├── CodegenContext.hpp         ← scope 栈、变量表、当前函数
│   ├── IREmitter.hpp / .cpp       ← 封装: 逻辑名 → runtime call
│   └── tests/
└── driver/
    ├── CompilerDriver.hpp / .cpp  ← AOT 编译入口
    └── tests/
```

### 4.4 三层架构

```
PylangCodegen (AST Visitor)
    │  语义层: 知道 Python 语义（作用域、控制流、函数定义）
    │  调用 IREmitter 的高层接口
    ▼
IREmitter (Runtime Call 生成器)
    │  中间层: 知道 runtime 函数的逻辑名和参数约定
    │  调用 RuntimeLinker 获取函数声明
    ▼
RuntimeLinker (Annotation 索引)
    │  底层: 只知道 annotation 格式和 llvm::Function*
    │  不知道 Python 语义
    ▼
runtime.bc (LLVM Bitcode)
    │  其中 src/runtime/export/*.cpp 的 annotation
    │  嵌入在 @llvm.global.annotations 中
    ▼
src/runtime/export/ (导出层)
    │  统一的包装函数，标注 PYLANG_EXPORT_* 宏
    │  展平成员调用 + 解包 PyResult
    ▼
src/runtime/ (内部实现)
    PyObject::add(), PyInteger::create(), py_true(), ...
```

### 4.5 变量管理详细设计

#### 4.5.1 变量分类

PylangCodegen 复用 `VariablesResolver` 的分析结果。`VariablesResolver::resolve(ast::Module*)` 返回每个 scope 中每个变量的 `Visibility`，直接决定 IR 中的存取方式：

| Visibility | bytecode 指令 | LLVM IR 实现 |
|:---|:---|:---|
| `IMPLICIT_GLOBAL` | `LoadGlobal`/`StoreGlobal` | `call @rt_load_global(%module, "name")` / `call @rt_store_global(...)` |
| `EXPLICIT_GLOBAL` | `LoadGlobal`/`StoreGlobal` | 同上（`global x` 声明强制为全局） |
| `NAME` | `LoadName`/`StoreName` | `call @rt_load_name(%frame_ns, "name")`（按 local→global→builtins 查找） |
| `LOCAL` | `LoadFast`/`StoreFast` | `alloca ptr` 在函数入口 + `load`/`store`。LLVM `mem2reg` 自动提升为 SSA。 |
| `CELL` | `LoadDeref`/`StoreDeref` | 通过 cell 数组间接访问。cell 是堆分配的盒子，存放变量指针。 |
| `FREE` | `LoadDeref`/`StoreDeref` | 同 CELL，但 cell 由外层函数创建并通过闭包传入。 |
| `HIDDEN` | `LoadName`/`StoreName` | 类作用域中的赋值，不能被内层函数直接引用。 |

#### 4.5.2 局部变量初始化

对于 `LOCAL` 变量，PylangCodegen 在函数入口为**所有局部变量**生成 `alloca`，初始值为 `null`（表示"未初始化"）。这与 bytecode VM 中 `PyFrame` 预分配 `stack_locals` 数组并填充 `nullptr` 等价。

```
define ptr @pylang_func_foo(ptr %module, ptr %args, ptr %kwargs) {
entry:
    %x = alloca ptr          ; LOCAL 变量 x
    store ptr null, ptr %x   ; 初始化为 null（未赋值）
    ; ... 函数体 ...
}
```

运行时，`LoadFast` 对应的 `load` 后应检查 null（等价于 bytecode VM 的 `UnboundLocalError` 检查）：

```
    %x.val = load ptr, ptr %x
    %is_unbound = icmp eq ptr %x.val, null
    br i1 %is_unbound, label %unbound_error, label %continue
```

Phase 2 简化：省略 unbound 检查，假设程序正确。Phase 3 加入检查。

#### 4.5.3 闭包变量（Cell / Free）

闭包是 Python 中最复杂的变量机制之一。通过分析 `MakeFunction` 和 `LoadClosure`/`LoadDeref`/`StoreDeref` 指令：

**Cell 变量**（外层函数中被内层函数引用的变量）：

1. 外层函数在入口为每个 cell 变量分配一个 **cell 对象**（堆分配的单元素盒子）
2. 所有对该变量的读写通过 cell 间接进行
3. `LoadClosure` 将 cell 对象打包为 tuple，传给 `MakeFunction`

**Free 变量**（内层函数中引用外层变量）：

1. 内层函数接收 closure tuple（`PyFunction.__closure__`）
2. `LoadDeref` 从 closure tuple 中取出 cell，再从 cell 中取值
3. `StoreDeref` 取出 cell，更新 cell 中的值

LLVM IR 实现方案：

```
; 外层函数
define ptr @outer(ptr %module, ...) {
    ; 为 cell 变量 x 分配 cell 对象
    %cell_x = call ptr @rt_create_cell()
    ; 赋值 x = 42
    %val = call ptr @rt_integer_from_i64(i64 42)
    call void @rt_cell_set(%cell_x, %val)
    ; 创建 closure tuple
    %closure = call ptr @rt_build_tuple(i32 1, ptr %cells_arr)
    ; 创建内层函数
    %inner = call ptr @rt_make_function(%code, %module, ..., %closure)
    ret ptr %inner
}

; 内层函数
define ptr @inner(ptr %module, ptr %args, ptr %kwargs) {
    ; %closure 从 PyFunction 中提取
    %closure = call ptr @rt_get_closure(%self)
    ; 读取 free 变量 x
    %cell_x = call ptr @rt_tuple_getitem(%closure, i32 0)
    %x_val = call ptr @rt_cell_get(%cell_x)
    ; ...
}
```

需要新增的 runtime 导出：

| 逻辑名 | 分类 | 说明 |
|:---|:---|:---|
| `create_cell` | func | 创建空 cell 对象 |
| `cell_get` | func | 从 cell 读取值 |
| `cell_set` | func | 向 cell 写入值 |
| `get_closure` | func | 从 PyFunction 获取 closure tuple |

#### 4.5.4 默认参数值

函数默认值在**定义时**求值（不是调用时）。参考 `BytecodeGenerator::visit(FunctionDefinition*)`：

1. 编译器在定义点生成默认值表达式的 IR
2. 将默认值收集到数组中
3. 传递给 `rt_make_function`，存入 `PyFunction.__defaults__` / `__kwdefaults__`

```
; def foo(a, b=10, c=20):
%def_b = call ptr @rt_integer_from_i64(i64 10)
%def_c = call ptr @rt_integer_from_i64(i64 20)
; defaults = (10, 20)
%defaults = call ptr @rt_build_tuple(i32 2, ptr %defs_arr)
%foo = call ptr @rt_make_function(%code, %module, %defaults, ptr null, ptr null)
```

#### 4.5.5 解包赋值

`a, b, c = expr` 对应 `UnpackSequence` 指令。runtime 需要：

1. 对目标对象调用 `iter()`
2. 依次 `next()` 取出 n 个值
3. 检查是否恰好 n 个（多了或少了都是 ValueError）

```
; a, b = [1, 2]
%packed = ...
call void @rt_unpack_sequence(%packed, i32 2, ptr %out_arr)
%a = load ptr, ptr getelementptr([2 x ptr], ptr %out_arr, i32 0, i32 0)
%b = load ptr, ptr getelementptr([2 x ptr], ptr %out_arr, i32 0, i32 1)
```

#### 4.5.6 `global` / `nonlocal` 声明

这些声明**不生成任何 IR**——它们仅影响 `VariablesResolver` 的分析结果：

- `global x`：强制 `x` 的 Visibility 为 `EXPLICIT_GLOBAL`，即使在函数内赋值也走 `StoreGlobal`
- `nonlocal x`：强制 `x` 的 Visibility 为 `FREE`，触发 cell/free 变量链

PylangCodegen 只需在查阅 `VariablesResolver` 结果时正确分派即可，无需额外处理。

---

## 5. 类型系统策略

### 5.1 现状分析：bytecode VM 是 100% 动态类型

通过分析现有 runtime 代码，类型分派路径为：

```
BinaryOperation::execute()
  → py::add(Value lhs, Value rhs, Interpreter&)
    → std::visit(variant dispatch):
        Number + Number → Number 运算
        PyObject* + PyObject* → obj->add(other)
          → type_prototype().__add__ slot 查找
            → std::function<PyResult<PyObject*>(const PyObject*, const PyObject*)>
              → 具体类型的实现（如 PyInteger::__add__）
```

**关键观察**：

1. `Value` 是 `std::variant<Number, String, Bytes, Ellipsis, NameConstant, Tuple, PyObject*>`
2. 第一层分派：`std::visit` 在 variant 的类型索引上跳转（编译期确定）
3. 第二层分派：`TypePrototype` 的 slot 是 `std::function`（运行时虚函数表）
4. **整个过程中没有任何类型推导或类型特化**

### 5.2 设计决策：Phase 2 保持全动态，类型分派交给 Runtime

**Phase 2 策略：所有操作都通过通用 runtime 入口点**

与 bytecode VM 完全一致。`rt_binary_add(a, b)` 内部走 `obj->add(other)` → slot 分派。编译器不做任何类型推导。

理由：
- 语义正确性优先——Python 是动态类型语言，任何变量可以在运行时改变类型
- 与 bytecode VM 行为一致，便于验证
- 实现简单，关键路径上没有额外复杂度

```
; x = a + b  （编译器不知道 a, b 的类型）
%a = call ptr @rt_load_global(%mod, "a")
%b = call ptr @rt_load_global(%mod, "b")
%x = call ptr @rt_binary_add(%a, %b)    ; ← runtime 内部做类型分派
call void @rt_store_global(%mod, "x", %x)
```

### 5.3 Phase 3+ 优化策略：前端类型特化

未来可在 PylangCodegen 中引入**推测性类型特化**（Speculative Type Specialization），类似 CPython 3.11 的 Adaptive Interpreter (PEP 659)：

**策略一：常量折叠 + 类型传播**

对于编译期可确定类型的表达式，直接生成特化调用：

```python
x = 1 + 2     # 编译器知道两边都是 int
y = "a" + "b"  # 编译器知道两边都是 str
```

生成：
```
%x = call ptr @rt_integer_add_i64(i64 1, i64 2)  ; 特化版本，跳过 slot 分派
%y = call ptr @rt_string_concat("a", "b")         ; 特化版本
```

需要 runtime 导出特化版本的 API（`Tier_opt`）：

| 逻辑名 | 分类 | 说明 |
|:---|:---|:---|
| `integer_add_i64` | op_spec | 两个 PyInteger 的快速加法 |
| `integer_sub_i64` | op_spec | 两个 PyInteger 的快速减法 |
| `string_concat` | op_spec | 两个 PyString 的拼接 |
| `float_add_f64` | op_spec | 两个 PyFloat 的快速加法 |

**策略二：Guard + Deoptimization**

对于无法静态确定但"高概率"是某类型的操作，生成带类型检查的快速路径：

```
%a_type = call ptr @rt_type_of(%a)
%is_int = icmp eq ptr %a_type, @PyInteger_type
br i1 %is_int, label %fast, label %slow

fast:
    %result = call ptr @rt_integer_add_fast(%a, %b)
    br label %merge

slow:
    %result2 = call ptr @rt_binary_add(%a, %b)    ; 通用路径
    br label %merge

merge:
    %x = phi ptr [%result, %fast], [%result2, %slow]
```

**策略三：Profile-Guided Optimization (PGO)**

1. 第一次编译使用全动态路径 + 类型计数 instrumentation
2. 运行收集热点的类型 profile
3. 第二次编译基于 profile 生成特化代码

**Phase 2 不实现任何类型特化**。所有优化推迟到 Phase 3+，且以独立 pass 形式添加，不影响 Phase 2 的架构。

### 5.4 与 LLVM 优化的协同

即使 PylangCodegen 不做类型推导，LLVM 的 IPO 优化在链接 `runtime.bc` 后仍然有效：

1. **函数内联**：`rt_integer_from_i64` 等小函数被内联到调用处
2. **常量传播**：内联后 `PyInteger::create(42)` 的分支可被消除



3. **死代码消除**：内联后未走的类型分派路径被删除

这意味着"编译器不做类型推导 + LLVM O2 内联"在很多情况下可以达到接近手工特化的效果。

---

## 6. 类与继承设计

### 6.1 Python 类创建的底层机制

通过分析 `BytecodeGenerator::visit(ClassDefinition*)` 和 `LoadBuildClass` 指令，类创建的完整流程为：

```python
class Foo(Base1, Base2, metaclass=Meta):
    x = 1
    def method(self):
        pass
```

bytecode 编译为：

```
1. LoadBuildClass          → 获取 builtins.__build_class__
2. MakeFunction("Foo")     → 将类体编译为函数对象 (body_func)
3. Push "Foo"              → 类名
4. Push Base1, Base2       → 基类
5. Push metaclass=Meta     → 关键字参数
6. FunctionCallWithKeywords(__build_class__, body_func, "Foo", Base1, Base2, metaclass=Meta)
```

`__build_class__` 内部执行：

```
1. 确定 metaclass（默认为 type）
2. 调用 metaclass.__prepare__(name, bases) → 获取 namespace dict
3. 执行 body_func(namespace)   → 填充 namespace（x=1, method=<function>）
4. 调用 metaclass(name, bases, namespace) → 创建类对象
   → type.__new__(meta, name, bases, ns)
   → type.__init__(cls, name, bases, ns)
```

### 6.2 LLVM IR 中的类创建

PylangCodegen 不需要理解元类协议的细节——这些全部在 runtime 中实现。编译器只需生成等价于上述 bytecode 序列的调用：

```
; class Foo(Bar):
;     x = 1
;     def method(self): pass

; Step 1: 获取 __build_class__
%build_class = call ptr @rt_load_build_class()

; Step 2: 编译类体为函数（类体函数在编译时已生成为独立的 LLVM Function）
%body_code = @pylang_class_body_Foo   ; 编译器生成的类体函数指针
%body_func = call ptr @rt_make_function(%body_code, %module, ...)

; Step 3: 构造参数
%name = call ptr @rt_string_from_cstr("Foo")
%bar = call ptr @rt_load_global(%module, "Bar")
%args = call ptr @rt_build_tuple(i32 3, ptr [%body_func, %name, %bar])

; Step 4: 调用 __build_class__
%Foo = call ptr @rt_call(%build_class, %args, ptr null)

; Step 5: 将类对象存入模块
call void @rt_store_global(%module, "Foo", %Foo)
```

**类体函数**的编译与普通函数相同，但其作用域类型为 `Scope::Type::CLASS`：

```
; 类体函数（自动接收 namespace dict 作为 __locals__）
define ptr @pylang_class_body_Foo(ptr %module, ptr %args, ptr %kwargs) {
    ; 类体函数的 locals 是 namespace dict
    %ns = ... ; 从参数中提取

    ; x = 1
    %one = call ptr @rt_integer_from_i64(i64 1)
    call void @rt_dict_setitem(%ns, "x", %one)

    ; def method(self): pass
    %method = call ptr @rt_make_function(@pylang_func_Foo_method, ...)
    call void @rt_dict_setitem(%ns, "method", %method)

    ret ptr @rt_none()
}
```

### 6.3 类体中的变量可见性

`VariablesResolver` 对类作用域有特殊处理：

- 类体中的赋值标记为 `HIDDEN`——不能被内层函数通过闭包引用
- 类体中自动有 `__name__`（LOAD）、`__module__`（STORE）、`__qualname__`（STORE）
- `__class__` 被标记为 `CELL`，支持 `super()` 的零参数形式

PylangCodegen 对 `HIDDEN` 变量使用 `StoreName`（写入 namespace dict），对 `CELL`（`__class__`）使用 cell 机制。

### 6.4 继承与 MRO

**编译器不处理继承和 MRO**。这些完全是 runtime 的职责：

- `type.__new__` 内部调用 `PyType::initialize()` → `ready()` → 计算 MRO
- 属性查找通过 `PyType::lookup()` 沿 MRO 搜索
- `super()` 通过 `__class__` cell 变量实现

编译器只需确保：
1. 基类在调用 `__build_class__` 时作为参数传入
2. `__class__` cell 在类体函数中正确设置

### 6.5 方法调用优化

bytecode VM 有 `LoadMethod` + `MethodCall` 优化对，避免每次方法调用都创建 bound method 对象。PylangCodegen 可以复用这个优化：

```
; obj.method(args)

; 非优化路径：
%method = call ptr @rt_getattr(%obj, "method")   ; 创建 bound method
%result = call ptr @rt_call(%method, %args, null) ; 调用

; 优化路径：
%method_or_self = call ptr @rt_load_method(%obj, "method") ; 返回 unbound + self
%result = call ptr @rt_method_call(%method_or_self, %obj, %args, null)
```

Phase 2 使用非优化路径。Phase 3 引入 `LoadMethod`/`MethodCall` 优化。

### 6.6 分阶段实施

| 阶段 | 支持的类特性 | 依赖的 runtime 导出 |
|:---|:---|:---|
| Phase 2.5 | 无类支持 | — |
| Phase 3.1 | 简单类定义（无继承） | `load_build_class`, `make_function`, `store_name` |
| Phase 3.2 | 单继承 | 同上 + 基类作为 `__build_class__` 参数 |
| Phase 3.3 | 多继承 + `super()` | 同上 + `create_cell`, `cell_set`/`cell_get` |
| Phase 3.4 | 元类 | 同上 + `call_with_kwargs` |
| Phase 3.5 | `@property`/`@classmethod`/`@staticmethod` | 装饰器 = 普通函数调用 |

---

## 7. Phase 2.4: AOT Driver

### 7.1 编译流程

```
pylangc hello.py -o hello

Step 1: hello.py → Lexer → Parser → AST
Step 2: AST → VariablesResolver → Scope/Visibility 分析
Step 3: AST + Visibility → PylangCodegen → hello_user.bc (LLVM IR)
Step 4: hello_user.bc + runtime.bc → llvm::Linker → merged.bc
Step 5: merged.bc → LLVM PassManager (O2) → optimized.bc
Step 6: optimized.bc → LLVM TargetMachine → hello.o
Step 7: hello.o → system linker (ld/lld) → hello
```

注意 Step 2：`VariablesResolver` 在 codegen 之前运行，产出的 `VisibilityMap` 传递给 `PylangCodegen`。这与 bytecode 后端的流程一致（`BytecodeGenerator::compile()` 内部先调用 `VariablesResolver::resolve()`）。

### 7.2 main() 入口生成

```
define i32 @main(i32 %argc, ptr %argv) {
    call void @rt_init()            ; Arena + types + builtins
    call void @PyInit_<module>()    ; 模块初始化（执行模块体）
    ret i32 0
}
```

### 7.3 命令行接口

```
pylangc <input.py> [options]

Options:
  -o <file>         输出文件名（默认 a.out）
  --emit-llvm       输出 LLVM IR 文本（调试用）
  --emit-bc         输出 LLVM Bitcode
  -c                只编译不链接，输出 .o
  -O0/-O1/-O2/-O3   优化级别
  --runtime-bc <path>   指定 runtime.bc 路径
  --hidden-import <mod> 强制编译并链接指定模块
```

---

## 8. Phase 2.5: 里程碑验证 — print("hello world")

### 8.1 目标程序

```python
print("Hello, World!")
```

### 8.2 编译器生成的 IR（伪代码）

```
@.str.hello = private constant [14 x i8] c"Hello, World!\00"
@.str.print = private constant [6 x i8] c"print\00"
@.str.builtins = private constant [9 x i8] c"builtins\00"

define void @PyInit_hello() {
    %builtins = call ptr @rt_import(ptr @.str.builtins)
    %print_fn = call ptr @rt_getattr(ptr %builtins, ptr @.str.print)
    %hello = call ptr @rt_string_from_cstr(ptr @.str.hello, i64 13)
    %args_arr = alloca [1 x ptr]
    store ptr %hello, ptr %args_arr
    %args = call ptr @rt_build_tuple(i32 1, ptr %args_arr)
    call ptr @rt_call(ptr %print_fn, ptr %args, ptr null)
    ret void
}

define i32 @main(i32 %argc, ptr %argv) {
    call void @rt_init()
    call void @PyInit_hello()
    ret i32 0
}
```

### 8.3 验证方式

```bash
pylangc hello.py -o hello && ./hello
# 期望输出: Hello, World!
```

---

## 9. 实施顺序与依赖关系

```
Phase 1 (✅ 完成)
    │
    ▼
  2.1a  完善 export.hpp（新增 lifecycle/singleton/error/func/class 分类）
    │
    ▼
  2.1b  rt_api.cpp（Tier 0 + Tier 1 包装层）
    │
    ▼
  2.1c  CMake: runtime → runtime.bc（clang -emit-llvm + llvm-link）
    │
    ▼
  2.2   RuntimeLinker（扫描 llvm.global.annotations，建立索引）
    │
    ▼
  2.3a  IREmitter（逻辑名 → CreateCall）
    │
    ├──────────────────┐
    ▼                  ▼
  2.3b               2.3d
  Constant/Name/     If/While/For
  BinaryExpr         (控制流)
    │                  │
    ▼                  ▼
  2.3c               2.3e
  Call/Module/       FunctionDef
  Assign             /Return
    │                  │
    ▼                  │
  2.4  CompilerDriver ◄┘
    │
    ▼
  2.5  Hello World 🎉
    │
    ▼
  ──── Phase 3 ────
    │
  3.1  Tier 2-3: 比较/容器/迭代器
    │
  3.2  Tier 4: 闭包/make_function
    │
  3.3  Tier 5: 类与继承
    │
  3.4  Tier 6: 异常处理 (try/except)
    │
  3.5  Tier 7: 增强赋值/f-string/import
    │
    ▼
  ──── Phase 4 ────
    │
  4.1  类型特化 pass（常量折叠 + 类型传播）
    │
  4.2  Guard + Deoptimization
    │
  4.3  PGO (Profile-Guided Optimization)
```

**关键路径**：2.1a → 2.1b → 2.1c → 2.2 → 2.3a → 2.3b → 2.3c → 2.4 → 2.5

每一步的可测试产出：
- 2.1b：C++ 单元测试验证包装层正确性
- 2.1c：`file runtime.bc` 确认生成了 valid bitcode
- 2.2：单元测试验证 `linker.get("binary_add")` 返回非空
- 2.3b：生成 IR 文本，验证常量和表达式的 IR 结构
- 2.4：端到端编译 + 运行

---

## 10. 与现有代码的复用关系

| 现有组件 | Phase 2 中的角色 |
|:---|:---|
| `src/lexer/` + `src/parser/` | 100% 复用，作为 Step 1 |
| `src/ast/AST.hpp` | 100% 复用，PylangCodegen 的输入 |
| `bytecode/codegen/VariablesResolver` | 100% 复用，LEGB 分析 + Visibility 判定 |
| `bytecode/codegen/BytecodeGenerator` | **参考**：visit 方法的结构和变量管理逻辑是 PylangCodegen 的蓝本 |
| `bytecode/instructions/*.cpp` | **参考**：每条指令的 `execute()` 是 runtime API 需求的权威来源 |
| `executable/llvm/LLVMGenerator` | **不复用**：是 JIT 架构，只支持 int，不支持 PyObject |
| `executable/mlir/` | **不复用**：最终降到 bytecode，不是 native |
| `executable/Mangler` | **复用**：函数名 mangling 逻辑 |
| `src/runtime/` | 编译为 runtime.bc，通过 annotation 暴露给前端 |