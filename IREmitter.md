## 分析结果

### Part 1: IREmitter 当前完善度

| Tier | Phase | 完成度 | 缺失项 |
|:---:|:---:|:---:|:---|
| 0-2 | 2.5-3.1 | ✅ 100% | — |
| 3 | 3.1 | ✅ 100% | — |
| 4 | 3.2 | ⚠️ 40% | `make_function`, `get_closure` |
| 5 | 3.3 | ⚠️ 67% | `load_build_class` |
| 6 | 3.4 | ✅ 100% | — |
| 7 | 3.5 | ❌ 0% | inplace ops, f-string, import_from/star |

**Phase 2.5 (Hello World)：完全足够。**

---

### Part 2: 不应导出而应通过 builtins 访问的函数

这是**核心问题**。对照 [`BuiltinsModule.cpp`](src/runtime/modules/BuiltinsModule.cpp ) 的注册表和 [`rt_convert.cpp`](src/runtime/export/rt_convert.cpp ) 中的导出，存在**重复导出**：

#### ❌ 不应导出（已在 builtins 模块中注册）

| 导出函数 | 所在文件 | builtins 中的对应 | 分析 |
|:---|:---|:---|:---|
| `rt_len` | rt_convert.cpp | `builtins.len` | `len(x)` 是 Python 函数调用，应走 `rt_load_global("len") + rt_call` |
| `rt_isinstance` | rt_convert.cpp | `builtins.isinstance` | 同上 |
| `rt_type_of` | rt_convert.cpp | `builtins.type` | `type(x)` 单参数形式，builtins 中已注册 type 类型 |
| `rt_to_int` | rt_convert.cpp | `builtins.int` | `int(x)` 是类型构造调用 |
| `rt_to_str` | rt_convert.cpp | `builtins.str` | `str(x)` 同上 |
| `rt_to_float` | rt_convert.cpp | `builtins.float` | `float(x)` 同上 |
| `rt_to_bool` | rt_convert.cpp | `builtins.bool` | `bool(x)` 同上 |

**证据**：在 `builtins_module()` 中，`int`、`str`、`float`、`bool`、`type` 都通过 `builtin_types()` 注册为类型对象。`len`、`isinstance` 显式注册为 `PyNativeFunction`。

**调用路径对比**：

```
# len(x) 当前的冗余导出路径：
%result = call i64 @rt_len(%obj)          ← 直接调用，跳过 Python 调用协议

# len(x) 正确的 builtins 路径：
%len_fn = call ptr @rt_load_global(%mod, "len")
%args   = call ptr @rt_build_tuple(i32 1, ptr %arr)
%result = call ptr @rt_call(%len_fn, %args, null)

# int(x) 当前的冗余导出路径：
%result = call ptr @rt_to_int(%obj)       ← 直接调用

# int(x) 正确的 builtins 路径：
%int_type = call ptr @rt_load_global(%mod, "int")
%args     = call ptr @rt_build_tuple(i32 1, ptr %arr)
%result   = call ptr @rt_call(%int_type, %args, null)
```

#### ✅ 必须保留的导出（编译器原语，不是 Python 函数）

| 导出函数 | 原因 |
|:---|:---|
| `rt_is_true` | `if x:`、`while x:`、`and`、`or` 的内部操作，**不是 Python 函数** |

`is_true` 对应 CPython 的 `PyObject_IsTrue()`，是**字节码指令级别**的原语（`JUMP_IF_FALSE`），Python 层面没有对应的可调用对象。

---

### Part 3: 修改建议

#### 1. 清理 [`rt_convert.cpp`](src/runtime/export/rt_convert.cpp )

````cpp
#include "rt_common.hpp"

#include "runtime/PyBool.hpp"

// =============================================================================
// Tier 1: 布尔判断（编译器原语，不是 Python 函数）
// =============================================================================

PYLANG_EXPORT_CONVERT("is_true", "bool", "obj")
bool rt_is_true(py::PyObject *obj) { return rt_unwrap(obj->true_()); }

