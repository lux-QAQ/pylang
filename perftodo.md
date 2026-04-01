# AOT 性能优化与值层重构 TODO

本文档是当前项目的性能优化清单，也是后续 AOT runtime 重构的设计文档。

文档目标：

- 严格符合 Python 3.9 语义。
- 全面拥抱 AOT。
- 让 AOT 热路径中的值尽量不退化。
- 在重构过程中尽量避免全局替换、全局签名改写、全局使用点爆炸。
- 为未来类型扩展预留良好结构，例如复数、更多 immediates、更多专门容器。

这里不写“不要做什么”，只写：

- 需要达成的目标
- 推荐采用的设计
- 具体需要改什么文件
- 每一阶段的完成标准

---

## 1. 现状与核心问题

当前已经有 `RtValue` / tagged int，但整体性能仍然差，根因不是算术本身，而是值在 AOT 热路径中不断退化：

1. AOT 函数 ABI 仍然以 `Value* args_array` 为主。
2. 参数提取仍会走 `Value -> PyObject::from(...)`。
3. 迭代器与容器在元素出口经常重新 materialize heap `PyObject`。
4. 调用路径虽然有 `raw_ptrs`，但很多地方仍然要桥接回通用 runtime。
5. `flatten` 不是根因，它是“值流在边界不断退化”的症状。

这意味着当前 `RtValue` 只是一个局部优化点，还没有成为：

- AOT 调用帧的统一值
- AOT 容器的统一元素表示
- AOT 迭代器的统一产出值

因此本次重构的主线不是继续局部打磨 `flatten`，而是让 AOT 热路径形成一条稳定的轻量值通道。

---

## 2. 必须守住的 Python 3.9 语义

所有优化都必须保持 Python 3.9 的可观察行为一致。

### 2.1 值语义

必须保持：

- `is` 与 `==` 的区别
- `__hash__` / `__eq__` / `richcompare`
- `bool` 与 `int` 的 Python 关系
- `None` / `Ellipsis` / 其他单例行为

### 2.2 对象语义

必须保持：

- descriptor 协议
- `__getattribute__` / `__getattr__`
- `__setattr__` / `__delattr__`
- bound method 绑定规则
- 类型查找与 MRO

### 2.3 调用语义

必须保持：

- 位置参数 / 关键字参数绑定
- 默认参数在定义时求值
- 闭包、cell/free variable 行为
- `super()` / `__classcell__`
- 类体执行与 metaclass 路径

### 2.4 迭代与异常语义

必须保持：

- `__iter__` / `__next__`
- `StopIteration`
- `try` / `except` / `finally`
- 异常匹配与重抛

### 2.5 容器语义

必须保持：

- list / tuple / dict / set 的可观察行为
- dict 键比较与哈希一致性
- 迭代顺序与元素语义

结论：

所有快路径都必须是“与当前 Python 语义等价时才命中”，不能通过改变语言行为换性能。

---

## 3. 总体设计：引入 AOT 值层，但采用兼容式过渡

目标是引入 AOT 值层，并让它逐步成为 AOT 热路径中的统一值表示。

这里的关键不是立刻发明一套完全独立的新 runtime，而是分层：

1. 通用语义层
   - `PyObject`
   - `Value`
   - 完整 Python 语义逻辑

2. AOT 值层
   - AOT 调用帧
   - AOT 容器元素
   - AOT 迭代器返回值
   - AOT 热路径临时值

3. 边界桥接层
   - `box`
   - `to_value`
   - `from_value`
   - 在必须进入通用语义路径时才桥接

这样可以做到：

- 前端继续保持“生成 runtime 调用”为主，不强迫它立刻静态类型化。
- runtime 拥有明确的 AOT 高速路径。
- 旧接口可以逐步兼容，而不是一次性全部替换。

---

## 4. AOT 值层的具体设计

### 4.1 设计目标

AOT 值层需要满足：

- 可以表示 heap object
- 可以表示小整数 immediate
- 后续可以扩展更多值类别
- 能作为 ABI 稳定传输单元
- 与 GC / 引用生命周期兼容
- 不重复实现完整 Python 对象系统

### 4.2 推荐实现：先把 `RtValue` 提升为 AOT 值内核

第一阶段不直接把全项目切成一个全新的重量级 `class AotValue`。

第一阶段采用：

- 概念层面统一使用 `AotValue`
- 实现层面由 `RtValue` 承载
- 在少量新接口上显式使用 `AotValue`

建议初期形式：

```cpp
namespace py {
using AotValue = RtValue;
}
```

或者：

```cpp
namespace py {
struct AotValue {
    RtValue raw;
};
}
```

