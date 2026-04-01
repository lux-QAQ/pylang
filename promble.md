已完成: *分析热点与根因* (2/3)

结论先说：

**当前 12s 的根因不是单一热点，而是“生成的 AOT 代码仍在高频走 Python 对象模型 + variant 装箱/拆箱 + 动态属性/方法调用 + 哈希字典语义”**。  
你的 test.py 本质是一个**整数密集 + 容器密集 + 方法调用极密集**的 benchmark，但生成的 test.ll 还没有把它降到“接近 C 的循环”。

所以它虽然叫 AOT，实际上执行形态仍然更像：

- Python 对象解释执行的 runtime fast-path 版
- 而不是专用化后的 native code

---

## 一、从 vtune 看，主要瓶颈在哪里

你给的热点很有代表性：

### Top hotspots
- `py::RtValue::flatten` 6.1%
- `__memset_avx2_unaligned_erms` 4.7%
- `PyResult<...>::PyResult<PyObject*>` 4.3%
- `py::ValueEq::operator()` 4.1%
- `py::RtValue::from_value` 3.8%

再结合 bottom-up 里还有：

- `py::PyObject::richcompare`
- `tsl::ordered_hash...`
- `py::PyObject::type_prototype`
- `std::_Hash_bytes`
- `py::ValueHash::operator()`
- `py::PyType::lookup`
- `rt_call_method_raw_ptrs`
- `rt_value_array_get`
- `py::PyObject::from<variant...>`
- `py::RtValue::box`
- `GC_mark_from`
- `py::equals`
- `py::PyString::intern`
- `py::as<py::PyInteger>`
- `_pthread_mutex_lock`

这说明程序的时间没有花在“筛法数学本身”，而是花在：

1. **整数值和 `Value`/`PyObject*` 之间来回转换**
2. **字典 key 的 hash / eq**
3. **动态属性查找 / MRO lookup / 方法调用封装**
4. **频繁构造临时对象、结果对象、variant 对象**
5. **类实例 `Node` 的 `children/terminal` 全都走通用对象模型**
6. **GC/内存清零成本高**
7. **字符串 intern 和哈希也在热路径里出现**

这不是“某个函数写慢了”，而是**整条执行链没有被专用化**。

---

# 二、最致命的问题：AOT 生成的 test.ll 仍然在高频调用 runtime

看你给的 test.ll 片段，`step1/step2/step3/loop_y` 里都是这种模式：

- `rt_integer_from_i64(4)`
- `rt_binary_mul`
- `rt_binary_add`
- `rt_getattr_fast(self, "limit")`
- `rt_compare_le`
- `rt_binary_mod`
- `rt_compare_eq`
- `rt_getattr_fast(self, "prime")`
- `rt_getitem`
- `rt_unary_not`
- `rt_setitem`
- `rt_call_method_raw_ptrs(self, "step1", ...)`

这意味着：

## 1）数值循环没有 lowering 成机器整数循环
例如：

```python
n = (4 * x * x) + (y * y)
if n <= self.limit and n % 12 == 1 ...
```

在 IR 里仍是对象运算调用，而不是：

- `mul i64`
- `add i64`
- `icmp sle`
- `srem i64`

这直接决定了你不可能接近 CPython 的 2s。  
因为 CPython 虽然也是对象语义，但它对 `int`、`list`、`dict`、attribute access 的实现是多年极限优化过的 C；  
你现在的路径是**自定义 runtime + 更重的抽象层**。

---

## 2）方法调用没有内联，循环被拆碎成大量 Python 风格调用
`loop_y` 里：

```llvm
call rt_call_method_raw_ptrs(self, "step1", ...)
call rt_call_method_raw_ptrs(self, "step2", ...)
call rt_call_method_raw_ptrs(self, "step3", ...)
```

这非常伤。

因为 `loop_y` / `step1` / `step2` / `step3` 是**静态可知、同类内调用、无多态需求**，本来应该：

- 直接 call 到内部函数
- 甚至进一步 inline
- 最终成为一个连续的双层整数循环

而现在每次都要经过：

- method name
- type lookup
- descriptor check
- instance dict check
- Value 数组构造
- `call_raw`
- 参数解包 `rt_value_array_get`

