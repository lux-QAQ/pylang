# 异步 Codegen 实现设计 — LLVM Coroutines

## 1. 现状盘点

### 已有的 runtime 基础设施
- `PyGenerator` / `PyCoroutine` / `PyAsyncGenerator` — 三种生成器类型（字节码解释器用，依赖 `StackFrame`）
- `GeneratorInterface<T>` — 统一的 `send()` / `__next__()` / `close()` 接口（依赖 `interpreter->call()`）
- `StopIteration` / `GeneratorExit` / `RuntimeError` / `StopAsyncIteration` 异常类型已实现
- `PylangException` + landingpad EH 机制已完整工作
- `export.hpp` 宏体系已建立，支持分类导出
- `BuiltinTypes` 已注册 `generator` / `coroutine` / `async_generator` 类型槽位

### AOT 约束
- **不使用任何解释器相关代码**（无 `StackFrame`、无 `PyFrame`、无 `interpreter->call()`）
- 使用 LLVM Coroutines（`llvm.coro.*` intrinsics）实现挂起/恢复
- 与现有 landingpad EH 机制完全兼容
- `PyNativeGenerator` / `PyNativeCoroutine` / `PyNativeAsyncGenerator` 是全新类型，不复用 `GeneratorInterface<T>`

## 2. Python 3.9 语义完整性清单

| # | 语义 | 状态 | 实现阶段 |
|:--|:-----|:-----|:---------|
| 1 | `def gen(): yield x` — 基础 generator | 待实现 | Phase 1 |
| 2 | `gen.send(value)` — 双向通信 | 待实现 | Phase 1 |
| 3 | `gen.throw(type[, value[, tb]])` — 三参数形式 | 待实现 | Phase 1 |
| 4 | `gen.close()` → GeneratorExit | 待实现 | Phase 1 |
| 5 | `return value` in generator → `StopIteration.value` | 待实现 | Phase 1 |
| 6 | generator return 后再 `next()` → `StopIteration` | 待实现 | Phase 1 |
| 7 | PEP 479: `StopIteration` 在 generator 内 → `RuntimeError` | 待实现 | Phase 1 |
| 8 | yield in `try/finally` 的精确 `close()` 行为 | 待实现 | Phase 1 |
| 9 | `y = yield x` — yield 作为表达式 | 待实现 | Phase 1 |
| 10 | 首次 `send(non-None)` → `TypeError` | 待实现 | Phase 1 |
| 11 | `yield from iterable` — 委托迭代 | 待实现 | Phase 2 |
| 12 | `yield from` 双向透传 send/throw | 待实现 | Phase 2 |
| 13 | `yield from` 的 `StopIteration.value` 作为表达式值 | 待实现 | Phase 2 |
| 14 | `async def f(): await x` — 基础 coroutine | 待实现 | Phase 3 |
| 15 | `await` 的 `__await__` 协议 | 待实现 | Phase 3 |
| 16 | coroutine 未 await 时的 RuntimeWarning | 延后 | Phase 5 |
| 17 | `async def agen(): yield x` — async generator | 待实现 | Phase 4 |
| 18 | `async for x in agen()` — `__aiter__` / `__anext__` | 待实现 | Phase 4 |
| 19 | `async with ctx as v` — `__aenter__` / `__aexit__` | 待实现 | Phase 4 |
| 20 | `StopAsyncIteration` 终止 `async for` | 待实现 | Phase 4 |
| 21 | async generator `athrow` / `aclose` / `asend` | 待实现 | Phase 4 |
| 22 | `sys.set_asyncgen_hooks` (firstiter/finalizer) | 延后 | Phase 5+ |
| 23 | `@types.coroutine` 装饰器兼容 | 延后 | Phase 5 |
| 24 | `sys.set_coroutine_wrapper` (3.7 deprecated) | 不实现 | — |

## 3. 架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                      PylangCodegen                           │
│  visit(Yield*) visit(YieldFrom*) visit(Await*)              │
│  visit(FunctionDef*) visit(AsyncFunctionDef*)               │
│  compile_generator_body()                                    │
│  emit_coro_prologue() emit_coro_suspend() emit_coro_epilogue│
├─────────────────────────────────────────────────────────────┤
│                       IREmitter                              │
│  emit_coro_id()  emit_coro_begin()  emit_coro_suspend()     │
│  emit_coro_end() emit_coro_free()  emit_coro_save()         │
│  call_generator_set_yield()  call_generator_get_sent()      │
│  call_create_native_generator/coroutine/async_generator()   │
│  call_get_awaitable()  call_yield_from_step()               │
├─────────────────────────────────────────────────────────────┤
│                     RuntimeLinker                             │
│  declare_in() for rt_coro.cpp exported functions             │
├─────────────────────────────────────────────────────────────┤
│                    Runtime Export Layer                       │
│  rt_coro.cpp: PYLANG_EXPORT_CORO(...)                        │
├─────────────────────────────────────────────────────────────┤
│                    Runtime Core Layer                         │
│  NativeCoroState (通信结构, 类型安全枚举)                      │
│  PyNativeGenerator   → send/throw/close/__iter__/__next__    │
│  PyNativeCoroutine   → send/throw/close/__await__            │
│  PyNativeAsyncGenerator → asend/athrow/aclose/__aiter__      │
└─────────────────────────────────────────────────────────────┘
```

## 4. NativeCoroState — 类型安全的通信协议

### 4.1 状态枚举

```cpp
// src/runtime/NativeCoroState.hpp

/// 协程/生成器的生命周期状态
/// 使用 enum class 替代裸数字，编译期类型安全
enum class CoroStatus : int32_t
{
    CREATED   = 0,  // 已创建，尚未启动（初始挂起前）
    SUSPENDED = 1,  // 已挂起（yield 后等待 resume）
    RUNNING   = 2,  // 正在执行（send/throw 后，下一个 yield 前）
    DONE      = 3,  // 已完成（return 或未捕获异常后）
};

/// resume 时的操作类型
/// 告诉 generator 函数内部：恢复后应该做什么
enum class ResumeAction : int32_t
{
    SEND  = 0,  // 正常 send(value)
    THROW = 1,  // throw(exc) — generator 内部需要 raise
    CLOSE = 2,  // close() — 等同于 throw(GeneratorExit)
};
```

### 4.2 通信结构

```cpp
/// 嵌入在 LLVM coro frame 中的通信结构
///
/// 编译器在 coro.begin 后通过 alloca 创建此结构。
/// 由于 alloca 在 coro frame 内，LLVM CoroSplit 会自动将其
/// 提升到堆分配的 coro frame 中，跨 suspend 点存活。
///
/// runtime 通过 PyNativeGenerator 持有的 state 指针访问。
struct NativeCoroState
{
    PyObject *yielded_value;     // generator → caller (yield 出的值)
    PyObject *sent_value;        // caller → generator (send 进的值)
    PyObject *thrown_exception;  // caller → generator (throw 进的异常)
    PyObject *return_value;      // generator return value → StopIteration.value
    CoroStatus status;           // 生命周期状态
    ResumeAction resume_action;  // 恢复时的操作类型
};
```

### 4.3 为什么不嵌入 coro frame 头部

LLVM coro frame 的内存布局由 CoroSplit pass 决定，不可控。
`NativeCoroState` 作为 coro frame 内的 alloca 存在：
- CoroSplit 自动将跨 suspend 的 alloca 提升到 coro frame
- 编译器在 `coro.begin` 后创建 alloca，将地址传给 `rt_create_native_generator`
- runtime 通过 `PyNativeGenerator::m_state` 指针访问

## 5. LLVM Coroutine IR 结构

### 5.1 Generator 函数完整 IR

```python
def gen(x):
    y = yield x + 1
    yield y * 2
    return 42
```

编译为:

```llvm
define ptr @gen(ptr %module, ptr %args, ptr %kwargs) presplitcoroutine
    personality ptr @__gxx_personality_v0 {
entry:
  ; === coro 框架初始化 ===
  %id = call token @llvm.coro.id(i32 0, ptr null, ptr null, ptr null)
  %need_alloc = call i1 @llvm.coro.alloc(token %id)
  br i1 %need_alloc, label %coro.alloc, label %coro.begin

coro.alloc:
  %size = call i64 @llvm.coro.size.i64()
  %mem = call ptr @malloc(i64 %size)
  br label %coro.begin

coro.begin:
  %phi.mem = phi ptr [ null, %entry ], [ %mem, %coro.alloc ]
  %hdl = call ptr @llvm.coro.begin(token %id, ptr %phi.mem)

  ; === NativeCoroState 初始化 (alloca 在 coro frame 内) ===
  %state = alloca %NativeCoroState, align 8
  store ptr null, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 0)  ; yielded_value
  store ptr null, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 1)  ; sent_value
  store ptr null, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 2)  ; thrown_exception
  store ptr null, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 3)  ; return_value
  store i32 0,   ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 4)   ; CREATED
  store i32 0,   ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 5)   ; SEND

  ; === 创建 PyNativeGenerator 包装 ===
  %gen_obj = call ptr @rt_create_native_generator(ptr %hdl, ptr %state, ptr @.str.gen)

  ; === 解包参数 ===
  %x = call ptr @rt_tuple_getitem(ptr %args, i32 0)

  ; === 初始挂起 (创建后立即挂起，返回 generator 对象给调用者) ===
  %tok0 = call token @llvm.coro.save(ptr %hdl)
  %sp0 = call i8 @llvm.coro.suspend(token %tok0, i1 false)
  switch i8 %sp0, label %coro.suspend [
    i8 0, label %resume.0
    i8 1, label %coro.cleanup
  ]