推荐先从 `using AotValue = RtValue;` 开始。

原因：

- 改动半径最小
- 旧代码无需立刻全部知道新类型
- 新接口可以开始围绕 `AotValue` 命名
- 编译错误和测试失败范围可控

### 4.3 第二阶段再把 `AotValue` 从别名升级为薄封装

当 AOT ABI、容器、迭代器稳定后，再考虑从别名升级为薄封装：

- 初期只包含 `RtValue bits`
- 保持内联和零开销
- 新增更中性的接口名

这样未来可以逐步扩展，而不需要再改所有调用点。

### 4.4 `AotValue` 的推荐接口

需要提供以下能力：

- `is_heap_object()`
- `is_tagged_int()`
- `is_null()`
- `box()`
- `to_value()`
- `from_value(...)`
- `from_ptr(...)`
- `from_int(...)`
- `from_int_or_box(...)`
- `as_pyobject_raw()`
- `kind()` 或后续等价扩展点

其中：

- `box()` 表示显式可能触发 materialize / 分配
- `to_value()` 表示显式回到通用 runtime 容器
- 所有桥接必须是显式命名

### 4.5 扩展性要求

后续支持复数、更多 immediates、特殊常量时，AOT 值层必须可扩展。

具体要求：

- 不能把设计写死为“只有 tagged int + heap ptr”
- 新增值类别时，容器 / 调用帧 / 迭代器接口不需要全部重写
- 新增值类别时，只需要补：
  - 表示层
  - 桥接层
  - 必要的算术 / 比较 / hash 快路径

对复数的第一阶段方案：

- 先仍然走 heap object
- 但必须能自然流经 `AotValue`
- 不要求复数一开始就是 immediate

---

## 5. 优雅过渡的核心策略

重构过程中要尽量避免全局修改所有使用点，因此采用“边界优先、局部替换、双接口共存”的方式。

### 5.1 先改边界，不先改所有内部实现

优先改这些热点边界：

1. AOT 函数 ABI
2. AOT 参数提取
3. AOT 迭代器返回值
4. AOT 容器元素表示
5. AOT raw call 路径

这样最先消灭的是值退化最严重的地方。

### 5.2 先加新接口，再迁移调用点

采用模式：

- 保留旧接口
- 新增 AOT 专用接口
- 热路径调用点逐步迁移
- 旧接口在一段时间内作为 fallback 和兼容桥

### 5.3 不做全局强替换

不要求一开始把：

- `Value`
- `PyObject*`
- `RtValue`

所有使用点全部统一改名或统一替换。

第一阶段只要求：

- AOT 热路径开始显式使用 `AotValue`
- 通用 runtime 继续可以使用 `Value`
- 桥接边界清晰可控

### 5.4 让类型名迁移和数据流迁移分开

迁移顺序应该是：

1. 先迁移数据流
   - 参数数组
   - 返回值
   - 容器元素
   - 迭代器产物

2. 再迁移命名与接口
   - `AotValue` 别名
   - `aot_*` API
   - 少量薄封装

这样可以减少一次性重构量。

---

## 6. 第一阶段的具体文件级修改清单

这一节是最核心的落地任务。

### 6.1 `src/runtime/taggered_pointer/RtValue.hpp`

TODO:

- 在此文件中引入 `AotValue` 第一阶段定义。
- 初期使用 `using AotValue = RtValue;`。
- 补充面向 AOT 值层的中性接口命名说明。
- 明确哪些 API 属于“桥接 API”：
  - `box`
  - `to_value`
  - `from_value`
- 为未来扩展预留接口注释，例如 `kind()` 的设计位置。

完成标准：

- 新代码可以开始引用 `AotValue`，旧代码不需要大面积修改。

### 6.2 `src/runtime/taggered_pointer/RtValue.cpp`

TODO:

- 保留现有 tagged int 快路径。
- 明确 `flatten` 的角色是桥接修复，而不是主路径常规操作。
- 补整理 `from_value` / `to_value` / `box` 的职责注释。
- 为后续扩展值种类预留结构，不把逻辑写死为只有 int。

性能目标：

- 后续通过减少调用次数让 `flatten` 变冷，而不是继续让它承担高频主逻辑。

### 6.3 `src/compiler/Codegen/PylangCodegen.cpp`

TODO:

- 为 AOT 函数引入新的参数数组 ABI。
- 现有 `Value* args_array` 路径继续保留一段时间。
- 新增 `AotValue[]` 路径后，函数、lambda、推导式、类体统一使用新 ABI。
- 参数提取逻辑从 `call_value_array_get` 逐步迁移到 `call_aot_array_get`。

重点修改点：

- 普通函数定义
- lambda
- comprehension
- class body
- raw function call emission

