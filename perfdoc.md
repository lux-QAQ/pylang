已创建 3 个待办事项

结合 top-down-tree.md，现在可以把问题定得更准：

# 一、核心瓶颈已经非常明确

top-down-tree.md 里最重要的信息不是 `RtValue::flatten`，而是这条主链：

- `PyInit_test`
- `rt_call_raw_ptrs`
- `py::PyNativeFunction::call_raw`
- `test.<module>.0:0.find.86:0`
- **`rt_call_method_raw_ptrs` 占 63.1%**

这说明：

## 当前最核心的问题不是“单个算术慢”
而是：

## **AOT 代码把 Python 源里的普通方法调用，编译成了高频 runtime 通用方法调用链**

也就是：

- `self.calc()`
- `self.loop_x()`
- `self.loop_y(x)`
- `self.step1(x, y)`
- `self.step2(x, y)`
- `self.step3(x, y)`

这些调用在 test.py 里是热点中的热点，结果现在全都经过：

- `PylangCodegen::visit(const ast::Call *)`
- `IREmitter::call_method_raw_ptrs`
- `rt_call_method_raw_ptrs`
- `PyType::lookup`
- descriptor 判断
- instance dict 覆盖判断
- `call_raw`
- AOT 函数包装
- `rt_value_array_get`

这条链语义是对的，但**对“稳定单态对象 + 高频方法调用”极不经济**。

---

# 二、基于当前架构的根因排序

结合你项目的架构，我认为性能根因按优先级是：

## 1. 方法调用通路过重
对应文件：

- PylangCodegen.cpp
- IREmitter.cpp
- rt_func.cpp
- PyFunction.cpp
- PyBoundMethod.cpp
- PyObject.cpp

这是第一瓶颈。  
`63.1%` 已经给出结论。

---

## 2. 属性访问与实例字典仍然太贵
对应文件：

- rt_attr.cpp
- PyObject.cpp
- PyDict.hpp
- PyDict.cpp

尤其是：

- `self.limit`
- `self.prime`
- `head.children`
- `head.terminal`

这些都是超高频。

---

## 3. `PyDict<Value, Value>` 的通用 key/value 语义过重
对应文件：

- PyDict.hpp
- PyDict.cpp
- PyObject.cpp 里的 `ValueHash::operator()` / `ValueEq::operator()`

这个是对象/属性/容器全局公共瓶颈。

---

## 4. `RtValue / Value / PyObject*` 表示切换成本高
对应文件：

- RtValue.cpp
- rt_op.cpp
- rt_cmp.cpp
- rt_attr.cpp
- rt_func.cpp

这不是第一根因，但会把上面 1~3 的成本继续放大。

---

# 三、正确方向：不是“静态化 Python”，而是“让动态语义走更薄的专用快路径”

为了保持 Python 3.9 语义，改进方案必须遵守三条原则：

## 原则 A：默认语义仍然是动态的
不能假设：

- 无 monkey patch
- 无实例覆盖
- 无子类覆写
- 无 descriptor
- 无 `__getattribute__`

---

## 原则 B：热点路径用 guard + cache 专用化
即：

- 命中快路径：零或极少分配、极少分支
- 失配：回退当前完整 runtime 逻辑

---

## 原则 C：优先优化对象模型基础设施，而不是只优化算术
也就是优先优化：

- call
- getattr
- dict
- method binding
- instance storage

而不是继续把主要精力放在 `RtValue::mul` 这类叶子节点。

---

# 四、详细改进方案

我按“收益最大、语义安全、符合你当前架构”来给出方案。

---

# 方案 1：把 `rt_call_method_raw_ptrs` 改造成“带版本守卫的单态方法调用缓存”
这是第一优先级。

---

## 1.1 当前问题

现在 `PylangCodegen::visit(const ast::Call *)` 对方法调用统一生成：

- `IREmitter::call_method_raw_ptrs(...)`

然后 `rt_call_method_raw_ptrs(...)` 每次都要：

1. `ensure_box(owner)`
2. `PyString::intern(method_name)`
3. `type->lookup(attr_name)`
4. descriptor 判断
5. instance dict 覆盖检查
6. 可能 `getattribute`
7. 构造 `Value` 数组
8. `call_raw`

这对 Python 语义没问题，但对单态热循环太重。

---

## 1.2 目标

引入一个**call-site inline cache**，但不破坏语义。

### 新增结构
建议在 rt_func.cpp 增加：