resume.0:
  ; === 恢复后: 检查 resume_action，可能需要 raise ===
  ; rt_generator_get_sent 内部检查 thrown_exception 并可能 throw C++ 异常
  ; 使用 invoke 以便 try/finally 能捕获
  %sent0 = invoke ptr @rt_generator_get_sent(ptr %state)
    to label %body.start unwind label %coro.unwind

body.start:
  ; === 函数体: y = yield (x + 1) ===
  %one = call ptr @rt_integer_from_i64(i64 1)
  %xp1 = invoke ptr @rt_binary_add(ptr %x, ptr %one)
    to label %yield.0 unwind label %coro.unwind

yield.0:
  ; yield x + 1: 设置 yielded_value 然后挂起
  call void @rt_generator_set_yield(ptr %state, ptr %xp1)
  %tok1 = call token @llvm.coro.save(ptr %hdl)
  %sp1 = call i8 @llvm.coro.suspend(token %tok1, i1 false)
  switch i8 %sp1, label %coro.suspend [
    i8 0, label %resume.1
    i8 1, label %coro.cleanup
  ]

resume.1:
  ; y = send 进来的值 (yield 表达式的值)
  %y = invoke ptr @rt_generator_get_sent(ptr %state)
    to label %body.1 unwind label %coro.unwind

body.1:
  ; === yield y * 2 ===
  %two = call ptr @rt_integer_from_i64(i64 2)
  %ym2 = invoke ptr @rt_binary_mul(ptr %y, ptr %two)
    to label %yield.1 unwind label %coro.unwind

yield.1:
  call void @rt_generator_set_yield(ptr %state, ptr %ym2)
  %tok2 = call token @llvm.coro.save(ptr %hdl)
  %sp2 = call i8 @llvm.coro.suspend(token %tok2, i1 false)
  switch i8 %sp2, label %coro.suspend [
    i8 0, label %resume.2
    i8 1, label %coro.cleanup
  ]

resume.2:
  %_discard = invoke ptr @rt_generator_get_sent(ptr %state)
    to label %body.ret unwind label %coro.unwind

body.ret:
  ; === return 42 → StopIteration(42) ===
  %ret = call ptr @rt_integer_from_i64(i64 42)
  call void @rt_generator_set_return(ptr %state, ptr %ret)
  br label %coro.cleanup

  ; === 未捕获异常的统一 unwind 路径 ===
coro.unwind:
  %lp = landingpad { ptr, i32 }
    cleanup
  ; 标记 done，然后 resume 传播异常给调用者
  call void @rt_generator_mark_done(ptr %state)
  resume { ptr, i32 } %lp

coro.cleanup:
  call void @rt_generator_mark_done(ptr %state)
  %mem.cleanup = call ptr @llvm.coro.free(token %id, ptr %hdl)
  call void @free(ptr %mem.cleanup)
  br label %coro.suspend

coro.suspend:
  call i1 @llvm.coro.end(ptr %hdl, i1 false, token none)
  ret ptr %gen_obj
}
```

### 5.2 yield 在 try/finally 内

```python
def gen():
    try:
        yield 1
        yield 2
    finally:
        cleanup()
```

关键点：`rt_generator_get_sent` 可能抛出 C++ 异常（当 caller 调用 `throw()` 或 `close()` 时），
这个异常会被 generator 内部的 `try/finally` 的 landingpad 捕获，finally 块正常执行。

```llvm
resume.0:
  %sent = invoke ptr @rt_generator_get_sent(ptr %state)
    to label %try.body unwind label %try.landingpad

try.body:
  ; yield 1
  call void @rt_generator_set_yield(ptr %state, ptr %v1)
  %tok = call token @llvm.coro.save(ptr %hdl)
  %sp = call i8 @llvm.coro.suspend(token %tok, i1 false)
  switch i8 %sp, label %coro.suspend [
    i8 0, label %resume.1
    i8 1, label %coro.cleanup
  ]

resume.1:
  ; 恢复后 get_sent 也用 invoke → 如果 throw(GeneratorExit)，跳到 landingpad
  %sent1 = invoke ptr @rt_generator_get_sent(ptr %state)
    to label %try.body.cont unwind label %try.landingpad

try.body.cont:
  ; yield 2 ...
  br label %try.finally.normal

try.landingpad:
  %lp = landingpad { ptr, i32 }
    cleanup
  ; 保存 landingpad token
  store { ptr, i32 } %lp, ptr %lp_save
  br label %try.finally.exc

try.finally.normal:
  ; finally: cleanup() — 正常路径
  invoke ptr @rt_call(ptr %cleanup_fn, ...)
    to label %try.done unwind label %coro.unwind
  br label %try.done

try.finally.exc:
  ; finally: cleanup() — 异常路径
  invoke ptr @rt_call(ptr %cleanup_fn, ...)
    to label %try.reraise unwind label %coro.unwind

try.reraise:
  ; 重新抛出保存的异常
  %saved_lp = load { ptr, i32 }, ptr %lp_save
  resume { ptr, i32 } %saved_lp
```

### 5.3 generator.throw() 机制

`throw()` 通过 `NativeCoroState` 的 `thrown_exception` + `resume_action` 字段实现：

```
caller 调用 gen.throw(exc):
  1. state->thrown_exception = exc
  2. state->resume_action = ResumeAction::THROW
  3. coro.resume(hdl)  ← 正常恢复 coroutine

generator 恢复后调用 rt_generator_get_sent():
  1. 检查 resume_action
  2. 如果 THROW → 取出 thrown_exception → throw PylangException(exc)
  3. C++ 异常沿正常 EH 路径传播
  4. 如果 generator 内有 try/except → 可以捕获
  5. 如果没有 → 异常传播到 coro.unwind → resume → 回到 caller 的 catch
```

### 5.4 generator.close() 机制

```
close() 语义 (PEP 342):
  1. 如果 status == DONE → return None
  2. 如果 status == CREATED → 标记 DONE, return None
  3. throw(GeneratorExit) 进 generator
  4. 如果 generator 正常 return 或抛出 GeneratorExit/StopIteration → return None
  5. 如果 generator yield 了 → RuntimeError("generator ignored GeneratorExit")
  6. 如果 generator 抛出其他异常 → 传播给 caller
```

### 5.5 PEP 479: StopIteration 防泄漏

Python 3.7+ 强制：generator 内部未捕获的 `StopIteration` 自动转为 `RuntimeError`。

实现方式：在 `PyNativeGenerator::send()` 的 catch 块中检查：

```cpp
try {
    resume(m_coro_handle);
} catch (PylangException &e) {
    m_state->status = CoroStatus::DONE;
    // PEP 479
    if (e.exc->type()->issubclass(stop_iteration()->type())) {
        return Err(runtime_error("generator raised StopIteration"));
    }
    return Err(e.exc);
}
```

## 6. yield from 完整语义 (Phase 2)

### 6.1 Python 3.9 yield from 规范 (PEP 380)

```python
# RESULT = yield from EXPR 等价于:
_i = iter(EXPR)
try:
    _y = next(_i)
except StopIteration as _e:
    _r = _e.value
else:
    while True:
        try:
            _s = yield _y          # 挂起，等待 send/throw
        except GeneratorExit as _e:
            try:
                _m = _i.close
            except AttributeError:
                pass
            else:
                _m()
            raise _e
        except BaseException as _e:
            _x = sys.exc_info()
            try:
                _m = _i.throw
            except AttributeError:
                raise _e
            else:
                try:
                    _y = _m(*_x)
                except StopIteration as _e:
                    _r = _e.value
                    break
        else:
            try:
                if _s is None:
                    _y = next(_i)
                else:
                    _y = _i.send(_s)
            except StopIteration as _e:
                _r = _e.value
                break
RESULT = _r
```

### 6.2 AOT 实现策略

yield from 不能编译为单个 runtime 调用，因为每次内层迭代器 yield 时，外层 generator 也必须挂起。
编译器将 `yield from` 展开为 **循环 + 多个 coro.suspend 点 + 异常分派**。

核心思路：编译器生成的 IR 精确对应 PEP 380 的伪代码，每个 `yield _y` 对应一个 `coro.suspend`。

#### 6.2.1 完整 IR 展开

```python
RESULT = yield from EXPR
```

展开为：

```llvm
yield_from.init:
  %iter = call ptr @rt_get_iter(ptr %expr)
  ; 首次 next(iter) — 可能立即 StopIteration
  %first_result = invoke ptr @rt_yield_from_next(ptr %iter)
    to label %yield_from.loop unwind label %yield_from.first_stop