这在 `x,y` 双重循环里会爆炸。

---

## 3）属性访问 `self.limit` / `self.prime` 在热路径反复动态查找
在 `step1/2/3` 里频繁：

```llvm
rt_getattr_fast(self, @.pystr_obj.limit)
rt_getattr_fast(self, @.pystr_obj.prime)
```

而 `self.limit`、`self.prime` 对 `Sieve` 来说是**构造后不变布局**。  
这类字段应在 AOT 阶段尽量专用化为：

- 固定 slot offset
- 或进入函数前 hoist 到局部变量
- 或把 `step1/2/3` 直接改写成接受 `limit` 和 `prime` 参数

否则每次都要：

- 查实例字典
- 做 `Value(name)` 包装
- 做哈希/相等比较
- 取出 `Value`
- `from_value`

这正好对应了你 vtune 里的：

- `ValueEq::operator()`
- `ValueHash::operator()`
- `RtValue::from_value`
- `PyType::lookup`
- `PyObject::type_prototype`

---

# 三、为什么 `RtValue::flatten / from_value / box` 这么热

这三个热点其实暴露出一个事实：

## 你在“伪装成轻量 tagged int”，但调用链仍然不断被迫回到通用对象语义

### `flatten`
```cpp
RtValue RtValue::flatten(PyObject *ptr)
```

它会：

- 判断 tagged/null
- `as<PyInteger>`
- 读取 `mpz_class`
- 判断能否缩回 tagged
- 还要检查 `PyBool`

这个函数 6.1% 说明：

**大量路径本来已经是数字，却仍反复被塞回 `PyObject*` 再拿出来。**

### `from_value`
```cpp
RtValue RtValue::from_value(const py::Value &v)
```

它会：

- `visit variant`
- `Number` 分支判断 big int / fit slong
- `PyObject*` 分支 `flatten`
- 其他分支 `PyObject::from(other).unwrap()`

这更重。  
说明你的字典、列表、属性值、调用参数大量在 `Value` 里进出。

### `box`
`box()` 热说明 tagged int 经常又被迫装箱成 `PyInteger` 才能继续参与通用协议。

---

## 这代表 tagged pointer 方案目前只做了一半

你已经优化了：

- 算术/比较的一部分 fast-path
- 小整数缓存
- tagged int 表示

但是没有做到：

- **调用约定全链路 RtValue 化**
- **容器 key/value 全链路避免装箱**
- **AOT 代码本身不再走对象协议**

所以 tagged int 只是减轻了一部分成本，**没有改变整体执行形态**。

---

# 四、`ValueEq::operator()` 和 dict/list 路径为什么这么贵

你热点里 `ValueEq::operator()` 4.1%，`ValueHash`、`std::_Hash_bytes`、`ordered_hash` 也都很高。

这和 test.py 的 trie 结构强相关。

## `Node.children` 是 `dict[str, Node]`
插入 348513 个 prime 的十进制字符串时：

- 每个字符都要查 `if ch not in head.children`
- 然后 `head.children[ch] = Node()`
- 然后 `head = head.children[ch]`

这会引发大量：

- 字符串 key hash
- 字符串 key eq
- `Value` <-> `PyObject*` 转换
- `dict` 查找/插入
- `Node` 对象构造
- `children` 属性读取
- `terminal` 属性写入

日志里也已经证明了对象爆炸：

- `PyDict`
- `PyString`
- `PyTuple`
- `PyInteger`
- `Type`
- `PyStringIterator`

尤其 trace 里 `PyDict::create()` 出现在：

- `Node.__init__`
- `PyObject::__setattribute__`
- `generate_trie`

这说明：

## 你不是只在建 trie 节点时分配一个 dict
而是在**很多属性设置路径里不断创建/传播 dict/属性结构**。

---

# 五、`__memset_avx2_unaligned_erms` 很高意味着什么

`memset` 4.7% 很不正常，通常表示：

1. **大量对象/variant/容器在清零**
2. arena/GC allocator 在返回新内存时做了清零
3. `std::variant` / `PyResult` / `Value` / `PyTuple` / `PyDict` 等临时对象构造太多
4. 大量小对象分配导致 cache/memory bandwidth 被吃掉

结合日志和热点，基本可以判断：

