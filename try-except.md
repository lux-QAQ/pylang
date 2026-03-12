## 四项未实现功能分析

---

### 1. `**kwargs` expansion（函数调用中的 kwargs 展开）

**Python 语义**：
```python
f(a, b, key=val, **d1, **d2)
```

**已有基础**：`rt_call(callable, args, kwargs)` 接受 `PyDict*` 作为 kwargs，`rt_dict_merge` 可合并多个 dict。

**实现方案**：修改 `visit(Call*)`

````cpp
// ...existing code...

ast::Value *PylangCodegen::visit(const ast::Call *node)
{
	auto *func = generate(node->function().get());
	if (!func) { return nullptr; }

	// --- 收集位置参数 ---
	std::vector<llvm::Value *> pos_args;
	for (const auto &arg : node->args()) {
		if (auto *starred = dynamic_cast<const ast::Starred *>(arg.get())) {
			// *iterable 展开 — Phase 3+: 需要 rt_tuple_extend
			// 暂时: 生成 generate + rt_list_extend 到临时 list，最后转 tuple
			// TODO: 实现 *args 展开
			log::codegen()->warn("Call: *args expansion not yet supported");
			auto *val = generate(starred->value().get());
			if (val) { pos_args.push_back(val); }
		} else {
			auto *val = generate(arg.get());
			if (val) { pos_args.push_back(val); }
		}
	}
	auto *args_tuple = m_emitter.create_tuple(pos_args);

	// --- 收集关键字参数 ---
	llvm::Value *kwargs_dict = nullptr;
	bool has_kwargs = false;

	for (const auto &kw : node->keywords()) {
		auto *kw_node = dynamic_cast<const ast::Keyword *>(kw.get());
		if (!kw_node) { continue; }

		auto *kw_value = generate(kw_node->value().get());
		if (!kw_value) { continue; }

		if (kw_node->arg().empty()) {
			// **d 展开: arg 为空字符串表示 **kwargs
			if (!kwargs_dict) {
				// 首次遇到: 创建空 dict
				kwargs_dict = m_emitter.create_dict({}, {});
				has_kwargs = true;
			}
			// 将 **d 合并到 kwargs_dict
			m_emitter.emit_runtime_call("dict_merge", { kwargs_dict, kw_value });
		} else {
			// key=value 普通关键字参数
			if (!kwargs_dict) {
				kwargs_dict = m_emitter.create_dict({}, {});
				has_kwargs = true;
			}
			m_emitter.emit_runtime_call("dict_setitem_str",
				{ kwargs_dict, m_emitter.create_string(kw_node->arg()), kw_value });
		}
	}

	if (!has_kwargs) { kwargs_dict = m_emitter.null_pyobject(); }

	auto *result = m_emitter.emit_runtime_call("call", { func, args_tuple, kwargs_dict });
	return make_value(result);
}

// ...existing code...
````

**需要新增**：无（已有 `rt_dict_merge`、`rt_dict_setitem_str`、`rt_call`）。

---

### 2. Subscript 目标的 AugAssign

**Python 语义**：
```python
obj[key] += value
# 等价于:
# tmp = obj.__getitem__(key)
# result = tmp.__iadd__(value)  # fallback to __add__
# obj.__setitem__(key, result)
```

**实现方案**：在 `visit(AugAssign*)` 中添加 Subscript 分支

````cpp
// ...existing code...