yield_from.first_stop:
  ; StopIteration → 提取 .value 作为 yield from 的结果
  %lp0 = landingpad { ptr, i32 } catch ptr @_ZTI15PylangException
  %exc_ptr0 = extractvalue { ptr, i32 } %lp0, 0
  %exc0 = call ptr @rt_catch_begin(ptr %exc_ptr0)
  %is_stop0 = call i1 @rt_is_stopiteration(ptr %exc0)
  br i1 %is_stop0, label %yield_from.extract_value, label %yield_from.reraise_init

yield_from.extract_value:
  %result_from_stop = call ptr @rt_stopiteration_value(ptr %exc0)
  call void @rt_catch_end()
  br label %yield_from.done

yield_from.reraise_init:
  ; 非 StopIteration 异常 → 传播
  call void @rt_catch_end()
  resume { ptr, i32 } %lp0

yield_from.loop:
  ; phi: 当前要 yield 出去的值
  %y = phi ptr [ %first_result, %yield_from.init ],
               [ %next_y, %yield_from.send_ok ],
               [ %throw_y, %yield_from.throw_ok ]

  ; === 挂起外层 generator，yield _y 给 caller ===
  call void @rt_generator_set_yield(ptr %state, ptr %y)
  %tok = call token @llvm.coro.save(ptr %hdl)
  %sp = call i8 @llvm.coro.suspend(token %tok, i1 false)
  switch i8 %sp, label %coro.suspend [
    i8 0, label %yield_from.resumed
    i8 1, label %coro.cleanup
  ]

yield_from.resumed:
  ; 恢复后检查 resume_action
  %action = load i32, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 5)

  ; switch: SEND=0, THROW=1, CLOSE=2
  switch i32 %action, label %yield_from.do_send [
    i32 0, label %yield_from.do_send
    i32 1, label %yield_from.do_throw
    i32 2, label %yield_from.do_close
  ]

  ; === SEND 路径 ===
yield_from.do_send:
  %sent = load ptr, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 1)
  %is_none = call i1 @rt_compare_is(ptr %sent, ptr @rt_none())
  br i1 %is_none, label %yield_from.send_next, label %yield_from.send_value

yield_from.send_next:
  ; _s is None → next(_i)
  %next_y = invoke ptr @rt_yield_from_next(ptr %iter)
    to label %yield_from.send_ok unwind label %yield_from.send_stop

yield_from.send_value:
  ; _s is not None → _i.send(_s)
  %next_y_send = invoke ptr @rt_yield_from_send(ptr %iter, ptr %sent)
    to label %yield_from.send_ok unwind label %yield_from.send_stop

yield_from.send_ok:
  %next_y = phi ptr [ %next_y, %yield_from.send_next ],
                     [ %next_y_send, %yield_from.send_value ]
  br label %yield_from.loop

yield_from.send_stop:
  ; StopIteration from next/send → 提取 .value，结束循环
  %lp_send = landingpad { ptr, i32 } catch ptr @_ZTI15PylangException
  %exc_ptr_send = extractvalue { ptr, i32 } %lp_send, 0
  %exc_send = call ptr @rt_catch_begin(ptr %exc_ptr_send)
  %is_stop_send = call i1 @rt_is_stopiteration(ptr %exc_send)
  br i1 %is_stop_send, label %yield_from.send_stop_ok, label %yield_from.send_reraise

yield_from.send_stop_ok:
  %result_send = call ptr @rt_stopiteration_value(ptr %exc_send)
  call void @rt_catch_end()
  br label %yield_from.done

yield_from.send_reraise:
  call void @rt_catch_end()
  resume { ptr, i32 } %lp_send

  ; === THROW 路径 ===
yield_from.do_throw:
  %thrown = load ptr, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 2)
  ; 清除 thrown_exception（已取出）
  store ptr null, ptr getelementptr(%NativeCoroState, ptr %state, i32 0, i32 2)

  ; 尝试 _i.throw(exc)
  %throw_y = invoke ptr @rt_yield_from_throw(ptr %iter, ptr %thrown)
    to label %yield_from.throw_ok unwind label %yield_from.throw_stop

yield_from.throw_ok:
  br label %yield_from.loop  ; 子迭代器 yield 了新值

yield_from.throw_stop:
  %lp_throw = landingpad { ptr, i32 } catch ptr @_ZTI15PylangException
  %exc_ptr_throw = extractvalue { ptr, i32 } %lp_throw, 0
  %exc_throw = call ptr @rt_catch_begin(ptr %exc_ptr_throw)
  %is_stop_throw = call i1 @rt_is_stopiteration(ptr %exc_throw)
  br i1 %is_stop_throw, label %yield_from.throw_stop_ok, label %yield_from.throw_reraise

yield_from.throw_stop_ok:
  %result_throw = call ptr @rt_stopiteration_value(ptr %exc_throw)
  call void @rt_catch_end()
  br label %yield_from.done

yield_from.throw_reraise:
  ; 非 StopIteration → 传播（包括子迭代器没有 .throw 方法的情况）
  call void @rt_catch_end()
  resume { ptr, i32 } %lp_throw

  ; === CLOSE 路径 (GeneratorExit) ===
yield_from.do_close:
  ; 尝试 _i.close()
  invoke void @rt_yield_from_close(ptr %iter)
    to label %yield_from.close_ok unwind label %yield_from.close_fail

yield_from.close_ok:
  ; close 成功 → 重新抛出 GeneratorExit 给外层
  %gen_exit = call ptr @rt_create_generator_exit()
  call void @rt_raise(ptr %gen_exit)
  unreachable

yield_from.close_fail:
  ; close 抛出异常 → 传播
  %lp_close = landingpad { ptr, i32 } cleanup
  resume { ptr, i32 } %lp_close

  ; === 结果汇合 ===
yield_from.done:
  %RESULT = phi ptr [ %result_from_stop, %yield_from.extract_value ],
                     [ %result_send, %yield_from.send_stop_ok ],
                     [ %result_throw, %yield_from.throw_stop_ok ]
  ; RESULT 就是 yield from 表达式的值
```

#### 6.2.2 yield from 辅助 runtime 函数

这些函数封装了子迭代器的协议调用，处理 `AttributeError`（没有 `.send`/`.throw` 方法）的情况：

```cpp
// rt_yield_from_next(iter) → PyObject*
//   等同于 next(iter)，StopIteration 不捕获（让 landingpad 处理）

// rt_yield_from_send(iter, value) → PyObject*
//   尝试 iter.send(value)
//   如果 iter 没有 .send 方法 → 退回 next(iter)（忽略 value）

// rt_yield_from_throw(iter, exc) → PyObject*
//   尝试 iter.throw(type(exc), exc, exc.__traceback__)
//   如果 iter 没有 .throw 方法 → raise exc（直接抛出）

// rt_yield_from_close(iter) → void
//   尝试 iter.close()
//   如果 iter 没有 .close 方法 → 忽略（AttributeError 被吞掉）
```

## 7. Coroutine (Phase 3)

### 7.1 async def 编译

`async def` 与 generator 函数的 LLVM coroutine 结构完全相同，区别仅在于：

1. 函数属性标记为 `presplitcoroutine`（相同）
2. 创建包装对象时调用 `rt_create_native_coroutine` 而非 `rt_create_native_generator`
3. `await expr` 编译为 `yield from get_awaitable(expr)`

### 7.2 await 编译

```python
result = await expr
```

等价于：

```python
result = yield from _get_awaitable(expr)
```

IR 生成：

```llvm
  %obj = <generate expr>
  %awaitable = call ptr @rt_get_awaitable(ptr %obj)
  ; 然后生成与 yield from 完全相同的 IR，iter = awaitable
```

### 7.3 GET_AWAITABLE 语义

```
rt_get_awaitable(obj):
  1. 如果 obj 是 native coroutine (PyNativeCoroutine) → 直接返回
  2. 如果 obj 是 bytecode coroutine (PyCoroutine) → 直接返回
  3. 如果 obj 有 __await__ 方法 → 调用并返回结果
  4. 否则 → TypeError("object X can't be used in 'await' expression")
```

### 7.4 PyNativeCoroutine 与 PyNativeGenerator 的差异

| 特性 | PyNativeGenerator | PyNativeCoroutine |
|:-----|:------------------|:------------------|
| `__iter__` | 返回 self | TypeError |
| `__next__` | = send(None) | TypeError |
| `__await__` | TypeError | 返回 self |
| `send()` | ✅ | ✅ |
| `throw()` | ✅ | ✅ |
| `close()` | ✅ | ✅ |
| 属性前缀 | `gi_` | `cr_` |
| 类型名 | `"generator"` | `"coroutine"` |
| PEP 479 | ✅ StopIteration → RuntimeError | ✅ |

## 8. Async Generator (Phase 4)

### 8.1 async def + yield = async generator

```python
async def agen():
    yield 1
    await something()
    yield 2
```

编译器检测：函数同时有 `async` 标记和 `yield` 节点 → async generator。

LLVM coroutine 结构与 generator 相同，区别：
- 创建时调用 `rt_create_native_async_generator`
- yield 的值通过 `__anext__` 返回的 awaitable 传递
- 迭代结束抛 `StopAsyncIteration` 而非 `StopIteration`

### 8.2 PyNativeAsyncGenerator 协议

```
__aiter__()  → return self
__anext__()  → return awaitable that resolves to next yielded value
               raises StopAsyncIteration when done

