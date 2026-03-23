## 分析

### `build_class` vs `build_class_aot` 调用区别

| 步骤 | `build_class` (BuiltinsModule) | `build_class_aot` (新建) |
|:---|:---|:---|
| **body 函数来源** | 从字节码帧 `make_function` 获取 | 已是 `PyNativeFunction`（`rt_make_function` 创建） |
| **body 执行** | `call_with_frame(ns, args, kwargs)` — 帧 locals=ns | `call(PyTuple(ns), nullptr)` — ns 作为 args[0] |
| **类体变量写入** | 字节码 `STORE_NAME` → 写帧 locals → 写 ns | codegen `dict_setitem_str(ns, ...)` |
| **metaclass 协议** | **完全相同** | **完全相同** |
| **`__prepare__`** | **完全相同** | **完全相同** |
| **`metaclass(name,bases,ns)`** | **完全相同** | **完全相同** |
| **`__classcell__`** | `call_with_frame` 返回 cell | 从 `ns["__classcell__"]` 取 |
| **依赖 Interpreter** | ✅ 必须 | ❌ 不需要 |

### 放置位置：新建 `ClassBuilder.{hpp,cpp}`

原因：
- BuiltinsModule.cpp 是 builtins 模块注册入口，`build_class` 中深度依赖 `current_interpreter()`
- `build_class_aot` 不依赖解释器，属于**独立的运行时基础设施**
- 分离后依赖关系清晰，且两者可独立演化

### C3/多继承/super 兼容性

`build_class_aot` 最终调用 `metaclass(name, bases, ns)` → `PyType::__call__` → `PyType::__new__` → `build_type`：

```
build_type(metatype, name, base, bases, ns)
  → best_base(bases)           ← 选择 C 布局兼容的最佳基类
  → compute_bases(...)         ← 处理 __mro_entries__
  → type->initialize(name, base, bases, ns)
    → mro_()                   ← C3 线性化
    → ready()                  ← 设置 slots + 描述器
```

全链路与 `build_class` **完全一致**。

---

### 1. Runtime 层：`ClassBuilder.hpp`

````cpp
#pragma once

#include "PyObject.hpp"

namespace py {

class PyTuple;
class PyDict;

/// AOT 模式类创建 — 不依赖解释器
///
/// 与 BuiltinsModule::build_class 的区别仅在类体执行方式:
///   build_class:     call_with_frame(ns, args, kwargs) — 帧 locals = ns
///   build_class_aot: call(PyTuple(ns), nullptr)        — ns 作为 args[0]
///
/// metaclass 协议、C3 MRO、__prepare__、__classcell__ 处理完全一致
///
/// @param body_fn  类体函数（PyNativeFunction，AOT 编译器通过 rt_make_function 创建）
/// @param name     类名
/// @param bases    基类元组
/// @param kwargs   关键字参数（metaclass=... 等），可为 nullptr
PyResult<PyObject *> build_class_aot(
	PyObject *body_fn,
	const std::string &name,
	PyTuple *bases,
	PyDict *kwargs);

}// namespace py
````

### 2. Runtime 层：`ClassBuilder.cpp`

````cpp
#include "ClassBuilder.hpp"

#include "PyCell.hpp"
#include "PyDict.hpp"
#include "PyNone.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"
#include "PyType.hpp"
#include "TypeError.hpp"
#include "types/builtin.hpp"