```cpp
struct MethodInlineCacheEntry {
    PyType *owner_type;
    uint64_t type_version;
    PyObject *interned_name;
    PyObject *resolved_callable;   // function/native_function/descriptor result
    bool needs_self;
    bool is_data_descr;
    bool valid;
};
```

如果你已有 `PyType::global_version()` / dict version 机制，可直接复用。

---

## 1.3 编译器侧改动

### IREmitter.hpp
新增一个 runtime 调用：

```cpp
llvm::Value *call_method_ic(
    llvm::Value *owner,
    llvm::Value *callsite_id,
    llvm::Value *method_name_cstr,
    llvm::Value *args_ptr,
    llvm::Value *argc,
    llvm::Value *kwargs);
```

### IREmitter.cpp
链接到新导出函数，例如 `call_method_ic_ptrs`。

### PylangCodegen.cpp
在 `visit(const ast::Call *)` 中，方法调用不再统一走：

- `call_method_raw_ptrs`

而改为：

- 为每个方法调用点分配一个唯一 `callsite_id`
- 调 `call_method_ic(...)`

例如 `loop_y` 里的三次：
- `step1` 调用点一个 id
- `step2` 一个 id
- `step3` 一个 id

---

## 1.4 runtime 侧改动

### 在 rt_func.cpp
新增：

```cpp
PYLANG_EXPORT_FUNC("call_method_ic_ptrs", "obj", "obj,i32,str,ptr,i32,obj")
py::PyObject *rt_call_method_ic_ptrs(...);
```

内部逻辑：

### fast path
1. 读取 `callsite_id` 对应缓存槽
2. 检查：
   - `owner->type() == cached.owner_type`
   - `owner_type->version == cached.type_version`
   - 若有实例 dict，确认未覆盖该名字
3. 命中则直接：
   - 若 `needs_self`，构造 `[self, args...]`
   - 调 `resolved_callable->call_raw(...)`

### miss path
回退当前 `rt_call_method_raw_ptrs`
并更新缓存。

---

## 1.5 语义如何保证

这个方案是完全 Python 3.9 语义安全的，因为：

- type 变了：失配
- 类字典改了：version 变了，失配
- 实例字典覆盖了方法：fast path 检测并失配
- data descriptor：缓存中记录 `is_data_descr`
- kwargs：保持原逻辑
- 子类覆写：owner_type 不同，失配

### 这不是静态绑定
而是**有守卫的动态绑定缓存**。

---

## 1.6 预期收益

对于 test.py 这种稳定对象调用，`63.1%` 这条链能被大幅压缩。  
这是最接近 CPython `LOAD_METHOD/CALL_METHOD`、`vectorcall` 风格的方向。

---

# 方案 2：把 AOT 原生函数调用改成真正的“vectorcall-like ABI”
这是第二优先级。

---

## 2.1 当前问题

当前 `rt_call_raw_ptrs` / `PyNativeFunction::call_raw` 仍然会：

- 把参数变成 `Value` 数组
- `call_raw`
- AOT 函数里再 `rt_value_array_get`
- 再还原到局部变量

虽然比 tuple 已经好很多，但还是厚。

---

## 2.2 改进目标

对 AOT 函数增加第二套调用 ABI：

### 当前 ABI
```cpp
ptr func(ptr module, ptr closure, ptr args_array, i32 argc, ptr kwargs)
```

### 新增 ABI
```cpp
ptr func_fast(ptr module, ptr closure, ptr* argv, i32 argc, ptr kwargs)
```

即直接传 `PyObject** argv`。

---

## 2.3 编译器改动

### PylangCodegen.cpp
在函数定义生成处，为 AOT 函数生成：
- 默认 ABI
- fast ABI 包装，或直接改为 fast ABI

函数入口直接：

- 从 `argv[i]` 取参数
- 不再通过 `rt_value_array_get`

这样 test.ll 里的：

- `rt_value_array_get`
- 局部参数重建

会直接消失。

---

## 2.4 runtime 改动

### `PyNativeFunction`
在 `src/runtime/PyFunction.hpp/.cpp` 中新增：

```cpp
using AotFastFn = PyObject *(*)(PyObject *module,
                                PyObject *closure,
                                PyObject **argv,
                                int32_t argc,
                                PyObject *kwargs);
```

`PyNativeFunction` 保存两个函数指针：
- `m_aot_ptr`
- `m_aot_fast_ptr`

命中 `m_aot_fast_ptr` 时直接走它。

---

## 2.5 语义

完全不变。  
这只是 ABI 精简。