asend(value) → return awaitable
athrow(type, value=None, tb=None) → return awaitable
aclose()     → return awaitable
```

每个异步方法返回一个 **awaitable 包装对象**（`PyAsyncGenWrappedValue`），
它本身是一个 coroutine-like 对象，支持 `__await__` 协议。

#### 8.2.1 Awaitable 包装器

async generator 的 `__anext__()` / `asend()` / `athrow()` / `aclose()` 不能直接返回值，
必须返回一个 awaitable。这个 awaitable 在被 `await` 时执行实际的 send/throw 操作。

```cpp
// PyAsyncGenASend — asend(value) 返回的 awaitable
// PyAsyncGenAThrow — athrow(type, value, tb) 返回的 awaitable
//
// 这两个类型实现 __await__ 协议：
//   __await__() → return self
//   __next__()  → 执行一步 send/throw，返回 yield 值或抛 StopAsyncIteration
//   send(v)     → 同上
//   throw(e)    → 同上
//   close()     → 同上
```

#### 8.2.2 async generator yield 语义差异

| 行为 | generator | async generator |
|:-----|:----------|:----------------|
| 迭代结束 | `StopIteration` | `StopAsyncIteration` |
| `return value` | `StopIteration(value)` | `StopAsyncIteration`（value 被忽略） |
| PEP 479 | `StopIteration` → `RuntimeError` | `StopIteration` → `RuntimeError` |
| `yield` 在 `async for` 中 | N/A | 值通过 awaitable 传递 |
| `await` 在 body 中 | 不允许 | 允许（挂起外层 coroutine） |

#### 8.2.3 async generator 的双层挂起

async generator 有两层挂起机制：

1. **yield 挂起**：`yield value` 将值传给 `__anext__` 的 awaitable，awaitable 完成
2. **await 挂起**：`await expr` 挂起外层 coroutine（事件循环层面），async generator 本身不感知

编译器处理：
- `yield value` → 与普通 generator 相同的 `coro.suspend`
- `await expr` → 展开为 `yield from get_awaitable(expr)`（与 coroutine 中的 await 相同）

两者共享同一个 LLVM coroutine frame，`coro.suspend` 不区分是 yield 还是 await。
区分由 runtime 的 `PyAsyncGenASend` 包装器处理：
- 如果 resume 后 `NativeCoroState.yielded_value` 非 null → 这是一个 yield，awaitable 完成
- 如果 resume 后 `yielded_value` 为 null 但 `status != DONE` → 这是一个 await 中间挂起，
  awaitable 需要继续 yield 给外层 coroutine

#### 8.2.4 NativeCoroState 扩展

```cpp
struct NativeCoroState
{
    PyObject *yielded_value;     // generator → caller
    PyObject *sent_value;        // caller → generator
    PyObject *thrown_exception;  // caller → generator
    PyObject *return_value;      // generator return value
    CoroStatus status;           // 生命周期状态
    ResumeAction resume_action;  // 恢复时的操作类型
    bool is_yield_point;         // true = yield 挂起, false = await 中间挂起
};
```

`is_yield_point` 由编译器在每个 `coro.suspend` 前设置：
- `yield value` → `state->is_yield_point = true; state->yielded_value = value;`
- `await` 内部的 `yield from` 的 yield → `state->is_yield_point = false;`

### 8.3 async for 编译

```python
async for x in agen():
    body
else:
    else_body
```

等价于：

```python
__aiter = agen().__aiter__()
while True:
    try:
        x = await __aiter.__anext__()
    except StopAsyncIteration:
        break
    body
else:
    else_body
```

编译器展开为：

```llvm
async_for.init:
  %agen = <generate agen()>
  %aiter = call ptr @rt_get_aiter(ptr %agen)
  br label %async_for.loop

async_for.loop:
  ; anext_aw = aiter.__anext__()
  %anext_aw = call ptr @rt_async_gen_anext(ptr %aiter)
  ; x = await anext_aw  (展开为 yield from)
  ; ... yield from IR (同 §6.2.1) ...
  ; 如果 StopAsyncIteration → 跳到 else/merge
  ; 如果正常 → x = result, 执行 body

async_for.body:
  ; body
  br label %async_for.loop

async_for.else:
  ; else_body
  br label %async_for.merge

async_for.stop:
  ; landingpad 捕获 StopAsyncIteration
  %exc = ...
  %is_stop_async = call i1 @rt_is_stop_async_iteration(ptr %exc)
  br i1 %is_stop_async, label %async_for.else, label %async_for.reraise

async_for.reraise:
  resume ...

async_for.merge:
  ; 继续
```

### 8.4 async with 编译

```python
async with expr as var:
    body
```

等价于：

```python
mgr = expr
aexit = type(mgr).__aexit__
aenter = type(mgr).__aenter__
var = await aenter(mgr)
exc = True
try:
    body
    exc = False
except:
    if not await aexit(mgr, *sys.exc_info()):
        raise
else:
    await aexit(mgr, None, None, None)
```

与同步 `with` 的区别仅在于 `__aenter__` / `__aexit__` 的调用结果需要 `await`。
编译器复用 `visit(With*)` 的结构，将 `emit_call("call", ...)` 替换为
`emit_call("call", ...) + yield from get_awaitable(result)`。

## 9. Runtime 导出函数完整列表

### 9.1 `rt_coro.cpp` — 分类: `coro`

```
创建:
  rt_create_native_generator(hdl, state, name)           → obj    (ptr,ptr,str)
  rt_create_native_coroutine(hdl, state, name)           → obj    (ptr,ptr,str)
  rt_create_native_async_generator(hdl, state, name)     → obj    (ptr,ptr,str)

通信 (generator 函数内部调用):
  rt_generator_set_yield(state, value)                   → void   (ptr,obj)
  rt_generator_get_sent(state)                           → obj    (ptr)  [可能抛异常]
  rt_generator_set_return(state, value)                  → void   (ptr,obj)
  rt_generator_mark_done(state)                          → void   (ptr)
  rt_generator_set_yield_point(state, is_yield)          → void   (ptr,bool)

协议:
  rt_get_awaitable(obj)                                  → obj    (obj)
  rt_get_aiter(obj)                                      → obj    (obj)
  rt_stopiteration_value(exc)                            → obj    (obj)

异常检查:
  rt_is_stopiteration(exc)                               → bool   (obj)
  rt_is_stop_async_iteration(exc)                        → bool   (obj)
  rt_is_generator_exit(exc)                              → bool   (obj)
  rt_create_generator_exit()                             → obj    ()
  rt_create_stop_iteration(value)                        → obj    (obj)
  rt_create_stop_async_iteration()                       → obj    ()

yield from 支持:
  rt_yield_from_next(iter)                               → obj    (obj)
  rt_yield_from_send(iter, value)                        → obj    (obj,obj)
  rt_yield_from_throw(iter, exc)                         → obj    (obj,obj)
  rt_yield_from_close(iter)                              → void   (obj)

async generator 支持:
  rt_async_gen_anext(agen)                               → obj    (obj)
  rt_async_gen_asend(agen, value)                        → obj    (obj,obj)
  rt_async_gen_athrow(agen, exc)                         → obj    (obj,obj)
  rt_async_gen_aclose(agen)                              → obj    (obj)

identity 检查 (yield from 需要):
  rt_compare_is(a, b)                                   → bool   (obj,obj)
```

### 9.2 导出宏

```cpp
// export.hpp 新增:
#define PYLANG_EXPORT_CORO(name, ret, params) \
    PYLANG_EXPORT_IMPL_("coro", name, ret, params)
```

## 10. 编译器检测: generator / coroutine / async generator

### 10.1 函数分类规则

编译器在 `visit(FunctionDefinition*)` / `visit(AsyncFunctionDefinition*)` 时，
需要扫描函数体确定函数类型：

| 条件 | 函数类型 | 创建函数 |
|:-----|:---------|:---------|
| 无 `async`，无 `yield` | 普通函数 | 不使用 coro |
| 无 `async`，有 `yield` | generator | `rt_create_native_generator` |
| 有 `async`，无 `yield` | coroutine | `rt_create_native_coroutine` |
| 有 `async`，有 `yield` | async generator | `rt_create_native_async_generator` |

### 10.2 AST 扫描

```cpp
/// 递归扫描函数体，检测是否包含 yield/yield from
/// 注意：不进入嵌套函数/类定义（它们有自己的 scope）
bool contains_yield(const std::vector<std::shared_ptr<ast::ASTNode>> &body)
{
    for (const auto &node : body) {
        if (node->node_type() == ASTNodeType::Yield) return true;
        if (node->node_type() == ASTNodeType::YieldFrom) return true;
        // 递归进入 if/for/while/try/with 的子块
        // 但不进入 FunctionDefinition/AsyncFunctionDefinition/ClassDefinition/Lambda
        ...
    }
    return false;
}
```

实际上 `VariablesResolver` 已经做了这个分析（`is_generator` 标记），
编译器可以直接从 `var_scope->is_generator` 获取。

## 11. IREmitter 新增方法

```cpp
// IREmitter.hpp 新增（在 Tier 6 之后）

