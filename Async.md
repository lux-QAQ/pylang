

## 异步 Codegen 实现分析

### 现状盘点

已有的 runtime 基础设施：
- `PyGenerator` / `PyCoroutine` / `PyAsyncGenerator` — 三种生成器类型
- `GeneratorInterface<T>` — 统一的 `send()` / `__next__()` / `close()` 接口
- `PyCode::eval()` 中已有 `CodeFlags::GENERATOR` / `CodeFlags::COROUTINE` 的分支处理
- `BytecodeGenerator` 中已有 `Yield` / `YieldFrom` / `Await` / `AsyncFunctionDefinition` 的字节码实现

核心难点：字节码解释器的 generator/coroutine 依赖**栈帧挂起/恢复**（`StackFrame` 保存寄存器状态，下次 `send()` 从断点继续执行）。AOT 编译器没有解释器循环，必须用不同的机制实现挂起/恢复。

### 方案选择

AOT 编译 generator/coroutine 有三种主流方案：

| 方案 | 原理 | 优点 | 缺点 |
|:---|:---|:---|:---|
| 1. Outlined coroutine (LLVM coroutines) | `llvm.coro.*` intrinsics，LLVM 自动拆分函数为 resume/destroy/cleanup | 原生 LLVM 支持，性能最优 | 复杂，需要精确管理 coro frame |


**推荐方案 1：LLVM Coroutines**。这是 Clang、Rust、Swift 都在用的方案，与现有 EH 机制完全兼容。

### 需要添加的代码

#### 1. Runtime 导出层 — `src/runtime/export/rt_coro.cpp`（新文件）

```
需要导出的函数：

rt_create_generator(frame_ptr, resume_fn, destroy_fn, name) → PyGenerator*
  - 从 AOT coroutine frame 创建 PyGenerator 包装

rt_create_coroutine(frame_ptr, resume_fn, destroy_fn, name) → PyCoroutine*
  - 从 AOT coroutine frame 创建 PyCoroutine 包装

rt_create_async_generator(frame_ptr, resume_fn, destroy_fn, name) → PyAsyncGenerator*
  - 从 AOT coroutine frame 创建 PyAsyncGenerator 包装

rt_generator_yield(generator, value) → void
  - yield value：挂起当前 coroutine，将 value 作为 __next__() 的返回值

rt_generator_get_sent_value(generator) → PyObject*
  - 恢复后获取 send() 传入的值

rt_get_awaitable(obj) → PyObject*
  - 实现 GET_AWAITABLE 语义：
    - 如果 obj 是 coroutine，直接返回
    - 否则调用 obj.__await__()

rt_yield_from(generator, iterable) → PyObject*
  - yield from 语义：委托给子迭代器

rt_async_generator_wrap_value(value, is_stop) → PyObject*
  - async generator 的 yield 包装（区分 yield value 和 return）
```

#### 2. Runtime 核心层 — 修改 `GeneratorInterface` 支持 AOT

当前 `GeneratorInterface::send()` 调用 `interpreter->call()`（字节码执行）。AOT 模式需要调用 LLVM coroutine 的 resume 函数。

```cpp
// src/runtime/PyNativeGenerator.hpp（新文件）
// AOT 编译的 generator/coroutine 的运行时表示

class PyNativeGenerator : public PyBaseObject {
    void *m_coro_frame;           // LLVM coro frame 指针
    void (*m_resume_fn)(void*);   // @func.resume
    void (*m_destroy_fn)(void*);  // @func.destroy
    bool m_done = false;
    PyObject *m_yielded_value = nullptr;
    PyObject *m_sent_value = nullptr;
    PyString *m_name;

public:
    PyResult<PyObject*> send(PyObject *value);
    PyResult<PyObject*> __next__();
    PyResult<PyObject*> __iter__() const;
    PyResult<PyObject*> close();
    // throw() 支持
};

// 同理 PyNativeCoroutine, PyNativeAsyncGenerator
```

#### 3. IREmitter 新增方法 — IREmitter.hpp

```cpp
// ========== Tier 7: 协程/生成器 (Phase 4) ==========

/// 声明 LLVM coroutine intrinsics
void declare_coro_intrinsics();

/// 在函数入口插入 coro.begin
/// 返回 coro frame handle
llvm::Value *emit_coro_begin(llvm::Function *func);

/// 在 yield 点插入 coro.suspend
/// @param value  yield 的值（存入 generator 的 yielded_value）
/// @param is_final  是否是最终挂起点（函数 return 时）
/// @return  恢复后的 sent value
llvm::Value *emit_coro_suspend(llvm::Value *coro_handle, 
                                llvm::Value *yield_value,
                                bool is_final = false);

/// 在函数末尾插入 coro.end + coro.free
void emit_coro_end(llvm::Value *coro_handle);

/// 创建 generator/coroutine 对象
/// @param coro_handle  coro.begin 返回的 handle
/// @param name         函数名
/// @param flags        CodeFlags（区分 generator/coroutine/async_generator）
llvm::Value *emit_create_generator(llvm::Value *coro_handle,
                                    std::string_view name,
                                    uint32_t flags);

/// GET_AWAITABLE: obj → awaitable
llvm::Value *call_get_awaitable(llvm::Value *obj);

/// YIELD_FROM: 委托给子迭代器
llvm::Value *call_yield_from(llvm::Value *generator, llvm::Value *iterable);
```

#### 4. PylangCodegen — visit 实现