完成标准：

- 新生成的 AOT IR 在热路径中不再依赖 `Value* args_array` 作为唯一参数形式。

### 6.4 `src/compiler/Codegen/IREmitter.hpp`

TODO:

- 新增 AOT 专用 emitter 接口：
  - `call_aot_array_get`
  - `call_function_aot_raw`
  - `call_method_aot_raw`
  - 未来可加 `call_iter_next_aot`
- 保留旧的 `call_value_array_get` 等接口。
- 明确哪些 emitter 接口服务于通用 runtime，哪些服务于 AOT fast path。

### 6.5 `src/compiler/Codegen/IREmitter.cpp`

TODO:

- 落实现新的 AOT emitter 接口。
- 让新接口链接到新的 runtime export 函数。
- 保证新旧接口可以并存。

完成标准：

- 前端可以选择生成新 ABI，不要求一次性重写所有 codegen 路径。

### 6.6 `src/runtime/export/*`

TODO:

- 为 AOT 参数数组与 AOT raw call 增加新的 export 函数。
- 保留旧导出函数。
- 命名建议：
  - `rt_aot_array_get`
  - `rt_call_raw_aot`
  - `rt_call_method_raw_aot`

要求：

- 这些导出函数在语义上与原有调用保持一致。
- 差异只体现在值表示和桥接成本。

---

## 7. 第二阶段：AOT 调用帧统一为 `AotValue`

这是性能收益最高的一步。

### 7.1 目标

将 AOT 调用帧中的以下对象统一为 `AotValue`：

- 参数数组
- 参数提取结果
- 局部临时值
- 返回值

### 7.2 具体任务

TODO:

1. 新增 AOT raw ABI 签名。
2. 在 runtime 中允许 native AOT function 直接接受 `AotValue*`。
3. 在 codegen 中优先生成新 ABI。
4. 在 runtime raw call 路径中优先识别并直达新 ABI。
5. 保留旧 ABI 的 fallback。

### 7.3 需要修改的模块

- `PylangCodegen.cpp`
- `IREmitter.hpp`
- `IREmitter.cpp`
- `runtime/export/rt_func*`
- `PyFunction` / `PyNativeFunction` 相关实现

### 7.4 验收标准

- 参数提取时不再主走 `PyObject::from(Value)`
- `flatten` 与 `from_value` 热点显著下降

---

## 8. 第三阶段：AOT 迭代器统一为 `AotValue`

### 8.1 目标

让 AOT 迭代器返回轻量值，而不是每次都重新分配 heap 对象。

### 8.2 具体任务

TODO:

1. 先改 list iterator。
2. 再改 tuple iterator。
3. 再改 dict/set/items/values iterator。
4. 增加 `rt_iter_next_aot` 或等价 AOT 接口。
5. 让 AOT `for` 循环优先调用 AOT 迭代器接口。

### 8.3 需要修改的模块

- `PyList` / `PyListIterator`
- `PyTuple` / `PyTupleIterator`
- dict/set 迭代器实现
- `runtime/export` 迭代相关导出函数
- `IREmitter` 迭代调用接口
- `PylangCodegen.cpp` 中 `For` / comprehension 相关生成逻辑

### 8.4 语义要求

- `StopIteration` 必须保持标准行为
- 元素值与当前 Python 语义一致
- 异常路径与普通迭代路径一致

### 8.5 验收标准

- list/tuple 热迭代中小整数不再频繁 materialize 为 `PyInteger`

---

## 9. 第四阶段：AOT 容器元素表示改造

### 9.1 目标

让热容器内部能够存放 `AotValue`，从而减少来回桥接。

### 9.2 改造顺序

TODO:

1. list
2. tuple
3. dict value path
4. dict key path
5. set

### 9.3 具体设计

采用“容器内部增加 AOT 存储能力”的方式，而不是一开始重写所有容器类。

建议路径：

- 初期容器内部允许保存 `AotValue`
- 在需要通用 Python 语义时才桥接到 `Value` / `PyObject`
- 容器对外语义不变

### 9.4 需要修改的模块

- list / tuple / dict / set 的内部元素存储
- 对应 iterator
- 对应 getitem / setitem / append / extend / insert / unpack 等路径

### 9.5 验收标准

- 热容器的读写与遍历中，`to_value` / `from_value` 次数明显下降

---

## 10. 第五阶段：AOT raw call 路径直达 native function

### 10.1 目标

让 AOT 函数之间调用尽可能不再经过 `Value` 桥接。

### 10.2 具体任务

TODO:

1. 在 runtime raw call 路径中识别 AOT native function。
2. 命中时直接走 AOT ABI。
3. 方法调用也增加对应快路径。
4. descriptor / bound method 语义保持一致。
5. 慢路径继续存在。