// ========== Tier 7: 协程/生成器 ==========

// --- 11.1 LLVM Coroutine Intrinsics 封装 ---

/// 声明所有 coro intrinsics + malloc/free
/// 在 compile_module() 开始时调用（如果模块包含 generator/coroutine）
void declare_coro_intrinsics();

/// @llvm.coro.id(i32 0, ptr null, ptr null, ptr null) → token
llvm::Value *emit_coro_id();

/// @llvm.coro.alloc(token %id) → i1
llvm::Value *emit_coro_alloc(llvm::Value *id);

/// @llvm.coro.size.i64() → i64
llvm::Value *emit_coro_size();

/// @llvm.coro.begin(token %id, ptr %mem) → ptr (coro handle)
llvm::Value *emit_coro_begin(llvm::Value *id, llvm::Value *mem);

/// @llvm.coro.save(ptr %hdl) → token
llvm::Value *emit_coro_save(llvm::Value *hdl);

/// @llvm.coro.suspend(token %save, i1 %is_final) → i8
/// 返回值: 0=resumed, 1=cleanup
llvm::Value *emit_coro_suspend(llvm::Value *save_token, bool is_final = false);

/// @llvm.coro.end(ptr %hdl, i1 false, token none) → i1
llvm::Value *emit_coro_end(llvm::Value *hdl);

/// @llvm.coro.free(token %id, ptr %hdl) → ptr
llvm::Value *emit_coro_free(llvm::Value *id, llvm::Value *hdl);

// --- 11.2 NativeCoroState 操作 ---

/// 创建 NativeCoroState 的 LLVM struct type
/// { ptr, ptr, ptr, ptr, i32, i32, i1 }
/// 对应 { yielded_value, sent_value, thrown_exception, return_value,
///        status(CoroStatus), resume_action(ResumeAction), is_yield_point }
llvm::StructType *get_coro_state_type();

/// 在当前函数 entry block 创建 NativeCoroState alloca 并零初始化
llvm::AllocaInst *emit_coro_state_alloca();

/// 存储 yielded_value 到 state (GEP index 0)
void emit_coro_state_set_yield(llvm::Value *state, llvm::Value *value);

/// 加载 sent_value 从 state (GEP index 1)
llvm::Value *emit_coro_state_get_sent(llvm::Value *state);

/// 存储 return_value 到 state (GEP index 3)
void emit_coro_state_set_return(llvm::Value *state, llvm::Value *value);

/// 加载 status 从 state (GEP index 4)
llvm::Value *emit_coro_state_get_status(llvm::Value *state);

/// 存储 status 到 state (GEP index 4)
void emit_coro_state_set_status(llvm::Value *state, llvm::Value *status);

/// 加载 resume_action 从 state (GEP index 5)
llvm::Value *emit_coro_state_get_resume_action(llvm::Value *state);

/// 存储 is_yield_point 到 state (GEP index 6)
void emit_coro_state_set_yield_point(llvm::Value *state, bool is_yield);

// --- 11.3 Runtime 调用封装 ---

/// call @rt_create_native_generator(ptr %hdl, ptr %state, str %name) → obj
llvm::Value *call_create_native_generator(llvm::Value *hdl,
                                           llvm::Value *state,
                                           std::string_view name);

/// call @rt_create_native_coroutine(ptr %hdl, ptr %state, str %name) → obj
llvm::Value *call_create_native_coroutine(llvm::Value *hdl,
                                           llvm::Value *state,
                                           std::string_view name);

/// call @rt_create_native_async_generator(ptr %hdl, ptr %state, str %name) → obj
llvm::Value *call_create_native_async_generator(llvm::Value *hdl,
                                                 llvm::Value *state,
                                                 std::string_view name);

/// call @rt_generator_set_yield(ptr %state, obj %value) → void
void call_generator_set_yield(llvm::Value *state, llvm::Value *value);

/// invoke @rt_generator_get_sent(ptr %state) → obj
/// 注意: 此函数可能抛异常 (throw 进来的)，必须用 invoke
llvm::Value *call_generator_get_sent(llvm::Value *state);

/// call @rt_generator_set_return(ptr %state, obj %value) → void
void call_generator_set_return(llvm::Value *state, llvm::Value *value);

/// call @rt_generator_mark_done(ptr %state) → void
void call_generator_mark_done(llvm::Value *state);

/// call @rt_get_awaitable(obj) → obj
llvm::Value *call_get_awaitable(llvm::Value *obj);

/// call @rt_get_aiter(obj) → obj
llvm::Value *call_get_aiter(llvm::Value *obj);

/// call @rt_yield_from_next(obj %iter) → obj  [可能抛 StopIteration]
llvm::Value *call_yield_from_next(llvm::Value *iter);

/// call @rt_yield_from_send(obj %iter, obj %value) → obj
llvm::Value *call_yield_from_send(llvm::Value *iter, llvm::Value *value);

/// call @rt_yield_from_throw(obj %iter, obj %exc) → obj
llvm::Value *call_yield_from_throw(llvm::Value *iter, llvm::Value *exc);

/// call @rt_yield_from_close(obj %iter) → void
void call_yield_from_close(llvm::Value *iter);

/// call @rt_is_stopiteration(obj %exc) → i1
llvm::Value *call_is_stopiteration(llvm::Value *exc);

/// call @rt_is_stop_async_iteration(obj %exc) → i1
llvm::Value *call_is_stop_async_iteration(llvm::Value *exc);

/// call @rt_stopiteration_value(obj %exc) → obj
llvm::Value *call_stopiteration_value(llvm::Value *exc);

/// call @rt_create_generator_exit() → obj
llvm::Value *call_create_generator_exit();

/// call @rt_compare_is(obj %a, obj %b) → i1
llvm::Value *call_compare_is(llvm::Value *a, llvm::Value *b);
```

### 11.4 IREmitter 内部成员

```cpp
// IREmitter.hpp private 区域新增:

/// coro intrinsics 是否已声明
bool m_coro_intrinsics_declared = false;

/// NativeCoroState struct type (lazy 创建)
llvm::StructType *m_coro_state_type = nullptr;

/// malloc/free 声明 (coro frame 分配)
llvm::FunctionCallee m_malloc_fn;
llvm::FunctionCallee m_free_fn;
```

## 12. CodegenContext 新增: CoroContext

```cpp
// CodegenContext.hpp 新增

/// 协程/生成器编译上下文
/// 在 compile_generator_body() 期间有效
struct CoroContext
{
    llvm::Value *coro_handle;       // llvm.coro.begin 返回的 handle
    llvm::Value *coro_id;           // llvm.coro.id 返回的 token
    llvm::AllocaInst *state_alloca; // NativeCoroState alloca
    llvm::Value *gen_obj;           // PyNativeGenerator/Coroutine/AsyncGenerator 对象
    llvm::BasicBlock *cleanup_bb;   // coro.cleanup 块
    llvm::BasicBlock *suspend_bb;   // coro.suspend 块
    llvm::BasicBlock *unwind_bb;    // coro.unwind 块 (未捕获异常)

    enum class Kind : uint8_t
    {
        GENERATOR,
        COROUTINE,
        ASYNC_GENERATOR,
    } kind;
};

// CodegenContext 类新增:
class CodegenContext
{
    // ...existing members...

    /// 当前协程上下文 (仅在 generator/coroutine/async_generator 函数内有效)
    std::optional<CoroContext> m_coro_ctx;

public:
    // ...existing methods...

    void set_coro_context(CoroContext ctx) { m_coro_ctx = std::move(ctx); }
    void clear_coro_context() { m_coro_ctx.reset(); }
    bool in_coroutine() const { return m_coro_ctx.has_value(); }
    CoroContext &coro_ctx() { return m_coro_ctx.value(); }
    const CoroContext &coro_ctx() const { return m_coro_ctx.value(); }
};
```

## 13. PylangCodegen 新增方法

```cpp
// PylangCodegen.hpp 新增

// --- 13.1 协程框架生成 ---

/// 生成 coro prologue: coro.id → coro.alloc → coro.begin → state init → 初始 suspend
/// 返回 { coro_handle, state_alloca, gen_obj }
/// 在 compile_function_body() 中调用（当函数是 generator/coroutine/async_generator 时）
void emit_coro_prologue(CoroContext::Kind kind, std::string_view func_name);

/// 生成 coro epilogue: cleanup_bb + suspend_bb + unwind_bb
/// 在函数体编译完成后调用
void emit_coro_epilogue();

/// 生成单个 yield 的 suspend 序列:
///   set_yield(state, value) → coro.save → coro.suspend → switch
/// 返回恢复后的 BB（resume point）
/// yield 表达式的值 = get_sent() 的返回值
llvm::BasicBlock *emit_yield_suspend(llvm::Value *yield_value);

// --- 13.2 yield from 展开 ---

/// 生成完整的 yield from IR (§6.2.1)
/// 返回 yield from 表达式的值 (StopIteration.value)
llvm::Value *emit_yield_from(llvm::Value *iterable);

// --- 13.3 visit 方法 ---