namespace py {

PyResult<PyObject *> build_class_aot(
	PyObject *body_fn,
	const std::string &name,
	PyTuple *bases,
	PyDict *kwargs)
{
	// =================================================================
	// Step 1: 确定 metaclass
	//
	// 逻辑与 BuiltinsModule::build_class 完全一致:
	//   1. kwargs 中显式指定 metaclass=Meta
	//   2. type(bases[0])
	//   3. 默认 type
	// =================================================================
	bool metaclass_is_class = false;
	PyObject *metaclass = py_none();

	if (kwargs && !kwargs->map().empty()) {
		auto it = kwargs->map().find(String{ "metaclass" });
		if (it != kwargs->map().end()) {
			auto mc = PyObject::from(it->second);
			if (mc.is_err()) { return mc; }
			metaclass = mc.unwrap();
			if (metaclass->type()->issubclass(types::type())) {
				metaclass_is_class = true;
			}
		}
	}

	if (metaclass == py_none()) {
		if (bases->size() == 0) {
			metaclass = types::type();
		} else {
			auto base0 = PyObject::from(bases->elements()[0]);
			if (base0.is_err()) { return base0; }
			metaclass = base0.unwrap()->type();
		}
		metaclass_is_class = true;
	}

	// =================================================================
	// Step 2: 计算 most-derived metaclass
	//
	// 复用 PyType::calculate_metaclass — 与 build_class 完全一致
	// 这保证了多继承中 metaclass 冲突检测的正确性
	// =================================================================
	if (metaclass_is_class) {
		std::vector<PyType *> bases_vector;
		bases_vector.reserve(bases->size());
		for (size_t i = 0; i < bases->size(); ++i) {
			auto base_i = PyObject::from(bases->elements()[i]);
			if (base_i.is_err()) { return base_i; }
			if (auto *b = as<PyType>(base_i.unwrap())) {
				bases_vector.push_back(b);
			} else {
				return Err(type_error("bases must be types"));
			}
		}
		auto winner =
			PyType::calculate_metaclass(static_cast<PyType *>(metaclass), bases_vector);
		if (winner.is_err()) { return Err(winner.unwrap_err()); }
		metaclass = const_cast<PyType *>(winner.unwrap());
	}

	// =================================================================
	// Step 3: 创建 namespace
	//
	// 与 build_class 完全一致:
	//   - type 作为 metaclass → 直接创建空 dict
	//   - 自定义 metaclass → 调用 __prepare__(name, bases, **kwargs)
	// =================================================================
	auto class_name_ = PyString::create(name);
	if (class_name_.is_err()) { return Err(class_name_.unwrap_err()); }
	auto *class_name = class_name_.unwrap();

	auto ns_ = [metaclass, class_name, bases, kwargs]() -> PyResult<PyObject *> {
		if (metaclass == types::type()) {
			return PyDict::create();
		} else {
			auto prepare = PyString::create("__prepare__");
			if (prepare.is_err()) { return prepare; }

			// 过滤掉 metaclass= 关键字
			auto new_kwargs_ = [kwargs]() {
				if (kwargs && !kwargs->map().empty()) {
					auto prepare_kwargs = kwargs->map();
					prepare_kwargs.erase(String{ "metaclass" });
					return PyDict::create(prepare_kwargs);
				} else {
					return PyDict::create();
				}
			}();
			if (new_kwargs_.is_err()) { return new_kwargs_; }
			auto *new_kwargs = new_kwargs_.unwrap();

			auto result = metaclass->lookup_attribute(prepare.unwrap());
			if (std::get<0>(result).is_ok()
				&& std::get<1>(result) == LookupAttrResult::FOUND) {
				return std::get<0>(result).and_then(
					[class_name, bases, new_kwargs](PyObject *prepare_fn) {
						auto args = PyTuple::create(class_name, bases);
						if (args.is_err()) { return Err(args.unwrap_err()); }
						return prepare_fn->call(args.unwrap(), new_kwargs);
					});
			} else {
				return PyDict::create();
			}
		}
	}();

	if (ns_.is_err()) { return ns_; }
	auto *ns = ns_.unwrap();

	// =================================================================
	// Step 4: 预设 namespace 特殊属性
	// =================================================================
	if (auto *ns_dict = as<PyDict>(ns)) {
		ns_dict->insert(String{ "__name__" }, class_name);
		ns_dict->insert(String{ "__qualname__" }, class_name);
	}

	// =================================================================
	// Step 5: 调用类体函数
	//
	// *** 这是与 build_class 唯一的实质区别 ***
	//
	// build_class:     callable->call_with_frame(ns, empty_args, empty_kwargs)
	//                  → 帧 locals = ns → STORE_NAME 写入 ns
	//
	// build_class_aot: body_fn->call(PyTuple(ns), nullptr)
	//                  → ns 作为 args[0] → dict_setitem_str 写入 ns
	// =================================================================
	auto body_args = PyTuple::create(ns);
	if (body_args.is_err()) { return Err(body_args.unwrap_err()); }

	auto body_result = body_fn->call(body_args.unwrap(), nullptr);
	if (body_result.is_err()) { return body_result; }

	// =================================================================
	// Step 6: 创建类对象
	//
	// metaclass(name, bases, ns) → PyType::__call__ → __new__ →
	//   build_type → best_base + compute_bases + mro_() + ready()
	//
	// 完整支持 C3 MRO、多继承、描述器协议
	// =================================================================
	auto call_args = PyTuple::create(class_name, bases, ns);
	if (call_args.is_err()) { return Err(call_args.unwrap_err()); }

	auto cls = metaclass->call(call_args.unwrap(), nullptr);
	if (cls.is_err()) { return cls; }

	// =================================================================
	// Step 7: 处理 __classcell__
	//
	// 与 build_class 完全一致:
	//   如果类体创建了 __class__ cell 并存入 ns["__classcell__"]
	//   将 cell 内容设为新创建的类对象
	//   这使 super() 无参调用能正确引用 __class__
	// =================================================================
	if (auto *ns_dict = as<PyDict>(ns)) {
		String classcell_key{ "__classcell__" };
		auto it = ns_dict->map().find(classcell_key);
		if (it != ns_dict->map().end()) {
			auto cell = PyObject::from(it->second);
			if (cell.is_ok()) {
				if (auto *cell_obj = as<PyCell>(cell.unwrap())) {
					cell_obj->set_cell(Value{ cls.unwrap() });
				}
			}
		}
	}

	return cls;
}

}// namespace py
````