---

## 2.6 收益

这会减少：

- `Value` 构造
- `rt_value_array_get`
- 参数搬运
- `variant` 访问

对所有 AOT 函数都有收益。

---

# 方案 3：为实例属性引入“split dict / shape / inline field cache”
这是第三优先级，收益非常大。

---

## 3.1 当前问题

`rt_getattr_fast` 现在对实例属性仍是：

- `inst_dict->map().find(Value(name))`
- 找到后 `RtValue::from_value(...)`

这对：

- `self.limit`
- `self.prime`
- `head.children`
- `head.terminal`

太贵。

---

## 3.2 目标

不要让普通 Python 对象每次属性访问都走 `PyDict<Value, Value>`。

### 引入 Shape（隐藏类）机制
给 heap object 增加：

```cpp
struct ObjectShape {
    PyType *owner_type;
    uint64_t version;
    std::unordered_map<PyObject *, uint32_t> attr_to_slot;
};
```

对象实例保存：

```cpp
ObjectShape *m_shape;
GCVector<Value> m_inline_fields;
```

---

## 3.3 行为

### 对普通实例属性写入
第一次 `self.limit = ...`：
- 若 shape 中无该属性
- 为 shape 分配 slot
- 对象存入 `m_inline_fields[slot]`

### 读取
`rt_getattr_fast` 对“普通 heap instance + 默认 __getattribute__”先走：

1. 取 `shape`
2. 用 interned `name_obj` 查 slot
3. 直接读 `m_inline_fields[slot]`
4. `RtValue::from_value`

只有失败时才回退实例 dict / `getattribute`

---

## 3.4 对语义的影响

可做到完全语义一致，但要注意：

- 若对象显式暴露 `__dict__`，则 shape 与 dict 必须同步
- 若类型覆写 `__getattribute__` / `__setattr__`，则不能走 shape fast path
- descriptor/data descriptor 仍优先于实例字段

### 推荐策略
仅对以下情况启用 shape fast path：

- heap type
- 默认 `__getattribute__`
- 默认 `__setattr__`
- 非 data descriptor 命中
- 对象有 `m_shape`

---

## 3.5 受益点

这会显著降低：
- `self.limit`
- `self.prime`
- `head.children`
- `head.terminal`

的访问成本。

对你的 benchmark，收益很大。

---

# 方案 4：重写 `ValueEq` / `ValueHash`，增加强快路径
这是第四优先级，但必须做。

---

## 4.1 当前问题

现在 `ValueEq::operator()` 太泛型：

```cpp
auto result = equals(lhs, rhs).and_then(...truthy...);
```

这对 dict key equality 来说成本过高。

---

## 4.2 目标

让 dict 热键比较优先走轻量路径。

### 建议重写 `ValueEq::operator()`

优先顺序：

1. **variant index 不同** 且不属于 Python 允许相等的特例，直接 false
2. `PyObject*` 指针相同，直接 true
3. `NameConstant/None/bool` 直接判
4. `Number` 对 `Number`：
   - 小整数直接比较
   - big int 再慢一点
5. `String` 对 `String`：
   - interned 或同地址直接 true
   - 长度不同直接 false
   - 最后再内容比较
6. `PyString*` 对 `PyString*`：
   - interned 指针直接比较
7. 只在最后才回退 `equals(...)->truthy(...)`

---

## 4.3 `ValueHash::operator()`
同样增加快路径：

1. `PyObject*` 若是 `PyString*` 且 cached hash 可用，直接取 cached hash
2. interned string 直接按 cached hash
3. `NameConstant`
4. small integer
5. big integer
6. 最后再回退对象 `hash()`

---

## 4.4 对文件的改动点

- PyObject.cpp
  - `ValueHash::operator()`
  - `ValueEq::operator()`

必要时给 `PyString` 增加：
- `bool is_interned() const`
- `size_t cached_hash() const`

---

## 4.5 语义

完全不变。  
只是把最常见 case 提前。

---

# 方案 5：属性名和单字符数字字符串的专门化
这是针对你的 benchmark 的高收益专项优化。

---

## 5.1 当前问题

`Node.children` 的 key 主要是：
- `'0'..'9'`

属性名主要是：
- `limit`
- `prime`
- `children`
- `terminal`
- `step1`
- `step2`
- `step3`
- `loop_x`
- `loop_y`
- `calc`

这些都是**稳定、极少量、重复出现**的 interned string。

---

## 5.2 改进方案