## 当前程序不是算得慢，是“分配 + 清零 + 哈希 + 调度”慢

---

# 六、为什么 CPython 反而更快

这点很关键。  
直觉上 AOT 应该比 CPython 快，但现在反过来，原因是：

## CPython 的优势
对这个 benchmark，CPython 虽然没有编译成机器整数循环，但它有：

- 极成熟的 `PyLong` 小整数路径
- 极成熟的 `dict` 和 `str` 实现
- 很少的抽象层重复
- `LOAD_ATTR/CALL_METHOD` 多年的专门优化
- 很高效的对象布局与 freelist/缓存
- C 代码内联、branch predict、cache 行为都已被长期打磨

## 你的 runtime 当前劣势
你现在多了一整层额外开销：

- `PyObject*` / `Value` / `RtValue` 三层来回
- `PyResult` 错误传播包装
- `variant` visit
- 通用 richcompare / type lookup
- `call_raw_ptrs` + `rt_value_array_get`
- 自己的 dict key eq/hash
- 可能还有 GC barrier/mark 路径

所以虽然你是 AOT，**但并没有把动态性消掉**，反而增加了抽象成本。

---

# 七、根因按优先级排序

我按影响从大到小排：

---

## 根因 1：AOT IR 没有把热点循环专用化成原生整数循环
这是**第一根因**。

`Sieve.loop_x/loop_y/step1/step2/step3` 仍然是对象级 runtime 调用图，不是 SSA 整数循环。

### 结果
- 每步都走 runtime
- 每个常量都要 `rt_integer_from_i64`
- 每个算术都要 runtime dispatch
- 每次判断都要 bool 对象/truthy
- 每次字段读取都要属性访问

### 这个问题不解决，不可能到 2s。

---

## 根因 2：方法调用开销极大
`rt_call_method_raw_ptrs` 出现在最热循环里，是灾难性的。

`step1/step2/step3` 应该被**去对象化**。

### 结果
- `call_raw`
- `Value` 数组构造
- 参数解包
- descriptor / instance dict / lookup
- bound/unbound 路径判断

全都在内层循环发生。

---

## 根因 3：属性访问没有 hoist / slot 化
`self.limit`、`self.prime` 是稳定字段，却在热循环里不断：

- `rt_getattr_fast`
- dict find
- `ValueEq` / `ValueHash`
- `RtValue::from_value`

### 结果
大量 CPU 用在取字段，不是算数。

---

## 根因 4：`Value`/`RtValue`/`PyObject*` 三层表示互转太频繁
这是 vtune 最直接反映的问题。

### 结果
- `flatten`
- `from_value`
- `box`
- `PyObject::from(Value)`
- `PyResult<PyObject*>`

成片出现。

---

## 根因 5：Trie 数据结构本身对你的 runtime 极不友好
`dict[str, Node] + class Node` 对自研 runtime 是最差场景之一。

### 因为它需要
- 频繁字典查找
- 频繁小字符串 key 操作
- 海量小对象
- 动态属性访问
- 节点对象 + 属性字典 + children 字典 多层 indirection

对 CPython 这类场景它优化很多；对你现在的 runtime 则正中弱点。

---

## 根因 6：异常/结果封装模型过重
`PyResult<variant<...>>`、`PyResult<PyObject*>` 在热路径出现，说明：

- 本来不会失败的 fast-path 也在走统一错误封装
- 影响 inlining
- 增加构造/析构/分支

---

## 根因 7：GC / allocator / memset 成本偏高
这更多是“放大器”。

如果对象创建已经非常多，那么：

- arena 清零
- GC root/mark
- cache miss
- mutex / intern
- memset

都会放大成显著热点。

---

# 八、如何“彻底”提升到 2s 甚至更少

如果目标真的是 **2s 甚至更少**，只修补 runtime 不够，必须**编译器和 runtime 一起改**。

我分三层说：

---

# 第一层：必须做，决定能不能从 12s 降到 3~5s

## 方案 A：对 `Sieve` 热循环做整数专用化
对以下模式做 AOT lowering：