### 3. Export 层：薄包装

````cpp
// ...existing code...

#include "runtime/ClassBuilder.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyTuple.hpp"

// ...existing code...

PYLANG_EXPORT_CLASS("build_class_aot", "obj", "obj,str,obj,obj")
py::PyObject *rt_build_class_aot(
	py::PyObject *body_fn,
	const char *name,
	py::PyObject *bases,
	py::PyObject *kwargs)
{
	// 薄包装 — 逻辑全在 runtime/ClassBuilder.cpp
	return rt_unwrap(py::build_class_aot(
		body_fn,
		std::string(name),
		static_cast<py::PyTuple *>(bases),
		kwargs ? static_cast<py::PyDict *>(kwargs) : nullptr));
}
````

### 4. IREmitter 新增

````cpp
// ...existing code...

	// ========== Tier 5: 类创建 (Phase 3.3) ==========
	/// 加载 builtins.__build_class__
	llvm::Value *call_load_build_class();

	/// AOT 类创建: rt_build_class_aot(body_fn, name, bases, kwargs)
	llvm::Value *call_build_class_aot(llvm::Value *body_fn,
		std::string_view class_name,
		llvm::Value *bases_tuple,
		llvm::Value *kwargs);

	/// 向 dict 写入字符串键条目（类体 namespace）
	void call_dict_setitem_str(llvm::Value *dict, std::string_view key, llvm::Value *value);

	/// 从 dict 读取字符串键条目（类体 namespace）
	llvm::Value *call_dict_getitem_str(llvm::Value *dict, std::string_view key);

	// ========== Tier 6: 异常匹配 (Phase 3.3) ==========
// ...existing code...
````

````cpp
// ...existing code...

// =============================================================================
// Tier 5: 类创建 (Phase 3.3)
// =============================================================================