ast::Value *visit(const ast::Yield *node) override;
ast::Value *visit(const ast::YieldFrom *node) override;
ast::Value *visit(const ast::Await *node) override;
ast::Value *visit(const ast::AsyncFunctionDefinition *node) override;
ast::Value *visit(const ast::AsyncFor *node) override;
ast::Value *visit(const ast::AsyncWith *node) override;
```

### 13.4 visit(FunctionDefinition*) 修改

```cpp
// 现有 visit(FunctionDefinition*) 需要修改的部分:

ast::Value *PylangCodegen::visit(const ast::FunctionDefinition *node)
{
    // ...existing code: 创建 LLVM function, 设置参数...

    bool is_generator = contains_yield(node->body());
    // 或者: bool is_generator = var_scope->is_generator;

    if (is_generator) {
        // 添加 presplitcoroutine 属性
        func->addFnAttr("presplitcoroutine");
        // 声明 coro intrinsics (如果尚未声明)
        m_emitter.declare_coro_intrinsics();

        emit_coro_prologue(CoroContext::Kind::GENERATOR, node->name());

        // 编译函数体 (yield 节点会使用 coro context)
        generate_body(node->body());

        // 函数体正常结束 = generator return (无显式 return value → StopIteration(None))
        if (!m_builder.GetInsertBlock()->getTerminator()) {
            m_emitter.call_generator_set_return(
                m_codegen_ctx.coro_ctx().state_alloca, m_emitter.get_none());
            m_builder.CreateBr(m_codegen_ctx.coro_ctx().cleanup_bb);
        }

        emit_coro_epilogue();
    } else {
        // ...existing code: 普通函数编译...
    }
}
```

### 13.5 visit(Yield*) 实现

```cpp
ast::Value *PylangCodegen::visit(const ast::Yield *node)
{
    assert(m_codegen_ctx.in_coroutine() && "yield outside generator");

    // 计算 yield 的值
    llvm::Value *yield_val = nullptr;
    if (node->value()) {
        yield_val = generate(node->value().get());
    } else {
        yield_val = m_emitter.get_none();
    }
    if (!yield_val) return nullptr;

    // 生成 suspend 序列，返回恢复后的 BB
    auto *resume_bb = emit_yield_suspend(yield_val);

    // yield 表达式的值 = get_sent() 返回值
    // get_sent 可能抛异常 (throw 进来的)，需要 invoke
    auto &coro = m_codegen_ctx.coro_ctx();
    auto *sent = m_emitter.call_generator_get_sent(coro.state_alloca);
    // call_generator_get_sent 内部使用 invoke → unwind 到 coro.unwind_bb
    // 如果当前在 try 块内，unwind 到 try 的 landingpad

    return sent;
}
```

### 13.6 visit(YieldFrom*) 实现

```cpp
ast::Value *PylangCodegen::visit(const ast::YieldFrom *node)
{
    assert(m_codegen_ctx.in_coroutine() && "yield from outside generator");

    auto *iterable = generate(node->value().get());
    if (!iterable) return nullptr;

    return emit_yield_from(iterable);
}
```

### 13.7 visit(Await*) 实现

```cpp
ast::Value *PylangCodegen::visit(const ast::Await *node)
{
    assert(m_codegen_ctx.in_coroutine() && "await outside coroutine");

    auto *expr = generate(node->value().get());
    if (!expr) return nullptr;

    // await expr = yield from get_awaitable(expr)
    auto *awaitable = m_emitter.call_get_awaitable(expr);
    return emit_yield_from(awaitable);
}
```

### 13.8 visit(AsyncFunctionDefinition*) 实现

```cpp
ast::Value *PylangCodegen::visit(const ast::AsyncFunctionDefinition *node)
{
    // 与 visit(FunctionDefinition*) 几乎相同
    // 区别:
    //   1. 检测 yield → 决定是 coroutine 还是 async_generator
    //   2. Kind = COROUTINE 或 ASYNC_GENERATOR
    //   3. 创建函数时调用对应的 rt_create_native_coroutine/async_generator

    bool has_yield = contains_yield(node->body());
    auto kind = has_yield ? CoroContext::Kind::ASYNC_GENERATOR
                          : CoroContext::Kind::COROUTINE;

    // ...创建 LLVM function, 添加 presplitcoroutine 属性...

    emit_coro_prologue(kind, node->name());
    generate_body(node->body());

    if (!m_builder.GetInsertBlock()->getTerminator()) {
        if (kind == CoroContext::Kind::ASYNC_GENERATOR) {
            // async generator return → StopAsyncIteration (value 被忽略)
            m_builder.CreateBr(m_codegen_ctx.coro_ctx().cleanup_bb);
        } else {
            // coroutine return None
            m_emitter.call_generator_set_return(
                m_codegen_ctx.coro_ctx().state_alloca, m_emitter.get_none());
            m_builder.CreateBr(m_codegen_ctx.coro_ctx().cleanup_bb);
        }
    }

    emit_coro_epilogue();
}
```

### 13.9 visit(AsyncFor*) 实现

```cpp
ast::Value *PylangCodegen::visit(const ast::AsyncFor *node)
{
    // async for x in expr:
    //     body
    // else:
    //     else_body

    auto *func = m_codegen_ctx.current_function();
    auto *iterable = generate(node->iter().get());
    auto *aiter = m_emitter.call_get_aiter(iterable);

    auto *loop_bb = llvm::BasicBlock::Create(m_ctx, "async_for.loop", func);
    auto *body_bb = llvm::BasicBlock::Create(m_ctx, "async_for.body", func);
    auto *else_bb = llvm::BasicBlock::Create(m_ctx, "async_for.else", func);
    auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "async_for.merge", func);
    auto *stop_bb = llvm::BasicBlock::Create(m_ctx, "async_for.stop", func);

    m_builder.CreateBr(loop_bb);
    m_builder.SetInsertPoint(loop_bb);

    // anext_aw = aiter.__anext__()
    auto *anext_str = m_emitter.create_string("__anext__");
    auto *anext_method = m_emitter.emit_runtime_call("getattr", { aiter, anext_str });
    auto *empty_args = m_emitter.create_tuple({});
    auto *anext_aw = m_emitter.emit_runtime_call(
        "call", { anext_method, empty_args, m_emitter.null_pyobject() });

    // x = await anext_aw
    auto *awaitable = m_emitter.call_get_awaitable(anext_aw);

    // yield from awaitable — 但需要捕获 StopAsyncIteration
    // 这里复用 emit_yield_from，但外层包一个 try/catch StopAsyncIteration
    // ... (使用 invoke + landingpad 捕获 StopAsyncIteration)

    // 正常路径: store to target, 执行 body
    store_to_target(node->target().get(), /* yield_from result */);
    generate_body(node->body());
    m_builder.CreateBr(loop_bb);

    // StopAsyncIteration 路径: 跳到 else
    m_builder.SetInsertPoint(stop_bb);
    // ... 检查是否 StopAsyncIteration ...
    // 是 → else_bb, 否 → reraise

    // else
    m_builder.SetInsertPoint(else_bb);
    if (!node->orelse().empty()) {
        generate_body(node->orelse());
    }
    m_builder.CreateBr(merge_bb);

    m_builder.SetInsertPoint(merge_bb);
    return nullptr;
}
```

### 13.10 visit(AsyncWith*) 实现

```cpp
ast::Value *PylangCodegen::visit(const ast::AsyncWith *node)
{
    // 与 visit(With*) 结构相同
    // 区别: __aenter__/__aexit__ 的返回值需要 await

    // 进入阶段:
    //   exit_fn = getattr(mgr, "__aexit__")
    //   enter_fn = getattr(mgr, "__aenter__")
    //   enter_result = call(enter_fn)
    //   value = await enter_result    ← 这里多了 await
    //   var = value

    // 退出阶段 (正常):
    //   exit_result = call(exit_fn, None, None, None)
    //   await exit_result             ← 这里多了 await

    // 退出阶段 (异常):
    //   exit_result = call(exit_fn, exc_type, exc_val, exc_tb)
    //   suppress = await exit_result  ← 这里多了 await
    //   if suppress: 吞掉异常
    //   else: reraise

    // 复用 visit(With*) 的整体结构
    // 将 emit_call 替换为 emit_call + emit_yield_from(get_awaitable(result))
}
```

## 14. emit_coro_prologue / emit_coro_epilogue 详细实现

### 14.1 emit_coro_prologue

```cpp
void PylangCodegen::emit_coro_prologue(CoroContext::Kind kind, std::string_view func_name)
{
    auto *func = m_codegen_ctx.current_function();
    auto *entry_bb = &func->getEntryBlock();

    // 1. coro.id
    auto *id = m_emitter.emit_coro_id();

    // 2. coro.alloc 分支
    auto *need_alloc = m_emitter.emit_coro_alloc(id);
    auto *alloc_bb = llvm::BasicBlock::Create(m_ctx, "coro.alloc", func);
    auto *begin_bb = llvm::BasicBlock::Create(m_ctx, "coro.begin", func);
    m_builder.CreateCondBr(need_alloc, alloc_bb, begin_bb);

    // 3. alloc: malloc(coro.size)
    m_builder.SetInsertPoint(alloc_bb);
    auto *size = m_emitter.emit_coro_size();
    auto *mem = m_builder.CreateCall(m_emitter.get_malloc(), { size }, "coro.mem");
    m_builder.CreateBr(begin_bb);

    // 4. begin: phi + coro.begin
    m_builder.SetInsertPoint(begin_bb);
    auto *phi = m_builder.CreatePHI(m_builder.getPtrTy(), 2, "coro.mem.phi");
    phi->addIncoming(llvm::ConstantPointerNull::get(m_builder.getPtrTy()), entry_bb);
    phi->addIncoming(mem, alloc_bb);
    auto *hdl = m_emitter.emit_coro_begin(id, phi);

    // 5. NativeCoroState alloca + 零初始化
    auto *state = m_emitter.emit_coro_state_alloca();

    // 6. 创建包装对象
    llvm::Value *gen_obj = nullptr;
    switch (kind) {
    case CoroContext::Kind::GENERATOR:
        gen_obj = m_emitter.call_create_native_generator(hdl, state, func_name);
        break;
    case CoroContext::Kind::COROUTINE:
        gen_obj = m_emitter.call_create_native_coroutine(hdl, state, func_name);
        break;
    case CoroContext::Kind::ASYNC_GENERATOR:
        gen_obj = m_emitter.call_create_native_async_generator(hdl, state, func_name);
        break;
    }

    // 7. 创建 cleanup/suspend/unwind BB (稍后在 epilogue 中填充)
    auto *cleanup_bb = llvm::BasicBlock::Create(m_ctx, "coro.cleanup", func);
    auto *suspend_bb = llvm::BasicBlock::Create(m_ctx, "coro.suspend", func);
    auto *unwind_bb = llvm::BasicBlock::Create(m_ctx, "coro.unwind", func);

    // 8. 设置 CoroContext
    CoroContext coro_ctx{};
    coro_ctx.coro_handle = hdl;
    coro_ctx.coro_id = id;
    coro_ctx.state_alloca = state;
    coro_ctx.gen_obj = gen_obj;
    coro_ctx.cleanup_bb = cleanup_bb;
    coro_ctx.suspend_bb = suspend_bb;
    coro_ctx.unwind_bb = unwind_bb;
    coro_ctx.kind = kind;
    m_codegen_ctx.set_coro_context(coro_ctx);

    // 9. 初始挂起
    auto *tok = m_emitter.emit_coro_save(hdl);
    auto *sp = m_emitter.emit_coro_suspend(tok, /*is_final=*/false);
    auto *resume_bb = llvm::BasicBlock::Create(m_ctx, "coro.initial_resume", func);
    auto *sw = m_builder.CreateSwitch(sp, suspend_bb, 2);
    sw->addCase(m_builder.getInt8(0), resume_bb);
    sw->addCase(m_builder.getInt8(1), cleanup_bb);

    // 10. 恢复后: 函数体从这里开始
    m_builder.SetInsertPoint(resume_bb);
}
```

### 14.2 emit_coro_epilogue

```cpp
void PylangCodegen::emit_coro_epilogue()
{
    auto &coro = m_codegen_ctx.coro_ctx();
    auto *func = m_codegen_ctx.current_function();

    // 如果当前 BB 没有 terminator，跳到 cleanup
    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(coro.cleanup_bb);
    }

    // --- coro.unwind: 未捕获异常 ---
    m_builder.SetInsertPoint(coro.unwind_bb);
    auto *lp_ty = llvm::StructType::get(m_ctx, { m_builder.getPtrTy(), m_builder.getInt32Ty() });
    auto *lp = m_builder.CreateLandingPad(lp_ty, 0, "coro.lp");
    lp->setCleanup(true);
    m_emitter.call_generator_mark_done(coro.state_alloca);
    m_builder.CreateResume(lp);

    // --- coro.cleanup: 正常结束或 return ---
    m_builder.SetInsertPoint(coro.cleanup_bb);
    m_emitter.call_generator_mark_done(coro.state_alloca);
    auto *mem_ptr = m_emitter.emit_coro_free(coro.coro_id, coro.coro_handle);
    // 条件释放: coro_free 可能返回 null (如果 frame 在栈上)
    auto *need_free = m_builder.CreateICmpNE(
        mem_ptr, llvm::ConstantPointerNull::get(m_builder.getPtrTy()));
    auto *do_free_bb = llvm::BasicBlock::Create(m_ctx, "coro.do_free", func);
    m_builder.CreateCondBr(need_free, do_free_bb, coro.suspend_bb);

    m_builder.SetInsertPoint(do_free_bb);
    m_builder.CreateCall(m_emitter.get_free(), { mem_ptr });
    m_builder.CreateBr(coro.suspend_bb);

    // --- coro.suspend: 最终挂起点 ---
    m_builder.SetInsertPoint(coro.suspend_bb);
    m_emitter.emit_coro_end(coro.coro_handle);
    m_builder.CreateRet(coro.gen_obj);

    // 清理 coro context
    m_codegen_ctx.clear_coro_context();
}
```

### 14.3 emit_yield_suspend

```cpp
llvm::BasicBlock *PylangCodegen::emit_yield_suspend(llvm::Value *yield_value)
{
    auto &coro = m_codegen_ctx.coro_ctx();
    auto *func = m_codegen_ctx.current_function();

    // 设置 yielded_value
    m_emitter.call_generator_set_yield(coro.state_alloca, yield_value);

    // 设置 is_yield_point = true
    m_emitter.emit_coro_state_set_yield_point(coro.state_alloca, true);

    // coro.save + coro.suspend
    auto *tok = m_emitter.emit_coro_save(coro.coro_handle);
    auto *sp = m_emitter.emit_coro_suspend(tok, /*is_final=*/false);

    auto *resume_bb = llvm::BasicBlock::Create(m_ctx, "yield.resume", func);
    auto *sw = m_builder.CreateSwitch(sp, coro.suspend_bb, 2);
    sw->addCase(m_builder.getInt8(0), resume_bb);
    sw->addCase(m_builder.getInt8(1), coro.cleanup_bb);

    m_builder.SetInsertPoint(resume_bb);
    return resume_bb;
}
```

## 15. LLVM Pass Pipeline 集成

### 15.1 当前状态

目前 `PylangCodegen::compile()` 直接生成 LLVM IR 并通过 `llvm::verifyModule` 验证，
没有独立的 PassManager / Driver 层。coroutine passes 的集成需要在未来添加优化管道时一并处理。

### 15.2 CoroSplit 要求

LLVM CoroSplit pass 要求：
- 函数标记 `presplitcoroutine` 属性（编译器在 `emit_coro_prologue` 中添加）
- 函数使用 `llvm.coro.*` intrinsics（编译器生成）
- CoroSplit 在 CGSCC pass pipeline 中运行，将函数拆分为 ramp/resume/destroy/cleanup

### 15.3 TODO: 未来 PassManager 集成

当项目添加优化管道时，需要确保 coroutine passes 在正确位置运行：

```
TODO:
  1. 在 ModulePassManager 中添加 CoroEarlyPass（标准化 coro intrinsics）
  2. 在 CGSCC pipeline 中添加 CoroSplitPass（拆分 coroutine 函数）
  3. 在 Function pipeline 中添加 CoroElidePass（可选，栈上分配优化）
  4. 在 pipeline 末尾添加 CoroCleanupPass（清理残留 intrinsics）

  新 PassManager (LLVM 16+) 示例:
    PB.registerCGSCCOptimizerLateEPCallback(
        [](CGSCCPassManager &PM, OptimizationLevel) {
            PM.addPass(CoroSplitPass());
        });

  或者使用 buildDefaultAAPipeline() 自动包含 coro passes。