ast::Value *PylangCodegen::visit(const ast::AugAssign *node)
{
	// --- Step 1: LOAD 目标的当前值 ---
	llvm::Value *target_val = nullptr;

	// 保存用于 Step 4 STORE 的中间值（避免重复求值副作用表达式）
	llvm::Value *subscr_obj = nullptr;
	llvm::Value *subscr_key = nullptr;
	llvm::Value *attr_obj = nullptr;

	if (auto *name_node = dynamic_cast<const ast::Name *>(node->target().get())) {
		target_val = load_variable(name_node->ids()[0]);

	} else if (auto *attr_node = dynamic_cast<const ast::Attribute *>(node->target().get())) {
		attr_obj = generate(attr_node->value().get());
		if (!attr_obj) { return nullptr; }
		target_val = m_emitter.emit_runtime_call(
			"getattr", { attr_obj, m_emitter.create_string(attr_node->attr()) });

	} else if (auto *subscr_node = dynamic_cast<const ast::Subscript *>(node->target().get())) {
		// obj[key] += value
		subscr_obj = generate(subscr_node->value().get());
		if (!subscr_obj) { return nullptr; }

		// 从 variant 中提取 key
		if (std::holds_alternative<ast::Subscript::Index>(subscr_node->slice())) {
			auto &idx = std::get<ast::Subscript::Index>(subscr_node->slice());
			subscr_key = generate(idx.value.get());
		} else if (std::holds_alternative<ast::Subscript::Slice>(subscr_node->slice())) {
			auto &sl = std::get<ast::Subscript::Slice>(subscr_node->slice());
			auto *start = sl.lower ? generate(sl.lower.get()) : m_emitter.get_none();
			auto *stop  = sl.upper ? generate(sl.upper.get()) : m_emitter.get_none();
			auto *step  = sl.step  ? generate(sl.step.get())  : m_emitter.get_none();
			subscr_key = m_emitter.emit_runtime_call("build_slice", { start, stop, step });
		} else {
			log::codegen()->warn("AugAssign: ExtSlice not supported");
			return nullptr;
		}
		if (!subscr_key) { return nullptr; }

		// getitem: 读取当前值
		target_val = m_emitter.emit_runtime_call("getitem", { subscr_obj, subscr_key });

	} else {
		log::codegen()->error("AugAssign: unsupported target type");
		return nullptr;
	}

	if (!target_val) { return nullptr; }

	// --- Step 2: 计算 RHS ---
	auto *rhs = generate(node->value().get());
	if (!rhs) { return nullptr; }

	// --- Step 3: inplace 运算 ---
	// ...existing inplace switch code...
	llvm::Value *result = nullptr;
	// (switch 代码不变)

	if (!result) { return nullptr; }

	// --- Step 4: STORE 结果 ---
	if (auto *name_node = dynamic_cast<const ast::Name *>(node->target().get())) {
		store_variable(name_node->ids()[0], result);
	} else if (auto *attr_node = dynamic_cast<const ast::Attribute *>(node->target().get())) {
		// 复用 Step 1 保存的 attr_obj，不重复求值
		m_emitter.emit_runtime_call(
			"setattr", { attr_obj, m_emitter.create_string(attr_node->attr()), result });
	} else if (subscr_obj && subscr_key) {
		// 复用 Step 1 保存的 subscr_obj 和 subscr_key
		m_emitter.emit_runtime_call("setitem", { subscr_obj, subscr_key, result });
	}

	return nullptr;
}

// ...existing code...
````

**需要新增**：无。

---

### 3. kwargs 在类定义基类中的展开

**Python 语义**：
```python
class Foo(Bar, metaclass=Meta, **extra_kwargs):
    pass
```

**实现方案**：修改 `visit(ClassDefinition*)`

````cpp
// ...existing code...

ast::Value *PylangCodegen::visit(const ast::ClassDefinition *node)
{
	// --- Step 1: 编译类体函数 ---
	auto *body_fn = compile_class_body(
		node->name(), node->body(), node->source_location());
	if (!body_fn) { return nullptr; }

	// --- Step 2: 收集基类 ---
	std::vector<llvm::Value *> base_vals;
	for (const auto &base : node->bases()) {
		auto *val = generate(base.get());
		if (val) { base_vals.push_back(val); }
	}
	auto *bases_tuple = m_emitter.create_tuple(base_vals);

	// --- Step 3: 收集 keywords（metaclass= 等）---
	llvm::Value *kwargs_dict = nullptr;
	bool has_kwargs = false;

	for (const auto &kw : node->keywords()) {
		auto *kw_node = dynamic_cast<const ast::Keyword *>(kw.get());
		if (!kw_node) { continue; }

		auto *kw_value = generate(kw_node->value().get());
		if (!kw_value) { continue; }

		if (!kwargs_dict) {
			kwargs_dict = m_emitter.create_dict({}, {});
			has_kwargs = true;
		}

		if (kw_node->arg().empty()) {
			// **extra → 展开到 kwargs dict
			m_emitter.emit_runtime_call("dict_merge", { kwargs_dict, kw_value });
		} else {
			// metaclass=Meta 等命名关键字
			m_emitter.emit_runtime_call("dict_setitem_str",
				{ kwargs_dict, m_emitter.create_string(kw_node->arg()), kw_value });
		}
	}

	if (!has_kwargs) { kwargs_dict = m_emitter.null_pyobject(); }

	// --- Step 4: 调用 rt_build_class_aot ---
	auto *cls = m_emitter.emit_runtime_call("build_class_aot",
		{ body_fn,
			m_emitter.create_string(node->name()),
			bases_tuple,
			kwargs_dict });

	// --- Step 5: 应用装饰器 ---
	const auto &decorators = node->decorator_list();
	llvm::Value *decorated = cls;
	for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) {
		auto *dec = generate(it->get());
		if (!dec) { continue; }
		auto *dec_args = m_emitter.create_tuple({ decorated });
		decorated = m_emitter.emit_runtime_call("call", { dec, dec_args, m_emitter.null_pyobject() });
	}

	// --- Step 6: 存储到变量 ---
	store_variable(node->name(), decorated);
	return nullptr;
}