### 对属性名
在 `rt_getattr_fast` 与方法调用缓存里，只接受已 intern 的 `PyString*`。

### 对 trie 数字字符 key
给 `PyDict` 增加一个可选 fast mode：
- 若 key 全是单字符 digit string
- 则走 10 槽位数组或 tiny map

### 更稳妥一点
不改 `PyDict` 语义，而是在 `PyString::intern("0"..."9")` 基础上，给 `ValueHash/ValueEq` 对单字符 interned string 做极快路径。

---

## 5.3 收益

这对 `generate_trie` 阶段非常关键。  
日志里 `PyString` / `PyDict` 大量增长就说明这里很重。

---

# 方案 6：给 `PyDict` 增加“属性字典”和“普通字典”分离实现
这是结构性改进，收益比单纯优化 `ValueEq` 更大。

---

## 6.1 当前问题

现在所有 dict 都是同一个 `PyDict<MapType>`：

- 实例属性 dict
- 普通 Python dict
- kwargs
- trie children
- globals

但这些场景访问模式完全不同。

---

## 6.2 改进方案

### 方案 A：最小改动
保留 `PyDict` 接口，但内部增加 mode：

```cpp
enum class DictKind {
    Generic,
    Attribute,
    TinyStringKey,
};
```

### `Attribute`
- key 必须是 interned `PyString*`
- hash/eq 简化到指针级
- 支持 version

### `TinyStringKey`
- 少量短字符串 key
- 用小向量或固定槽优化

### `Generic`
- 保持原 `ordered_map<Value, Value>`

---

## 6.3 代码位置

- PyDict.hpp
- PyDict.cpp
- PyObject.cpp 中对象属性访问逻辑
- `ClassBuilder` / 实例对象创建流程中指定 dict kind

---

## 6.4 语义

不变。  
只是底层表示分化。

---

# 方案 7：降低 `RtValue::flatten / from_value` 频率，而不是只优化函数本身
这是关键思路。

---

## 7.1 当前误区
不要只想着把 `RtValue::flatten` 写得更快。  
vtune 里它热，往往表示：

- 调用次数太多
- 边界设计不对

---

## 7.2 正确做法

### 减少这些场景：
- dict find 后马上 `from_value`
- 运算前每次都 `flatten`
- 容器访问后又 `box`
- 参数传递时 `ensure_box`

### 具体方法
1. AOT fast ABI 去掉 `rt_value_array_get`
2. `rt_getattr_fast` 命中 shape slot 时尽量直接返回 raw 表示
3. `PyDict` 内部对小整数 / PyObject 指针路径减少 `Value` 来回包装
4. 对热方法调用直接传 `PyObject**`，不要先变 `Value` 数组

---

# 方案 8：把 `rt_getattr_fast` 改成真正的多级属性缓存
这是和方案 1 配套的。

---

## 8.1 当前状态
`rt_getattr_fast` 现在主要是：
- 先查实例 dict
- 否则 `getattribute`

这还不够。

---

## 8.2 改进目标

为每个属性访问点增加 inline cache：

### 编译器
在 `PylangCodegen::visit(const ast::Attribute *)` 中：
- 为每个 `LOAD_ATTR` 调用点生成唯一 site id
- 调新的 `rt_getattr_ic(obj, site_id, name_obj)`

### runtime
缓存：
- owner type
- type version
- attr kind（实例槽位/descriptor/类属性）
- slot index 或直接 resolved object

命中则直接：
- 读实例 slot
- 或返回 cached descriptor get 结果

失配则回退原 `rt_getattr_fast`

---

## 8.3 语义
完全可保留。  
这和 CPython specialization 很接近。

---

# 方案 9：保留 Python 语义前提下的“方法内联前移”
这个不是直接 inline，而是**compiler-assisted fast dispatch**。

---

## 9.1 方式
对于方法调用：

```python
self.step1(x, y)
```

编译器可以先发：

1. fast call site
2. miss bb
3. fallback bb

IR 形态像：

- type/version guard
- direct call AOT fn
- fallback `rt_call_method_raw_ptrs`

这会比单纯 runtime 内做更利于 LLVM 优化。

---

## 9.2 但实现次序上
我建议先做 runtime inline cache。  
稳定后再把 guard 下沉到 IR。

---

# 方案 10：GC / memset / 小对象分配优化
这是后续优化，不是第一优先级。

---

## 10.1 日志说明
`PyDict`, `PyString`, `PyTuple` 大量创建。  
这说明对象 churn 很高。

---

