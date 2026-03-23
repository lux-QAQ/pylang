## 1. 已实现 visit() 的 Python 3.9 语义分析

### 🔴 严重语义偏差

| visit | 问题 | Python 3.9 语义 | 当前实现 |
|:---|:---|:---|:---|
| **`AugAssign`** | 无 inplace 操作 | `x += y` → 先尝试 `x.__iadd__(y)`，失败才 `x = x.__add__(y)` | 直接用 `binary_add` 等，对 `list`, `dict`, `set` 会创建新对象而非原地修改 |
| **`FunctionDefinition`** | 缺默认参数 | `def f(a, b=1):` 需要 `defaults` 元组 | `defaults`/`kwdefaults` 未传给 `rt_make_function`（始终 null） |
| **`FunctionDefinition`** | 缺 `*args`/`**kwargs` | `def f(*args, **kw):` 需要解包 | 仅用 `rt_tuple_getitem` 按位置提取，无 varargs 支持 |
| **`Raise`** | bare raise | `raise`（无参数）应重新抛出当前异常 | 注释 "Phase 4+" 但调 `CreateUnreachable()`，运行时会 crash |
| **`Lambda`** | 返回 None | `lambda x: x+1` 应返回可调用对象 | 返回 `get_none()`，**静默产生错误结果** |

### 🟡 次要语义偏差

| visit | 问题 | 说明 |
|:---|:---|:---|
| **`Compare`** | 链式比较中间值重复求值 | `a < f() < c` 中 `f()` 应只求值一次，需临时变量保存 |
| **`BoolOp`** | 返回值语义 | Python `a or b` 返回 `a`（truthy时）或 `b`，不是 bool。需确认是否返回原始对象 |
| **`For`/`While` else** | else 与 break 交互 | `else` 应仅在循环**正常结束**时执行（无 break），需验证 break 是否跳过 else_bb 到 merge_bb |
| **`Starred`** | 解包语义丢失 | `*args` 展开应创建 unpack 序列，当前直接求值忽略了 star 语义 |
| **`Attribute` STORE** | 未见 setattr 路径 | `obj.attr = val` 需要调 `rt_setattr`，代码被省略不确定是否实现 |
| **`Import`** | 相对导入 | `from . import x` 的 `level` 传递正确，但 `globals` 始终为 null，无法正确解析 `__package__` |
| **`Delete`** | 局部变量删除 | Python 允许 `del x` 后再赋值，需将 alloca 设为 null 或 poison，否则 load 会拿到旧值 |

### ✅ 语义正确的实现

| visit | 说明 |
|:---|:---|
| `Constant` | 正确处理 Number/String/NameConstant(None/True/False)/Ellipsis/Bytes |
| `Name` | 通过 Visibility 正确分发 LOCAL/GLOBAL/CELL/FREE |
| `BinaryExpr` | 委托给 `rt_binary_*`，运行时正确处理 `__add__`/`__radd__` 等 |
| `UnaryExpr` | 委托给 `rt_unary_*` |
| `If`/`While`/`For` | 基本控制流结构正确 |
| `Break`/`Continue` | 正确使用 LoopContext 栈 |
| `Assert` | 正确加载 AssertionError 类型并构造异常 |
| `Global`/`NonLocal` | 正确的 no-op（语义由 VariablesResolver 处理） |
| `NamedExpr` | walrus operator `:=` 正确实现 |
| `Dict`/`Set`/`List`/`Tuple` | 容器字面量正确委托给 `rt_build_*` |
| `Subscript` | 委托给 `rt_getitem`/`rt_setitem` |
| `FunctionDefinition` 装饰器 | 从内到外正确应用 |

---

## 2. 未实现节点的问题分析

### 🔴 `ClassDefinition` 没有被正确标记为 "not implemented"

````cpp
// ...existing code...