### 10.3 需要修改的模块

- `PyFunction` / `PyNativeFunction`
- `runtime/export/rt_func*`
- method lookup / raw call 相关实现

### 10.4 验收标准

- `rt_call_method_raw_ptrs` 的桥接开销显著下降

---

## 11. 第六阶段：AOT class layout 与属性快路径

### 11.1 目标

为 AOT 生成的类实例提供固定字段布局，但保留 Python 语义 fallback。

### 11.2 具体任务

TODO:

1. 给 AOT class 增加静态字段布局描述。
2. 字段命中时走 slot offset load/store。
3. 未命中时回退到现有 attribute machinery。
4. descriptor、`__dict__`、`__setattr__`、元类语义保持不变。

### 11.3 需要修改的模块

- `PyType`
- `PyObject`
- class build / class layout 相关 runtime 逻辑
- AOT class metadata 构建逻辑

### 11.4 验收标准

- 常见 AOT 实例字段访问的热点明显下降

---

## 12. 第七阶段：hash / eq / compare 快路径

### 12.1 目标

对 AOT 热路径中的常见值类型给出更轻量的比较和哈希。

### 12.2 具体任务

TODO:

1. 为小整数 AOT 值补轻量 hash / eq。
2. 为常见 heap object 类型补缓存或快路径。
3. 保留 `richcompare` 与通用对象比较逻辑。
4. dict / set 高热场景优先使用 AOT 快路径。

### 12.3 需要修改的模块

- `ValueHash`
- `ValueEq`
- `PyObject::hash`
- `PyObject::richcompare`
- dict / set 内部查找路径

### 12.4 语义要求

- `__hash__`
- `__eq__`
- `richcompare`

必须与 Python 3.9 一致。

---

## 13. 第八阶段：把 `AotValue` 从别名升级为稳定抽象

当前第一阶段采用 `using AotValue = RtValue;`，但长期仍然需要更稳定的抽象层。

### 13.1 触发条件

当下面这些完成后再升级：

- AOT ABI 已稳定
- AOT 迭代器已稳定
- AOT 容器已有统一值流
- `RtValue` 的接口边界已清晰

### 13.2 升级方式

TODO:

1. 将 `AotValue` 从 type alias 升级为薄封装。
2. 薄封装内部继续持有 `RtValue` 或等价位表示。
3. 保持零开销内联。
4. 避免改动大多数调用点签名。

### 13.3 目标

- 对外统一使用 `AotValue`
- 对内继续复用现有高性能位表示
- 为未来扩展更多类型留出空间

---

## 14. 测试与验证清单

### 14.1 语义测试

TODO:

- 补 `is` / `==` / `hash`
- 补 `bool` / `int` 交互
- 补 `StopIteration`
- 补 descriptor / bound method
- 补 kwargs / 默认参数 / 闭包
- 补异常传播 / finally
- 补 AOT 类实例字段访问
- 补 `super()` / `__classcell__`
- 补容器与迭代器一致性

### 14.2 性能测试

TODO:

- 统一使用 release 构建
- 关闭分配 trace 和多余日志
- 固定 benchmark
- 比较 CPython、旧 runtime、新 runtime

重点指标：

- 总耗时
- `flatten` 占比
- `PyObject::from(Value)` 占比
- `rt_call_method_raw_ptrs` 占比
- 小整数 materialization 次数
- 容器桥接次数

---

## 15. 推荐实施顺序

### P0

1. 在 `RtValue.hpp` 引入 `AotValue` 第一阶段定义
2. 新增 AOT ABI 与 emitter 接口
3. `PylangCodegen` 开始支持生成 `AotValue[]` 参数数组
4. runtime raw call 直达 AOT ABI

### P1

1. list / tuple AOT iterator
2. list / tuple AOT 容器元素表示
3. 参数提取与返回值链路去 `Value` 化

### P2

1. dict/set AOT 元素路径
2. hash / eq 快路径
3. AOT class layout

### P3

1. `AotValue` 从 alias 升级为薄封装
2. 扩展更多值类别
3. 更深层的前端专门化优化

---

## 16. 一句话落地原则

本次重构的核心执行原则是：

> 不从“全局替换所有使用点”开始，而是从 AOT 热路径的值边界开始，让 AOT 调用帧、AOT 容器、AOT 迭代器逐步统一到 `AotValue`，同时让旧接口继续可用，直到新值流稳定。

只要这条路径打通，`RtValue` 才会从“局部小整数优化”升级成真正的 AOT 值内核，后续扩展到复数和更多类型时也会有稳定落点。