## 10.2 建议
1. `PyTuple::create()` 空元组/单元素元组缓存
2. `PyString` intern 表预热
3. `PyDict` 小对象 slab 分配
4. `Node()` / 小对象构造减少无用属性 dict 分配
5. allocator 减少全量清零

### 但注意
这些是“放大器优化”，不是第一根因。

---

# 五、针对你项目的具体落地顺序

我建议按下面顺序做，不要并行乱改。

---

## Phase 1：先把调用链压薄
### 目标
干掉 `rt_call_method_raw_ptrs` 这 63.1% 的大头。

### 具体改动
- PylangCodegen.cpp：方法调用点生成 `callsite_id`
- `IREmitter.hpp/.cpp`：新增 `call_method_ic`
- rt_func.cpp：实现 `rt_call_method_ic_ptrs`
- `PyType` / `PyDict`：提供可用 version 接口
- `PyNativeFunction`：支持 fast call path

### 预期
第一波就应有非常明显收益。

---

## Phase 2：再压属性访问
### 目标
把 `self.limit/self.prime/head.children/head.terminal` 降成本。

### 具体改动
- rt_attr.cpp：新增 `rt_getattr_ic`
- PylangCodegen.cpp：属性访问点生成 `site_id`
- 对 heap object 增加 shape/slot 或最小版 field cache
- fallback 仍保留 `getattribute`

---

## Phase 3：优化 dict key/value 核心路径
### 目标
降低 `ValueEq` / `ValueHash` / `from_value`

### 具体改动
- PyObject.cpp：重写 `ValueEq::operator()`
- PyObject.cpp：重写 `ValueHash::operator()`
- `PyString` 增 hash cache / interned fast path
- `PyDict` 增 `DictKind::Attribute`

---

## Phase 4：替换 AOT 调用 ABI
### 目标
减少 `Value` 数组/参数搬运。

### 具体改动
- `PyFunction.hpp/.cpp`
- PylangCodegen.cpp
- IREmitter.cpp
- runtime export 层

---

## Phase 5：最后再看 `RtValue` 细节
### 目标
减少 `flatten / from_value / box` 的尾部成本。

### 具体改动
- RtValue.cpp
- rt_op.cpp
- rt_cmp.cpp
- rt_attr.cpp

---

# 六、最值得你先实现的三个具体点

如果你现在只想做最有把握的三项，我建议就是：

## 1. `rt_call_method_raw_ptrs` → `rt_call_method_ic_ptrs`
这是最大收益点。

## 2. `ValueEq/ValueHash` 快路径重写
这是全局公共收益点。

## 3. `rt_getattr_fast` → `rt_getattr_ic`
这是第二大结构性收益点。

---

# 七、测试与验证建议

为了确保“高性能 + Python 3.9 语义”，每一步都要配套测试。

---

## 7.1 方法调用 cache 测试
要覆盖：

- 正常类方法调用
- 实例字典覆盖方法
- 子类覆写
- monkey patch 类方法
- data descriptor
- `__getattribute__` 覆写

确保：
- 命中快路径结果正确
- miss 回退正确

---

## 7.2 属性 cache 测试
覆盖：

- 普通实例属性
- descriptor 优先级
- `__dict__` 同步
- 删除属性
- 动态新增属性

---

## 7.3 dict 快路径测试
覆盖：

- interned string key
- 非 interned string key
- small int key
- big int key
- `1 == True` 等 Python 边界语义

---

# 八、最终结论

结合 top-down-tree.md，现在可以非常明确地说：

# 当前最核心的性能问题是：
## **AOT 代码在热点区仍然大量走通用方法调用链，而这条链又建立在过重的属性访问和 `PyDict<Value,Value>` 基础设施之上。**

因此最优改进方向不是“更激进地静态化数值”，而是：

1. **方法调用 inline cache**
2. **属性访问 inline cache / shape**
3. **dict key/value fast path 专门化**
4. **AOT fast ABI**
5. **最后再收敛 `RtValue` 边界成本**

这是最符合你当前架构、也最符合 Python 3.9 语义的高性能路线。

如果你要，我下一步可以继续直接给你：

## 一个“可执行实现清单”
按文件列出：
- 要新增哪些结构体
- 哪些函数要改签名
- 哪些缓存字段放在哪里
- 每个改动的伪代码流程

我可以直接按：
PylangCodegen.cpp → IREmitter.cpp → rt_func.cpp → PyObject.cpp → PyDict.cpp
这个顺序给你写成工程实施文档。