llvm::Value *IREmitter::call_build_class_aot(llvm::Value *body_fn,
	std::string_view class_name,
	llvm::Value *bases_tuple,
	llvm::Value *kwargs)
{
	auto *name_str = create_global_string(class_name);
	return emit_runtime_call("build_class_aot", { body_fn, name_str, bases_tuple, kwargs });
}

void IREmitter::call_dict_setitem_str(
	llvm::Value *dict, std::string_view key, llvm::Value *value)
{
	auto *key_str = create_global_string(key);
	emit_runtime_call("dict_setitem_str", { dict, key_str, value });
}

llvm::Value *IREmitter::call_dict_getitem_str(llvm::Value *dict, std::string_view key)
{
	auto *key_str = create_global_string(key);
	return emit_runtime_call("dict_getitem_str", { dict, key_str });
}

// ...existing code...
````

### 5. CodegenContext：添加 `class_namespace`

````cpp
// ...existing code...

struct ScopeContext
{
	enum class Type { MODULE, FUNCTION, CLASS };

	Type type;
	std::string name;
	std::string mangled_name;

	// ...existing code...

	/// VariablesResolver 的 Scope（用于查询变量 Visibility）
	VariablesResolver::Scope *var_scope = nullptr;

	/// CLASS scope 专用: namespace dict 的 LLVM Value*
	/// 类体中 NAME visibility 的变量通过 dict_setitem_str/dict_getitem_str 存取
	/// 非 CLASS scope 为 nullptr
	llvm::Value *class_namespace = nullptr;
};

// ...existing code...
````

### 6. PylangCodegen：修改变量存取 + 类体编译 + visit

````cpp
// ...existing code...

	/// 编译类体函数
	llvm::Value *compile_class_body(const std::string &class_name,
		const std::vector<std::shared_ptr<ast::ASTNode>> &body,
		SourceLocation source_loc);

	//  成员
// ...existing code...
````

````cpp
// ...existing code...

// =============================================================================
// 变量操作 — NAME 分支区分类作用域与模块作用域
// =============================================================================

llvm::Value *PylangCodegen::load_variable(const std::string &name)
{
	// ...existing code... (获取 visibility)

	switch (visibility) {
	case Visibility::LOCAL: {
		// ...existing code...
	}
	case Visibility::CELL: {
		// ...existing code...
	}
	case Visibility::FREE: {
		// ...existing code...
	}
	case Visibility::IMPLICIT_GLOBAL:
	case Visibility::EXPLICIT_GLOBAL: {
		// ...existing code...
	}
	case Visibility::NAME: {
		// 类作用域: 从 namespace dict 读取
		if (m_codegen_ctx.current_scope().class_namespace) {
			return m_emitter.call_dict_getitem_str(
				m_codegen_ctx.current_scope().class_namespace, name);
		}
		// 模块作用域: 等同 GLOBAL
		return m_emitter.call_load_global(m_codegen_ctx.module_object(), name);
	}
	case Visibility::HIDDEN: {
		return m_emitter.call_load_global(m_codegen_ctx.module_object(), name);
	}
	}
}

void PylangCodegen::store_variable(const std::string &name, llvm::Value *value)
{
	// ...existing code... (获取 visibility)

	switch (visibility) {
	case Visibility::LOCAL: {
		// ...existing code...
	}
	case Visibility::CELL: {
		// ...existing code...
	}
	case Visibility::FREE: {
		// ...existing code...
	}
	case Visibility::IMPLICIT_GLOBAL:
	case Visibility::EXPLICIT_GLOBAL: {
		// ...existing code...
	}
	case Visibility::NAME: {
		// 类作用域: 写入 namespace dict
		if (m_codegen_ctx.current_scope().class_namespace) {
			m_emitter.call_dict_setitem_str(
				m_codegen_ctx.current_scope().class_namespace, name, value);
			return;
		}
		// 模块作用域: 等同 GLOBAL
		m_emitter.call_store_global(m_codegen_ctx.module_object(), name, value);
		return;
	}
	case Visibility::HIDDEN: {
		m_emitter.call_store_global(m_codegen_ctx.module_object(), name, value);
		return;
	}
	}
}