```
visit(AsyncFunctionDefinition*)
  与 FunctionDefinition 几乎相同，区别：
  1. 编译后的函数标记 CodeFlags::COROUTINE
  2. 函数体用 LLVM coroutine 框架包装
  3. 返回 PyNativeCoroutine 而非直接返回值

visit(FunctionDefinition*) 修改
  如果函数体包含 yield → 标记为 generator
  如果函数体包含 yield + async → 标记为 async_generator
  生成 LLVM coroutine 框架

visit(Yield*)
  1. emit_coro_suspend(handle, yield_value)
  2. 恢复后 emit_generator_get_sent_value() 获取 send 值
  3. 返回 sent value 作为 yield 表达式的值

visit(YieldFrom*)
  1. value = generate(node->value())
  2. iter = call_get_iter(value)
  3. 循环: 
     sent = emit_coro_suspend(handle, received)
     try: received = call(iter.send, sent)
     except StopIteration as e: result = e.value; break
  4. 返回 result

visit(Await*)
  语义等同于 yield from，但要求 awaitable 协议：
  1. awaitable = call_get_awaitable(value)
  2. 同 yield from 逻辑
```

#### 5. LLVM Coroutine IR 结构

一个 generator 函数编译后的 IR 结构：

```llvm
define ptr @gen_func(ptr %module) presplitcoroutine {
entry:
  %id = call token @llvm.coro.id(i32 0, ptr null, ptr null, ptr null)
  %size = call i64 @llvm.coro.size.i64()
  %mem = call ptr @malloc(i64 %size)
  %hdl = call ptr @llvm.coro.begin(token %id, ptr %mem)
  
  ; --- 初始挂起点（创建 generator 后立即挂起）---
  %0 = call i8 @llvm.coro.suspend(token none, i1 false)
  switch i8 %0, label %suspend [i8 0, label %resume.0  i8 1, label %cleanup]

resume.0:
  ; --- 函数体 ---
  %val = call ptr @rt_integer_from_i64(i64 42)
  call void @rt_generator_yield(ptr %generator, ptr %val)
  
  ; --- yield 挂起点 ---
  %1 = call i8 @llvm.coro.suspend(token none, i1 false)
  switch i8 %1, label %suspend [i8 0, label %resume.1  i8 1, label %cleanup]

resume.1:
  %sent = call ptr @rt_generator_get_sent_value(ptr %generator)
  ; ... 继续执行 ...
  br label %cleanup

cleanup:
  %mem2 = call ptr @llvm.coro.free(token %id, ptr %hdl)
  call void @free(ptr %mem2)
  br label %suspend

suspend:
  call i1 @llvm.coro.end(ptr %hdl, i1 false, token none)
  ret ptr %hdl
}
```

#### 6. 与异常机制的兼容

关键点：LLVM coroutine 的 suspend/resume 与 landingpad EH 完全兼容。

```
try 块内的 yield：
  - yield 前：正常的 invoke → landingpad
  - yield 挂起：coro.suspend 保存整个栈帧（包括 EH 状态）
  - 恢复后：EH 状态自动恢复，landingpad 仍然有效

generator.throw(exc)：
  - 调用 resume 后，在 generator 内部 raise exc
  - 如果 generator 有 try/except，正常捕获
  - 如果没有，异常传播到调用者

generator.close()：
  - 等同于 generator.throw(GeneratorExit)
  - 如果 generator 有 finally，正常执行
```

#### 7. 需要修改的现有文件

```
修改:
  src/compiler/Codegen/IREmitter.hpp      — 新增 coro 相关方法声明
  src/compiler/Codegen/IREmitter.cpp      — 实现 coro 相关方法
  src/compiler/Codegen/PylangCodegen.hpp  — compile_function_body 增加 coro 参数
  src/compiler/Codegen/PylangCodegen.cpp  — visit(Yield/YieldFrom/Await/AsyncFunctionDef)
  src/runtime/export/export.hpp           — 新增 PYLANG_EXPORT_CORO 宏

新增:
  src/runtime/export/rt_coro.cpp          — 协程相关导出函数
  src/runtime/PyNativeGenerator.hpp       — AOT generator 运行时类型
  src/runtime/PyNativeGenerator.cpp       — 实现
  src/runtime/PyNativeCoroutine.hpp       — AOT coroutine 运行时类型  
  src/runtime/PyNativeCoroutine.cpp       — 实现
  src/runtime/PyNativeAsyncGenerator.hpp  — AOT async generator 运行时类型
  src/runtime/PyNativeAsyncGenerator.cpp  — 实现
```

#### 8. 实现顺序建议

```
Phase 1: Generator（最基础）
  1. rt_coro.cpp 导出函数
  2. PyNativeGenerator 运行时类型
  3. IREmitter coro intrinsics
  4. visit(Yield*) + compile_function_body 的 generator 路径
  5. 测试: def gen(): yield 1; yield 2

Phase 2: Coroutine（在 generator 基础上）
  1. PyNativeCoroutine 运行时类型
  2. visit(AsyncFunctionDefinition*)
  3. visit(Await*)
  4. 测试: async def f(): return await g()

Phase 3: yield from + async generator
  1. visit(YieldFrom*)
  2. PyNativeAsyncGenerator
  3. 测试: async def agen(): yield 1
```

要开始实现吗？建议从 Phase 1 的 `rt_coro.cpp` + `PyNativeGenerator` 开始。