// Phase 3.3: 类定义
ast::Value *PylangCodegen::visit(const ast::ClassDefinition *node)
{
	log::codegen()->warn("ClassDefinition not implemented in Phase 2");
	(void)node;
	return nullptr;  // ← 静默丢弃！编译"成功"但类从未创建
}

// ...existing code...
````

**核心问题**：所有"未实现"节点只是 `warn()` + 返回 `nullptr`/`get_none()`，**编译不会报错**，产生的 IR 静默错误：

| 节点 | 返回值 | 后果 |
|:---|:---|:---|
| `ClassDefinition` | `nullptr` | 类从未创建，`Foo()` 调用会 `NameError` |
| `Lambda` | `get_none()` | `f = lambda x: x` → `f` 是 `None`，`f(1)` 崩溃 |
| `ListComp` | `get_none()` | `[x for x in y]` 返回 `None` |
| `DictComp` | `get_none()` | 同上 |
| `SetComp` | `get_none()` | 同上 |
| `GeneratorExp` | `get_none()` | 同上 |
| `JoinedStr` | `get_none()` | f-string 返回 `None` |
| `Try` | `nullptr` | try/except 块静默消失，异常无法捕获 |
| `With` | `nullptr` | `__enter__`/`__exit__` 从未调用 |

### 修复建议：未实现节点应产生编译错误，而非静默忽略

````cpp
// ...existing code...

// =============================================================================
// visit() 实现 — 未实现的节点 (Phase 3+)
//
// 策略: 产生 runtime abort 而非静默返回 None
//   - 编译期: warn 告知开发者
//   - 运行期: 如果真的执行到，abort 并给出明确消息
// =============================================================================

/// 为未实现节点生成 abort 调用（确保不会静默产生错误结果）
llvm::Value *PylangCodegen::emit_not_implemented(const char *feature)
{
	log::codegen()->warn("{} not yet implemented — will abort at runtime", feature);
	auto *msg = m_emitter.create_string(
		std::string("NotImplementedError: ") + feature + " is not supported in AOT mode");
	m_emitter.call_raise(msg);
	m_builder.CreateUnreachable();

	// 创建一个新的 BB 让后续代码有插入点（不可达但避免 LLVM 报错）
	auto *dead_bb = llvm::BasicBlock::Create(
		m_ctx, "unreachable", m_codegen_ctx.current_function());
	m_builder.SetInsertPoint(dead_bb);
	return m_emitter.get_none();
}

// Phase 3.3: 类定义
ast::Value *PylangCodegen::visit(const ast::ClassDefinition *node)
{
	(void)node;
	emit_not_implemented("class definition");
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Try *node)
{
	(void)node;
	emit_not_implemented("try/except");
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Lambda *node)
{
	(void)node;
	return make_value(emit_not_implemented("lambda"));
}

ast::Value *PylangCodegen::visit(const ast::ListComp *node)
{
	(void)node;
	return make_value(emit_not_implemented("list comprehension"));
}

// ... 同理应用到所有未实现节点 ...

// ...existing code...
````

---

## 3. 优先修复建议

| 优先级 | 问题 | 修复复杂度 |
|:---:|:---|:---:|
| **P0** | 未实现节点静默返回 None/nullptr | 低 — 加 `emit_not_implemented` |
| **P0** | `AugAssign` 缺 inplace 语义 | 中 — 需新增 `rt_inplace_add` 等导出函数 |
| **P1** | `FunctionDefinition` 缺默认参数 | 中 — 需构建 defaults tuple 传给 `rt_make_function` |
| **P1** | `Lambda` 应当和 `FunctionDefinition` 共用逻辑 | 中 — 创建匿名函数 |
| **P1** | `Compare` 链式比较中间值重复求值 | 低 — 用临时 alloca 保存中间值 |
| **P2** | `Import` globals=null 导致相对导入失败 | 低 — 传 module_obj |
| **P2** | `BoolOp` 需确认返回原始对象而非 bool | 低 — 检查现有实现 |