- 局部变量 `x, y, n, r, i` 推断为 `i64`
- 常量 `1, 3, 4, 5, 7, 11, 12, 500` 直接用 LLVM immediate
- `x*x`, `y*y`, `4*x*x`, `3*x*x` 直接发 `mul`
- `n <= limit`, `x > y` 直接 `icmp`
- `n % 12` 直接 `srem`

### 目标
把下面这类 IR：

```llvm
rt_binary_mul
rt_binary_add
rt_binary_mod
rt_compare_le
rt_compare_eq
rt_is_true
```

变成纯 LLVM 整数指令。

### 预期收益
这通常是**最大头**，单项可能就是数倍提升。

---

## 方案 B：消灭 `loop_y -> step1/step2/step3` 的动态方法调用
编译时知道：

- `self` 类型是 `Sieve`
- 调用目标是当前类定义的方法
- 无 monkey patch / 无继承覆写风险（若 AOT 假设允许）

那么直接生成：

- direct internal call
- 最好 inline `step1/2/3`

### 理想结果
`loop_y` 最终变成一个函数体内连续的三段整数逻辑，而不是 3 次 runtime 调用。

### 预期收益
非常大。  
因为这三个调用在双重循环里发生几百万次。

---

## 方案 C：把 `self.limit`、`self.prime` 提前 hoist 成局部
例如在 `calc` 或 `loop_x/loop_y` 开头：

- 读取一次 `limit`
- 读取一次 `prime`
- 作为参数传给内联后的 step 逻辑

而不是在 `step1/2/3` 每次重新 `getattr_fast`。

### 预期收益
显著降低：
- `rt_getattr_fast`
- `ValueEq`
- `ValueHash`
- `from_value`

---

# 第二层：强烈建议做，决定能不能从 3~5s 逼近 2s

## 方案 D：为 AOT 新增“native int calling convention”
你现在的问题之一是，即使值是 tagged int，也还在：

- `PyObject*`
- `Value`
- `RtValue`

三种形式之间折返。

### 建议
对编译器证明为 `int` 的函数和局部，直接生成专门 ABI：

- `i64` 传参
- `i64` 返回
- 分支条件直接 `i1`
- 只有逃逸到通用对象语义时才 box

例如：
- `step1_native(SieveFields*, int64_t x, int64_t y)`
- `step2_native(...)`
- `step3_native(...)`

或者更进一步直接消失成一个循环。

### 这是从“对象优化”到“原生专用化”的关键一步。

---

## 方案 E：为 `list[bool]`/`list[object]` 做专门容器
`self.prime = [False] * (limit + 1)` 是个大热点。

如果它现在仍是 Python list 语义对象：

- 下标访问走对象协议
- 元素是 `Value`
- 布尔是对象语义

那很慢。

### 最好做法
识别该数组在筛法里其实是**位图/字节数组**：

- `std::vector<uint8_t>` 或 bitset
- `prime[n]` 直接 indexed load/store
- `not prime[n]` 直接 xor/flip

### 收益
非常大。  
Atkin sieve 的核心就是大量数组读写。  
如果这仍走 Python list/object，基本注定输给 CPython。

> 甚至我会说：  
> **若你把 `prime` 专门化成原生 bitset，可能单项就能把总时间砍掉一半以上。**

---

## 方案 F：不要用 `Node` + `dict[str, Node]` 表示 trie
这是第二个大头。

对这个 benchmark，最佳结构不是 Python 对象 trie，而是原生结构：

### 方案 1：定长 10 分支节点
```cpp
struct Node {
    uint32_t child[10];
    bool terminal;
};
```

- 数字字符 `'0'..'9'`
- child 下标直接 `digit - '0'`
- 无字符串 key hash
- 无 dict
- 无 Node 属性查找
- 无 `ValueEq` / `ValueHash`

### 方案 2：干脆不用 trie
你的任务只是“找以 prefix 开头的 prime”。

那更简单：

#### 最优算法其实是：
- 筛出 prime
- 遍历 prime 列表
- 判断十进制前缀匹配
- 收集结果

甚至可以用数值区间做前缀判断，而非字符串 trie。

### 例如
prefix 为 `32338`，长度 `k=5`。  
对于一个数 `p`，只要把它裁剪到前 `k` 位看是否等于 `32338` 即可。  
或者简单转字符串，但原生字符串也比你当前 Node+dict 便宜。