// ...existing code...

// =============================================================================
// 类体编译
//
// 生成函数: PyObject* ClassName.__body__(PyTuple* args, PyDict* kwargs)
// 调用时: args[0] = namespace dict
//
// 类体中的变量存取路径:
//   store_variable("x", val)
//     → visibility = NAME
//     → class_namespace != nullptr
//     → call_dict_setitem_str(ns, "x", val)
// =============================================================================

llvm::Value *PylangCodegen::compile_class_body(
	const std::string &class_name,
	const std::vector<std::shared_ptr<ast::ASTNode>> &body,
	SourceLocation source_loc)
{
	auto *obj_ptr_ty = m_emitter.pyobject_ptr_type();

	// --- Step 1: 创建 LLVM 函数 ---
	auto *func_type = llvm::FunctionType::get(obj_ptr_ty, { obj_ptr_ty, obj_ptr_ty }, false);

	auto mangled = m_mangler.function_mangle(
		m_codegen_ctx.current_scope().name, class_name, source_loc);

	auto *llvm_func = llvm::Function::Create(
		func_type, llvm::Function::InternalLinkage, mangled + ".__body__", m_module.get());

	llvm_func->getArg(0)->setName("args");
	llvm_func->getArg(1)->setName("kwargs");

	// --- Step 2: 保存当前状态，切换到类体 ---
	auto *saved_bb = m_builder.GetInsertBlock();

	auto *entry_bb = llvm::BasicBlock::Create(m_ctx, "entry", llvm_func);
	m_builder.SetInsertPoint(entry_bb);

	// --- Step 3: 提取 namespace dict = args[0] ---
	auto *ns = m_emitter.emit_runtime_call("tuple_getitem",
		{ llvm_func->getArg(0), m_builder.getInt32(0) });

	// --- Step 4: 查找变量 scope ---
	auto it = m_codegen_ctx.visibility_map().find(mangled);
	VariablesResolver::Scope *var_scope =
		(it != m_codegen_ctx.visibility_map().end()) ? it->second.get() : nullptr;

	// --- Step 5: 进入 CLASS scope ---
	ScopeContext class_scope{};
	class_scope.type = ScopeContext::Type::CLASS;
	class_scope.name = class_name;
	class_scope.mangled_name = mangled;
	class_scope.llvm_func = llvm_func;
	class_scope.module_obj = m_codegen_ctx.module_object();
	class_scope.var_scope = var_scope;
	class_scope.class_namespace = ns;
	m_codegen_ctx.push_scope(std::move(class_scope));

	// --- Step 6: 编译类体 ---
	generate_body(body);

	// --- Step 7: 返回 None ---
	if (!m_builder.GetInsertBlock()->getTerminator()) {
		m_builder.CreateRet(m_emitter.get_none());
	}

	// --- Step 8: 离开 CLASS scope ---
	m_codegen_ctx.pop_scope();

	// --- Step 9: 恢复插入点 ---
	m_builder.SetInsertPoint(saved_bb);

	// --- Step 10: 包装为 Python 可调用对象 ---
	return m_emitter.call_make_function(
		class_name,
		llvm_func,
		m_codegen_ctx.module_object(),
		m_emitter.null_pyobject(),
		m_emitter.null_pyobject(),
		m_emitter.null_pyobject());
}