// ...existing code...
````

**需要新增**：无。

---

### 4. Try / ExceptHandler（最复杂）

**Python 语义**：
```python
try:
    risky()          # 可能抛异常
except ValueError as e:
    handle(e)        # 匹配特定异常
except:
    handle_all()     # 匹配所有异常
else:
    no_error()       # 无异常时执行
finally:
    cleanup()        # 总是执行
```

**核心问题**：当前 `rt_raise` 调用 `abort()`，异常无法被捕获。

#### 方案：`setjmp/longjmp` + 异常栈

**不用 LLVM landing pad 的原因**：landing pad 需要 C++ 异常 ABI（personality function）、LSDA 表，与当前 `abort()` 模型不兼容。`setjmp/longjmp` 是 C 级别的非局部跳转，与任何调用约定兼容。

```
┌─────────────────────────────────────────┐
│ 异常栈（thread_local 链表）               │
│                                         │
│  rt_push_exception_handler()            │
│    → setjmp(buf) → 返回 0              │
│    → 正常执行 try body                  │
│    → rt_pop_exception_handler()         │
│                                         │
│  rt_raise() 改为:                       │
│    → 如果异常栈非空: longjmp(buf, 1)    │
│    → 如果异常栈为空: abort() (不变)      │
└─────────────────────────────────────────┘
```

#### 4.1 Runtime 层：异常栈

````cpp
#pragma once

#include <csetjmp>
#include "runtime/forward.hpp"

namespace py {

/// 异常处理帧 — 对应一个 try 块
struct ExceptionHandler
{
	std::jmp_buf jmp_buf;          // setjmp/longjmp 恢复点
	ExceptionHandler *prev;        // 链表：指向外层 handler
	BaseException *caught_exc;     // longjmp 后存储的异常对象
};

/// 线程局部异常栈
class ExceptionState
{
	static thread_local ExceptionHandler *s_handler_stack;
	static thread_local BaseException *s_current_exception;

  public:
	/// 压入新 handler（try 块入口调用）
	static void push(ExceptionHandler *handler)
	{
		handler->prev = s_handler_stack;
		handler->caught_exc = nullptr;
		s_handler_stack = handler;
	}

	/// 弹出 handler（try 块正常退出时调用）
	static void pop()
	{
		if (s_handler_stack) { s_handler_stack = s_handler_stack->prev; }
	}

	/// 获取栈顶 handler
	static ExceptionHandler *top() { return s_handler_stack; }

	/// 当前活跃异常（except 块中的 sys.exc_info()）
	static BaseException *current_exception() { return s_current_exception; }
	static void set_current_exception(BaseException *exc) { s_current_exception = exc; }
};

}// namespace py
````

````cpp
#include "ExceptionState.hpp"

namespace py {
thread_local ExceptionHandler *ExceptionState::s_handler_stack = nullptr;
thread_local BaseException *ExceptionState::s_current_exception = nullptr;
}// namespace py
````

#### 4.2 修改 `rt_raise` 使用 `longjmp`

````cpp
// ...existing code...

#include "runtime/ExceptionState.hpp"
#include <csetjmp>

[[noreturn]] void rt_raise(py::BaseException *exc)
{
	// Phase 3.3: 如果有活跃的 try handler，longjmp 到它
	auto *handler = py::ExceptionState::top();
	if (handler) {
		handler->caught_exc = exc;
		py::ExceptionState::set_current_exception(exc);
		std::longjmp(handler->jmp_buf, 1);
		// 不可达
	}

	// 无 handler: 打印并 abort（与 Phase 2 行为一致）
	if (exc) {
		auto *obj = static_cast<py::PyObject *>(exc);
		const std::string &type_name_str =
			obj->type() ? obj->type()->name() : std::string("Exception");
		auto msg = obj->str();
		if (msg.is_ok()) {
			std::fprintf(stderr, "%s: %s\n", type_name_str.c_str(), msg.unwrap()->value().c_str());
		} else {
			std::fprintf(stderr, "%s\n", type_name_str.c_str());
		}
	} else {
		std::fprintf(stderr, "Unknown exception (null)\n");
	}
	std::abort();
}