```

### 15.4 临时方案：JIT 模式

在没有 PassManager 的情况下，可以通过 LLVM ORC JIT 的 `IRTransformLayer` 注入 coro passes，
或者在 `compile()` 返回前手动运行一次 coroutine pipeline。这是 Phase 1 的临时方案：

```cpp
// PylangCodegen::compile() 末尾，verifyModule 之前:
// 手动运行 coro passes（临时方案）
if (module_has_coroutines) {
    llvm::ModulePassManager MPM;
    llvm::ModuleAnalysisManager MAM;
    // ... 注册 coro passes ...
    MPM.run(*module, MAM);
}
```

## 16. PyNativeGenerator / PyNativeCoroutine / PyNativeAsyncGenerator 类型注册

### 16.1 与 BuiltinTypes 的关系

`BuiltinTypes` 已有 `m_generator` / `m_coroutine` / `m_async_generator` 槽位，
它们指向字节码解释器的类型。AOT 的 native 类型需要独立注册：

```
方案 A: 复用现有槽位
  - PyNativeGenerator 注册为 "generator" 类型的子类型
  - 优点: isinstance(gen, types.GeneratorType) 自动工作
  - 缺点: 需要修改 type_factory 支持多态

方案 B: 独立类型 (推荐)
  - PyNativeGenerator 是独立类型，但 __class__ 返回 generator type
  - BuiltinTypes 新增 m_native_generator / m_native_coroutine / m_native_async_generator
  - isinstance 通过 __instancecheck__ 或 abc 机制兼容