// =============================================================================
// visit(ClassDefinition)
//
// Python:
//   @decorator
//   class Foo(Bar, metaclass=Meta):
//       x = 1
//       def method(self): ...
//
// IR 调用序列:
//   %bases  = rt_build_tuple(...)
//   %kwargs = rt_build_dict(...)         ; metaclass=Meta
//   %body   = rt_make_function("Foo", @Foo.__body__, ...)
//   %cls    = rt_build_class_aot(%body, "Foo", %bases, %kwargs)
//   rt_store_global(%mod, "Foo", %cls)
//
// rt_build_class_aot 内部:
//   → py::build_class_aot (ClassBuilder.cpp)
//     → calculate_metaclass       ← C3 metaclass 冲突检测
//     → __prepare__               ← 创建 namespace
//     → body_fn(PyTuple(ns))      ← 执行类体
//     → metaclass(name,bases,ns)  ← PyType::__new__ → mro_() → ready()
//     → __classcell__ 处理        ← super() 支持
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::ClassDefinition *node)
{
	// --- Step 1: 编译基类表达式 ---
	std::vector<llvm::Value *> base_values;
	for (const auto &base : node->bases()) {
		auto *val = generate(base.get());
		if (val) { base_values.push_back(val); }
	}
	auto *bases_tuple = m_emitter.create_tuple(base_values);

	// --- Step 2: 编译 keyword 参数（metaclass= 等）---
	llvm::Value *kwargs = m_emitter.null_pyobject();
	if (!node->keywords().empty()) {
		std::vector<llvm::Value *> kw_keys;
		std::vector<llvm::Value *> kw_vals;
		for (const auto &kw : node->keywords()) {
			auto *kw_node = static_cast<const ast::Keyword *>(kw.get());
			auto *val = generate(kw_node->value().get());
			if (val) {
				kw_keys.push_back(m_emitter.create_string(kw_node->arg()));
				kw_vals.push_back(val);
			}
		}
		if (!kw_keys.empty()) { kwargs = m_emitter.create_dict(kw_keys, kw_vals); }
	}

	// --- Step 3: 编译类体函数 ---
	auto *body_fn = compile_class_body(
		node->name(), node->body(), node->source_location());
	if (!body_fn) {
		log::codegen()->error("Failed to compile class body for '{}'", node->name());
		return nullptr;
	}

	// --- Step 4: 调用 rt_build_class_aot ---
	auto *cls = m_emitter.call_build_class_aot(
		body_fn, node->name(), bases_tuple, kwargs);

	// --- Step 5: 应用装饰器（从内到外）---
	for (auto it = node->decorator_list().rbegin(); it != node->decorator_list().rend(); ++it) {
		auto *decorator = generate(it->get());
		if (decorator) {
			auto *args = m_emitter.create_tuple({ cls });
			cls = m_emitter.call_function(decorator, args);
		}
	}

	// --- Step 6: 存储到当前作用域 ---
	store_variable(node->name(), cls);
	return nullptr;
}

// ...existing code...
````

### 7. CMakeLists.txt

````cmake
# ...existing code...

# 在 runtime 源文件列表中添加:
ClassBuilder.cpp

# ...existing code...
````

---

## 调用链路对比

```
═══════════════════════════════════════════════════════════
  解释器路径 (build_class)
═══════════════════════════════════════════════════════════
  LOAD_BUILD_CLASS
  → builtins.__build_class__(body_fn, name, *bases, **kw)
    → calculate_metaclass(...)          ← PyType.cpp
    → metaclass.__prepare__(name, bases)
    → body_fn.call_with_frame(ns, ...)  ← 帧 locals = ns
      → STORE_NAME "x" → 写帧 locals → 写 ns
    → metaclass(name, bases, ns)
      → PyType::__new__ → build_type → mro_() → ready()

═══════════════════════════════════════════════════════════
  AOT 路径 (build_class_aot)
═══════════════════════════════════════════════════════════
  call @rt_build_class_aot(%body, "Foo", %bases, %kwargs)
  → py::build_class_aot(...)            ← ClassBuilder.cpp
    → calculate_metaclass(...)          ← 同一个函数
    → metaclass.__prepare__(name, bases)← 同一个协议
    → body_fn->call(PyTuple(ns), null)  ← *** 唯一区别 ***
      → dict_setitem_str(ns, "x", val) ← codegen 生成
    → metaclass(name, bases, ns)        ← 同一个调用
      → PyType::__new__ → build_type → mro_() → ready()
```