// ...existing code...
````

#### 4.3 新增 export 函数

````cpp
// ...existing code...

#include <csetjmp>
#include "runtime/ExceptionState.hpp"

/// 分配 ExceptionHandler 并 push 到异常栈
/// 返回 handler 指针（包含 jmp_buf）
/// 编译器随后对其调用 setjmp
PYLANG_EXPORT_ERROR("push_exception_handler", "ptr", "")
py::ExceptionHandler *rt_push_exception_handler()
{
	// 在堆上分配（不能在栈上，因为 longjmp 会回卷栈）
	auto *handler = new py::ExceptionHandler{};
	py::ExceptionState::push(handler);
	return handler;
}

/// 弹出异常栈并释放 handler
PYLANG_EXPORT_ERROR("pop_exception_handler", "void", "ptr")
void rt_pop_exception_handler(py::ExceptionHandler *handler)
{
	py::ExceptionState::pop();
	delete handler;
}

/// 从 handler 取出被捕获的异常对象
PYLANG_EXPORT_ERROR("get_caught_exception", "obj", "ptr")
py::PyObject *rt_get_caught_exception(py::ExceptionHandler *handler)
{
	return static_cast<py::PyObject *>(handler->caught_exc);
}

/// setjmp 包装（setjmp 是宏，不能直接导出为函数指针）
/// 返回 0 = 正常，非 0 = 异常被捕获
PYLANG_EXPORT_ERROR("try_setjmp", "i32", "ptr")
int32_t rt_try_setjmp(py::ExceptionHandler *handler)
{
	return setjmp(handler->jmp_buf);
}

// ...existing code...
````

#### 4.4 Codegen：`visit(Try*)`

````cpp
// ...existing code...