// =============================================================================
// 以下函数已移除：
//   rt_len        → 通过 builtins.len 访问
//   rt_isinstance → 通过 builtins.isinstance 访问
//   rt_type_of    → 通过 builtins.type 访问
//   rt_to_int     → 通过 builtins.int 访问
//   rt_to_str     → 通过 builtins.str 访问
//   rt_to_float   → 通过 builtins.float 访问
//   rt_to_bool    → 通过 builtins.bool 访问
//
// 这些都是 Python 可调用对象，应通过：
//   rt_load_global(mod, "len") + rt_call(len_fn, args)
// 的标准路径访问，不需要专门导出。
// =============================================================================
````

#### 2. 清理 IREmitter 中对应的方法

从 [`IREmitter.hpp`](src/compiler/Codegen/IREmitter.hpp ) 中移除：

````cpp
// ...existing code...

	// ========== Tier 1: 类型转换 ==========
	llvm::Value *call_is_true(llvm::Value *obj);

	// ❌ 以下方法移除（通过 builtins 访问）：
	// llvm::Value *call_len(llvm::Value *obj);
	// llvm::Value *call_isinstance(llvm::Value *obj, llvm::Value *type);
	// llvm::Value *call_type_of(llvm::Value *obj);
	// llvm::Value *call_to_int(llvm::Value *obj);
	// llvm::Value *call_to_str(llvm::Value *obj);
	// llvm::Value *call_to_float(llvm::Value *obj);
	// llvm::Value *call_to_bool(llvm::Value *obj);

// ...existing code...
````

#### 3. PylangCodegen 中如何处理 `len(x)`

PylangCodegen 在遇到 `len(x)` 时的正确做法：

```cpp
// PylangCodegen::visit(Call *node) 中，不需要特殊处理 len/isinstance/int 等
// 统一走：
//   1. visit(node->func())    → 可能是 rt_load_global(mod, "len")
//   2. visit each arg         → 构建参数
//   3. emitter.call_function(func, args)
```

---

### Part 4: 完整的导出/不导出分类表

| 分类 | 导出 | 不导出（builtins） |
|:---|:---|:---|
| **编译器原语** | `is_true`, 所有运算符, 所有比较, `get_iter`/`iter_next`, `getitem`/`setitem`, `load_global`/`store_global` | — |
| **字面量语法** | `string_from_cstr`, `integer_from_i64`, `build_tuple`, `build_list`, `build_dict`, `bytes_from_buffer`, `complex_from_doubles` | — |
| **闭包机制** | `create_cell`, `cell_get`, `cell_set` | — |
| **异常机制** | `raise`, `reraise`, `check_exception_match` | — |
| **Python 函数** | — | `len`, `isinstance`, `type`, `int`, `str`, `float`, `bool`, `print`, `repr`, `abs`, `hash`, ... |

---

### Part 5: Phase 3.2 必须补充的 IREmitter 方法

````cpp
// ...existing code...

	// ========== Tier 4: 函数创建 (Phase 3.2) ==========
	llvm::Value *call_make_function(
		llvm::Value *code_ptr,
		llvm::Value *module,
		llvm::Value *defaults = nullptr,
		llvm::Value *kwdefaults = nullptr,
		llvm::Value *closure = nullptr);
	llvm::Value *call_get_closure(llvm::Value *func);

	// ========== Tier 5: 类创建 (Phase 3.3) ==========
	llvm::Value *call_load_build_class();

// ...existing code...
````

**对应的 runtime 导出也需要新增**（rt_func.cpp 和 `rt_class.cpp`），但这属于 Phase 3.2/3.3 的工作。

---

### 总结

| 问题 | 结论 |
|:---|:---|
| IREmitter 对 Phase 2.5 足够吗？ | ✅ 完全足够 |
| 有不该导出的函数吗？ | ✅ `len`/`isinstance`/`type_of`/`to_int`/`to_str`/`to_float`/`to_bool` 共 7 个应移除 |
| 唯一保留的 convert 导出？ | `is_true`（编译器原语，非 Python 函数） |
| Phase 3.2 最关键缺失？ | `make_function` + `get_closure` |