### 重点
**当前 trie 是对 runtime 最不友好的 workload**。  
如果 benchmark 目标是性能，不应该拿这个结构去考验一个还未完全特化的 Python runtime。

---

# 第三层：runtime 层面的深度优化，决定能不能进一步超 CPython

## 方案 G：给 `Value` 增加“零装箱相等/哈希”快路径
现在 `ValueEq::operator()`：

```cpp
auto result = equals(lhs, rhs).and_then(...truthy...)
```

这太重。

对于 hot path，应先做：

1. 同类型直判
2. `PyObject*` 指针相等直判
3. interned `PyString*` 指针相等直判
4. tagged int / small int 直判
5. bool / none 直判

只在最后才回退到 `equals(...)->truthy(...)`。

### 尤其对 dict key
如果 key 是 interned 单字符字符串，这里本该几乎是 O(1) 指针比较。

---

## 方案 H：`ValueHash` 对常见键做更轻量 hash
现在：

- `String` 走 `std::hash<std::string>`
- `PyObject*` 走对象 hash
- `Tuple` 用 `elements.data()` 地址

对于 trie 场景，key 几乎都是**单字符数字字符串**。

### 可做
若 `PyString` 已 intern 且短字符串，可直接缓存 hash 或按指针 hash。

---

## 方案 I：把 `PyType::lookup` 结果缓存到底
你已经有 `rt_getattr` 的 `AttrCache`，但 `rt_call_method_raw_ptrs`、`getattribute`、descriptor 路径里仍反复查。

对 `Sieve.step1/2/3`、`Node.children/terminal` 这种稳定类型，应做到：

- call site inline cache
- monomorphic cache
- class version guard
- 命中后直接取 slot / callable

---

## 方案 J：减少 `PyResult` 在 hot path 出现
对不会失败的内部 fast-path 设计单独 API：

- `*_unsafe`
- `*_checked`
- `*_fast`

例如：
- `RtValue::compare_eq_fast`
- `dict_lookup_interned_fast`
- `getattr_known_field_fast`

不要让热路径每一步都携带异常通道。

---

## 方案 K：减少 `PyObject::from(Value)` 与 `RtValue::from_value`
如果容器内部是 `Value`，而调用方只是要比较/取 truthy，尽量直接在 `Value` 层完成，不要来回进入对象层。

---

## 方案 L：清理 `intern` 的锁竞争
出现 `_pthread_mutex_lock` + `PyString::intern`，说明 intern 表可能有全局锁。

若 AOT 常量字符串都在模块初始化阶段 intern 好，那么热路径就不应再触发新的 intern。  
尤其单字符 digit/string path，要避免运行时重复构造。

---

# 九、对当前 benchmark 的“最有效路线图”

如果你问我：**怎么最快把 12s 拉到 2s 附近？**

我建议按这个顺序做。

---

## 路线 1：先别追求“通用正确”，直接把 benchmark 打穿

### 第一步：把 `Sieve` 专用化
把以下函数编译成原生整数/原生数组版本：

- `Sieve.loop_x`
- `Sieve.loop_y`
- `Sieve.step1`
- `Sieve.step2`
- `Sieve.step3`
- `Sieve.omit_squares`

并把 `prime` 表示成：

- `std::vector<uint8_t>` 或 bitset

### 效果
这一项单独就可能让 12s 掉到 4s 甚至更低。

---

## 第二步：去掉 trie 的 Python 对象实现
不要用：

- `class Node`
- `children = {}`
- `terminal = False`

改成原生 digit trie，或者直接不用 trie。

### 更激进但更合理的方案
在筛出 prime 后直接做 prefix 过滤。  
这题其实没必要建 trie。

### 效果
这一项可能再砍掉 2~4 倍的 trie 部分时间。

---

## 第三步：只保留边界处的 Python 语义
例如：

- 输入输出还是 Python 语义
- 中间热函数 lowering 到 native region

这才是高性能 AOT 的正确方向。

---

# 十、如果暂时不改编译器，只改 runtime，能提升多少？

可以，但上限有限。