ast::Value *PylangCodegen::visit(const ast::Try *node)
{
	// =====================================================================
	// try:           → setjmp → body → pop_handler → else → jmp merge
	// except E as e: → check_exception_match → handler body → jmp merge
	// finally:       → always executed (both normal + exception path)
	//
	// IR 结构:
	//
	//   %handler = call ptr @rt_push_exception_handler()
	//   %jmp_result = call i32 @rt_try_setjmp(ptr %handler)
	//   %is_exc = icmp ne i32 %jmp_result, 0
	//   br i1 %is_exc, label %except_dispatch, label %try_body
	//
	// try_body:
	//   <body>
	//   call void @rt_pop_exception_handler(ptr %handler)
	//   br label %else_block  (或 %finally 如无 else)
	//
	// except_dispatch:
	//   call void @rt_pop_exception_handler(ptr %handler)
	//   %exc = call ptr @rt_get_caught_exception(ptr %handler)
	//   ; 逐个匹配 except handler
	//   br ...
	//
	// except_N:
	//   <handler body>
	//   br label %finally
	//
	// else_block:
	//   <orelse>
	//   br label %finally
	//
	// no_match:
	//   ; 未匹配 → 重新抛出
	//   call void @rt_reraise(ptr %exc)
	//   unreachable
	//
	// finally:
	//   <finalbody>
	//   br label %merge
	//
	// merge:
	//   ; 继续执行
	// =====================================================================

	auto *func = m_codegen_ctx.current_function();

	// --- 创建所有 BB ---
	auto *try_body_bb       = llvm::BasicBlock::Create(m_ctx, "try.body", func);
	auto *except_dispatch_bb = llvm::BasicBlock::Create(m_ctx, "except.dispatch", func);
	auto *else_bb           = node->orelse().empty() ? nullptr
	                          : llvm::BasicBlock::Create(m_ctx, "try.else", func);
	auto *finally_bb        = node->finalbody().empty() ? nullptr
	                          : llvm::BasicBlock::Create(m_ctx, "try.finally", func);
	auto *merge_bb          = llvm::BasicBlock::Create(m_ctx, "try.merge", func);

	auto *after_try_bb = else_bb ? else_bb : (finally_bb ? finally_bb : merge_bb);
	auto *after_except_bb = finally_bb ? finally_bb : merge_bb;

	// --- setjmp 调用 ---
	auto *handler = m_emitter.emit_runtime_call("push_exception_handler", {});
	auto *jmp_result = m_emitter.emit_runtime_call("try_setjmp", { handler });
	auto *is_exc = m_builder.CreateICmpNE(jmp_result, m_builder.getInt32(0), "is_exc");
	m_builder.CreateCondBr(is_exc, except_dispatch_bb, try_body_bb);

	// --- try body ---
	m_builder.SetInsertPoint(try_body_bb);
	generate_body(node->body());
	if (!m_builder.GetInsertBlock()->getTerminator()) {
		m_emitter.emit_runtime_call("pop_exception_handler", { handler });
		m_builder.CreateBr(after_try_bb);
	}

	// --- except dispatch ---
	m_builder.SetInsertPoint(except_dispatch_bb);
	m_emitter.emit_runtime_call("pop_exception_handler", { handler });
	auto *exc = m_emitter.emit_runtime_call("get_caught_exception", { handler });

	// 逐个匹配 except handler
	llvm::BasicBlock *current_check_bb = except_dispatch_bb;

	for (size_t i = 0; i < node->handlers().size(); ++i) {
		auto *handler_node = dynamic_cast<const ast::ExceptHandler *>(node->handlers()[i].get());
		if (!handler_node) { continue; }

		auto handler_name = fmt::format("except.handler.{}", i);
		auto *handler_body_bb = llvm::BasicBlock::Create(m_ctx, handler_name, func);

		if (handler_node->type()) {
			// except SomeType as name:
			auto *next_check_bb = (i + 1 < node->handlers().size())
				? llvm::BasicBlock::Create(m_ctx, fmt::format("except.check.{}", i + 1), func)
				: llvm::BasicBlock::Create(m_ctx, "except.nomatch", func);

			m_builder.SetInsertPoint(current_check_bb);
			auto *exc_type = generate(handler_node->type().get());
			auto *match = m_emitter.emit_runtime_call(
				"check_exception_match", { exc, exc_type });
			// check_exception_match 返回 bool (i1)
			m_builder.CreateCondBr(match, handler_body_bb, next_check_bb);

			current_check_bb = next_check_bb;
		} else {
			// bare except:（匹配所有）
			m_builder.SetInsertPoint(current_check_bb);
			m_builder.CreateBr(handler_body_bb);
			current_check_bb = nullptr;// 不再有下一个
		}

		// handler body
		m_builder.SetInsertPoint(handler_body_bb);

		// 绑定 as 变量
		if (!handler_node->name().empty()) {
			store_variable(handler_node->name(), exc);
		}

		generate_body(handler_node->body());

		if (!m_builder.GetInsertBlock()->getTerminator()) {
			m_builder.CreateBr(after_except_bb);
		}
	}

	// 未匹配 → reraise
	if (current_check_bb) {
		m_builder.SetInsertPoint(current_check_bb);
		m_emitter.emit_runtime_call("reraise", { exc });
		m_builder.CreateUnreachable();
	}

	// --- else block ---
	if (else_bb) {
		m_builder.SetInsertPoint(else_bb);
		generate_body(node->orelse());
		if (!m_builder.GetInsertBlock()->getTerminator()) {
			m_builder.CreateBr(finally_bb ? finally_bb : merge_bb);
		}
	}

	// --- finally block ---
	if (finally_bb) {
		m_builder.SetInsertPoint(finally_bb);
		generate_body(node->finalbody());
		if (!m_builder.GetInsertBlock()->getTerminator()) {
			m_builder.CreateBr(merge_bb);
		}
	}

	// --- merge ---
	m_builder.SetInsertPoint(merge_bb);
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::ExceptHandler *node)
{
	// ExceptHandler 由 visit(Try*) 内联处理
	(void)node;
	return nullptr;
}

// ...existing code...
````

---

## 总结

| 功能 | 难度 | 新增 runtime export | 新增 runtime 层 | Codegen 修改 |
|:---|:---|:---|:---|:---|
| **kwargs expansion** | 低 | 无 | 无 | `visit(Call*)` |
| **Subscript AugAssign** | 低 | 无 | 无 | `visit(AugAssign*)` |
| **class kwargs** | 低 | 无 | 无 | `visit(ClassDefinition*)` |
| **Try/Except** | **高** | 4 个新 export | `ExceptionState.hpp/cpp` | `visit(Try*)` |

### Try/Except 的局限性（Phase 3.3）

| 限制 | 影响 | Phase 4+ 修复 |
|:---|:---|:---|
| `setjmp/longjmp` 不析构 C++ 对象 | runtime 内部的 RAII guard 可能泄漏 | 改用 C++ exceptions 或 LLVM personality |
| `finally` 中 `return` 语义不完整 | `return` 值可能被覆盖 | 需要 `finally` 状态跟踪变量 |
| 嵌套 try 的性能 | 每个 try 一次 `setjmp` | LLVM zero-cost exception handling |
| `with` 的 `__exit__` 不会在异常时调用 | 违反上下文管理器协议 | 在 `with` 中也插入 setjmp |