```

推荐方案 B，在 `BuiltinTypes` 中新增三个槽位，`builtinTypeInit.cpp` 中注册。

### 16.2 需要新增的文件

```
src/runtime/PyNativeGenerator.hpp      — 类声明
src/runtime/PyNativeGenerator.cpp      — send/throw/close/__iter__/__next__ 实现
src/runtime/PyNativeCoroutine.hpp      — 类声明
src/runtime/PyNativeCoroutine.cpp      — send/throw/close/__await__ 实现
src/runtime/PyNativeAsyncGenerator.hpp — 类声明
src/runtime/PyNativeAsyncGenerator.cpp — asend/athrow/aclose/__aiter__/__anext__ 实现
src/runtime/NativeCoroState.hpp        — CoroStatus/ResumeAction/NativeCoroState 定义
src/runtime/PyAsyncGenASend.hpp        — async generator asend 返回的 awaitable
src/runtime/PyAsyncGenASend.cpp
src/runtime/PyAsyncGenAThrow.hpp       — async generator athrow/aclose 返回的 awaitable
src/runtime/PyAsyncGenAThrow.cpp
```

### 16.3 resume 函数指针获取

LLVM CoroSplit 后，coro handle 的第一个字段是 resume 函数指针。
`PyNativeGenerator::send()` 通过以下方式调用 resume：

```cpp
// coro handle 布局 (CoroSplit 保证):
// [0] = ptr to resume function
// [1] = ptr to destroy function
using ResumeFn = void(*)(void*);
auto resume = *reinterpret_cast<ResumeFn*>(m_coro_handle);
resume(m_coro_handle);
```

## 17. 详细实现步骤

### Phase 1: Generator（基础）

按以下顺序实现，每步完成后可独立测试：

```
Step 1.1: NativeCoroState.hpp (runtime 头文件)
  - CoroStatus enum class
  - ResumeAction enum class
  - NativeCoroState struct
  - 纯头文件，无 .cpp

Step 1.2: PyNativeGenerator.hpp / .cpp (runtime 核心类型)
  - 类声明 + type_factory
  - create() 静态工厂
  - send() — 核心: resume coro handle + PEP 479
  - throw_() — 三参数形式 + 异常实例化
  - close() — GeneratorExit 语义
  - __iter__() → return self
  - __next__() → send(None)
  - visit_graph() — GC 遍历
  依赖: Step 1.1

Step 1.3: BuiltinTypes 注册
  - builtin.hpp: 新增 m_native_generator 槽位
  - builtin.cpp: 构造函数中调用 PyNativeGenerator::type_factory()
  - builtinTypeInit.cpp: 注册到类型系统
  依赖: Step 1.2

Step 1.4: rt_coro.cpp (runtime 导出层)
  - rt_create_native_generator
  - rt_generator_set_yield
  - rt_generator_get_sent (关键: 检查 thrown_exception → throw C++ 异常)
  - rt_generator_set_return
  - rt_generator_mark_done
  依赖: Step 1.2, Step 1.3

Step 1.5: export.hpp 新增宏
  - PYLANG_EXPORT_CORO 宏定义
  依赖: 无

Step 1.6: CodegenContext 扩展
  - CoroContext struct 定义
  - set_coro_context / clear_coro_context / in_coroutine / coro_ctx 方法
  依赖: 无

Step 1.7: IREmitter Tier 7 — coro intrinsics
  - declare_coro_intrinsics()
  - emit_coro_id / alloc / size / begin / save / suspend / end / free
  - get_malloc / get_free
  - get_coro_state_type / emit_coro_state_alloca
  - emit_coro_state_set_yield / set_return / set_status / set_yield_point
  依赖: Step 1.5 (export 宏让 RuntimeLinker 能发现 rt_coro.cpp 的函数)

Step 1.8: IREmitter Tier 7 — runtime call 封装
  - call_create_native_generator
  - call_generator_set_yield / get_sent / set_return / mark_done
  依赖: Step 1.7

Step 1.9: PylangCodegen — coro 框架
  - emit_coro_prologue()
  - emit_coro_epilogue()
  - emit_yield_suspend()
  依赖: Step 1.6, Step 1.8

Step 1.10: PylangCodegen — visit 实现
  - visit(Yield*) — 替换现有 stub
  - visit(FunctionDefinition*) — 修改: 检测 yield → generator 路径
  - visit(Return*) — 修改: generator 内 return → set_return + br cleanup
  依赖: Step 1.9

Step 1.11: 测试
  - IREmitter_test: coro intrinsics 生成测试
  - PylangCodegen_test: 简单 generator 编译测试
  - 集成测试: def gen(): yield 1; yield 2 → 运行验证
```

### Phase 2: yield from

```
Step 2.1: rt_coro.cpp 扩展
  - rt_yield_from_next / send / throw / close
  - rt_is_stopiteration / rt_stopiteration_value
  - rt_create_generator_exit
  - rt_compare_is

Step 2.2: IREmitter 扩展
  - call_yield_from_next / send / throw / close
  - call_is_stopiteration / call_stopiteration_value
  - call_create_generator_exit / call_compare_is

Step 2.3: PylangCodegen
  - emit_yield_from() — 完整 PEP 380 IR 展开 (§6.2.1)
  - visit(YieldFrom*) — 替换现有 stub

Step 2.4: 测试
  - yield from range(5)
  - yield from 双向 send/throw 透传
  - yield from 的 StopIteration.value 返回
```

### Phase 3: Coroutine

```
Step 3.1: PyNativeCoroutine.hpp / .cpp
  - 与 PyNativeGenerator 结构相同
  - __await__() 替代 __iter__()
  - 类型名 "coroutine"，属性前缀 cr_

Step 3.2: BuiltinTypes 注册 m_native_coroutine

Step 3.3: rt_coro.cpp 扩展
  - rt_create_native_coroutine
  - rt_get_awaitable

Step 3.4: IREmitter 扩展
  - call_create_native_coroutine
  - call_get_awaitable

Step 3.5: PylangCodegen
  - visit(AsyncFunctionDefinition*) — 替换现有 stub
  - visit(Await*) — 替换现有 stub (= yield from get_awaitable)

Step 3.6: 测试
  - async def f(): return 42 → coroutine 对象
  - async def f(): return await g()
```

### Phase 4: Async Generator + async for + async with

```
Step 4.1: PyNativeAsyncGenerator.hpp / .cpp
Step 4.2: PyAsyncGenASend.hpp / .cpp — asend 返回的 awaitable
Step 4.3: PyAsyncGenAThrow.hpp / .cpp — athrow/aclose 返回的 awaitable
Step 4.4: BuiltinTypes 注册 m_native_async_generator
Step 4.5: rt_coro.cpp 扩展 — async generator 相关导出函数
Step 4.6: IREmitter 扩展 — call_get_aiter 等
Step 4.7: PylangCodegen — visit(AsyncFor*) / visit(AsyncWith*)
Step 4.8: 测试
```

### Phase 5: 边缘语义 (延后)

```
- coroutine 未 await 时的 RuntimeWarning
- @types.coroutine 装饰器兼容
- sys.set_asyncgen_hooks
- generator expression 的 coro 编译
```

### 文件修改总览

```
新增文件 (12):
  src/runtime/NativeCoroState.hpp
  src/runtime/PyNativeGenerator.hpp
  src/runtime/PyNativeGenerator.cpp
  src/runtime/PyNativeCoroutine.hpp
  src/runtime/PyNativeCoroutine.cpp
  src/runtime/PyNativeAsyncGenerator.hpp
  src/runtime/PyNativeAsyncGenerator.cpp
  src/runtime/PyAsyncGenASend.hpp
  src/runtime/PyAsyncGenASend.cpp
  src/runtime/PyAsyncGenAThrow.hpp
  src/runtime/PyAsyncGenAThrow.cpp
  src/runtime/export/rt_coro.cpp

修改文件 (8):
  src/runtime/export/export.hpp          — PYLANG_EXPORT_CORO 宏
  src/runtime/types/builtin.hpp          — 新增 native generator/coroutine/async_generator 槽位
  src/runtime/types/builtin.cpp          — 构造函数中初始化新槽位
  src/runtime/builtinTypeInit.cpp        — 注册新类型
  src/compiler/Codegen/CodegenContext.hpp — CoroContext struct + 相关方法
  src/compiler/Codegen/IREmitter.hpp     — Tier 7 方法声明 + 私有成员
  src/compiler/Codegen/IREmitter.cpp     — Tier 7 方法实现
  src/compiler/Codegen/PylangCodegen.hpp — emit_coro_prologue/epilogue/yield_suspend/yield_from 声明
  src/compiler/Codegen/PylangCodegen.cpp — visit(Yield/YieldFrom/Await/AsyncFunctionDef/AsyncFor/AsyncWith) 实现
                                           + visit(FunctionDefinition) 修改
                                           + visit(Return) 修改 (generator 内)

测试文件 (修改):
  src/compiler/Codegen/IREmitter_test.cpp     — coro intrinsics 测试
  src/compiler/Codegen/PylangCodegen_test.cpp — generator/coroutine 编译测试
```