## 可以做的 runtime-only 优化
1. `ValueEq` 增加大量直判快路径  
2. `ValueHash` 对 interned/string key 优化  
3. `RtValue::flatten/from_value/box` 内联 + 缓存 + 避免 `mpz` 路径  
4. `rt_call_method_raw_ptrs` 再做 monomorphic inline cache  
5. `rt_getattr_fast` 对固定实例字段做 slot/cache  
6. 减少 `PyResult` 临时对象  
7. 减少 `PyObject::from(Value)` 装箱  
8. allocator 减少清零 / 批量分配 / arena slab 化

## 但问题是
这些优化只能把“对象 runtime”磨快，**不能把对象 runtime 变成原生循环**。

### 我的判断
- 只改 runtime：也许从 12s 到 6~8s，做得很好可能到 4~6s
- 想到 2s：必须改 AOT 生成策略

---

# 十一、直接对应你现在代码中的具体问题

## 1. `RtValue::flatten` 太热
说明大量路径已经有值但还要重新判型。

### 处理
- 让 AOT 热路径直接用 `RtValue`/`i64`
- 减少容器/属性读取返回 `Value` 后再 `from_value`
- 常见 `PyInteger` 存储直接带 tag 或 cached small int

---

## 2. `rt_value_array_get` 出现在 method body 入口
说明每次 AOT 函数调用都先把参数从 `Value` 数组解包成 `PyObject*`。

### 处理
- 对 native-specialized 函数不要用这个 ABI
- 对内部 direct call 用 typed params

---

## 3. `ValueEq/Hash` 高
说明 `dict` 太热。

### 处理
- trie 不要用 `dict[str, Node]`
- 或至少给 digit key 特化

---

## 4. `PyType::lookup` 高
说明动态方法/属性路径还在热循环里。

### 处理
- 直接 call internal function
- 直接 field access
- inline cache with version guard

---

## 5. `PyObject::richcompare` 高
说明比较仍走对象协议。

### 处理
- `x > y`, `n <= limit`, `n % 12 == c` 必须直接降为整数比较

---

## 6. `__memset` 高
说明临时对象和小对象太多。

### 处理
- 降低对象创建数
- 让热循环不分配
- trie 改 native layout
- `Value` 避免大规模构造/析构

---

# 十二、最关键的两个“彻底性改造”

如果只能选两项，我会选：

## 改造 1：为 AOT 引入 Native Region
把可证明类型稳定的循环区域降成：

- native integers
- native bools
- native arrays
- direct calls / inlined code

这是决定性改变。

---

## 改造 2：把 benchmark 中的 trie 改成非 Python 对象结构
即使语言语义层面保留 Python，编译器也应能把这种局部类/局部容器模式识别成：

- escape analysis 后替换为 native struct
- 或至少替换 `dict[str, Node]` 为 digit trie

这会极大降低：
- `PyDict`
- `PyString`
- `ValueEq/Hash`
- `from_value`
- `flatten`

---

# 十三、一个现实的性能预期

按经验估计：

### 现状
- 12s

### 只做 runtime 微优化
- 6~8s 比较现实
- 做得非常好可能 4~6s

### 做整数专用化 + 消灭动态方法调用
- 3~5s

### 再把 `prime` 变 bitset / native array
- 2~3s

### 再去掉 Python trie / 改为 native digit trie 或直接 prefix filter
- 有机会到 **2s 以内**

---

# 十四、建议你下一步优先做什么

按优先级：

1. **检查编译器能否把 `Sieve.step1/2/3/loop_y/loop_x` 直接 lower 到整数 IR**
2. **禁止热循环内生成 `rt_call_method_raw_ptrs`**
3. **禁止热循环内生成 `rt_getattr_fast(self, "limit"/"prime")`**
4. **把 `prime` 表示改成原生 bitset / byte array**
5. **不要用 `Node + dict[str, Node]` 跑这个 benchmark**
6. 然后再优化 `ValueEq/Hash`、`RtValue::flatten/from_value`

---

如果你愿意，我下一步可以继续两件事中的任意一个：

1. **直接结合 test.ll 给你列出“哪些 call 应该被编译器消掉” 的清单**  
2. **给出一个具体的 AOT 优化设计方案**，包括：
   - typed SSA
   - native region
   - direct call devirtualization
   - field hoisting
   - `list[bool]` 专用 lowering
   - trie/native struct lowering