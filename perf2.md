搜索正则表达式 `map\(|find\(|insert\(|contains\(|class PyDict|using MapType|ValueHash|ValueEq` (`**/src/runtime/PyDict.*`)，40 个结果

读取 [](file:///home/lux/code/language/python-cpp/src/runtime/export/rt_op.cpp#1-1)，行 1 到 120

读取 [](file:///home/lux/code/language/python-cpp/src/runtime/export/rt_cmp.cpp#1-1)，行 1 到 120

读取 [](file:///home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#700-700)，行 700 到 860

读取 [](file:///home/lux/code/language/python-cpp/src/runtime/PyDict.hpp#1-1)，行 1 到 180

读取 [](file:///home/lux/code/language/python-cpp/src/runtime/PyDict.cpp#1-1)，行 1 到 260

读取 [](file:///home/lux/code/language/python-cpp/src/runtime/export/rt_convert.cpp)

你这次的质疑是对的。  
**核心问题确实不是“有没有 `RtValue` fast path”**。你已经有了。  
真正的问题是：

# 核心性能问题不在“叶子运算”，而在**热路径的大部分时间根本不在 RtValue 内部**

更准确地说：

## 你的快路径是“局部存在”的  
但程序的大头开销发生在 **RtValue 之外的胶水层**：

- `PyObject* ↔ RtValue`
- `Value ↔ RtValue`
- `dict/attr/method` 的泛型协议
- `ValueHash / ValueEq`
- `call_raw / kwargs / args 打包`
- `ensure_box / from_value / flatten`

所以现在的执行形态是：

$$
\text{fast arithmetic} \quad \text{被} \quad \text{慢的对象模型/容器模型/调用模型} \quad \text{包围}
$$

---

# 一、为什么“已经有 RtValue fast path”仍然慢

从代码看，rt_op.cpp / rt_cmp.cpp 里确实已经在做：

- `rt_binary_mul` → `RtValue::mul(flatten(lhs), flatten(rhs))`
- `rt_compare_gt` → `RtValue::compare_gt(flatten(lhs), flatten(rhs))`
- `rt_is_true` → `RtValue::flatten(obj).is_truthy()`

这说明：

## **算术和比较本身不是完全没优化**
这点要修正我之前的表述。

但问题是：

### 这些快路径只覆盖了“单个运算节点”
而没有覆盖：

1. 值是怎么从容器/属性里取出来的
2. 方法是怎么被调用的
3. 参数是怎么传的
4. 字典 key 是怎么比较/哈希的
5. 取出来后又怎么重新回到容器/对象层

---

# 二、CPython 为什么“同样动态”却快得多

你说得很关键：  
**CPython 也是动态的。**

所以真正的问题不能简单归结为“你没静态化”。

## CPython 快，不是因为它静态
而是因为它在动态语义下做到了三件事：

### 1. **表示统一**
CPython 基本全程就是 `PyObject*` 体系。  
它没有你现在这种频繁的三层切换：

- `PyObject*`
- `RtValue`
- `Value`

而你现在很多热点恰恰来自这三层之间的边界。

---

### 2. **动态热点优化覆盖的是 dict / attr / call**
CPython 的大头优化不是“整数乘法本身有多神”，而是：

- `dict` 极强
- `unicode` 极强
- `attribute access` 极强
- `method call` 极强
- `vectorcall` 极强

而这些正好是你 benchmark 的真正大头。

---

### 3. **公共慢路径非常薄**
CPython 的通用对象协议虽然动态，但实现非常扁平、缓存友好、分支少。  
你现在的通用层比较厚：

- `PyResult`
- `variant`
- `ValueHash/ValueEq`
- `from_value`
- `flatten`
- `box`
- `ensure_box`

这些单次都不一定贵，但在几千万次热路径里会爆炸。

---

# 三、你这个 benchmark 的核心性能问题到底在哪里

我现在给一个更准确的判断：

# **核心问题 = 热点主要消耗在“泛型对象/容器/调用基础设施”，而不是数值运算本身**

可以拆成两个核心层面。

---

## 核心问题 A：`dict + attr + string key` 这条链太重

这是最重要的。

你当前 trie 和对象属性都大量依赖：

- `PyDict<MapType = ordered_map<Value, Value, ValueHash, ValueEq>>`

这意味着每次：

- `head.children[ch]`
- `ch not in head.children`
- `self.limit`
- `self.prime`
- `head.terminal`
- `head.children`

本质都在走：

1. `Value` 包装 key
2. `ValueHash`
3. `ValueEq`
4. 找到 `Value`
5. `RtValue::from_value(...)` 或 `PyObject::from(...)`

而你的 `ValueEq::operator()` 是：

```cpp
auto result = equals(lhs, rhs).and_then([](const auto &result) { return truthy(result); });
```

这个实现语义很完整，但**作为 dict key equality 太重了**。

---

### 为什么这比你想象中更致命？

因为 trie 的 key 是**单字符字符串**，操作密度极高。  
对这种 workload，CPython 的优势非常大：

- `str` 有缓存 hash
- `unicode` 比较快
- dict 针对字符串 key 非常成熟
- attribute cache / type version cache 很成熟

而你这里每次查 key 仍然在走：

- `Value` variant
- generic hash
- generic eq
- 可能的 `equals/truthy`

所以：

## **你不是输在“动态语义”，而是输在“动态容器基础设施太重”**

---

## 核心问题 B：你的 fast path 是叶子级的，但热路径是“运算 + 胶水”

例如 `step1` 表面上是数值密集，但真实执行是：

- 取 `x`
- 取 `y`
- `mul`
- `mul`
- `add`
- 取 `self.limit`
- `compare`
- `is_true`
- `mod`
- `compare`
- `is_true`
- 取 `self.prime`
- `getitem`
- `not`
- 再取 `self.prime`
- `setitem`

这里真正昂贵的不是单个 `mul`，而是：

- `self.limit` 的属性访问
- `self.prime` 的属性访问
- `prime[n]` 的容器访问
- bool/结果的通用表示
- 这些操作之间的表示转换

所以即使 `mul` 已经走了 `RtValue`，  
**整个语句仍然大部分时间在慢层里。**

---

# 四、当前最核心的“结构性问题”是什么

如果必须压缩成一句话：

# **你的 runtime 现在最核心的问题，是“值表示分裂 + 容器协议泛化过度”，导致快路径覆盖率很低。**

不是没有 fast path。  
而是 fast path 只覆盖了最里面一小圈。

---

## 具体表现为 3 个“割裂”

### 1. 值表示割裂
你同时有：

- `PyObject*`
- `RtValue`
- `Value`

于是热路径不断出现：

- `flatten`
- `box`
- `from_value`
- `to_value`
- `ensure_box`

这不是单个函数慢，而是**体系切换成本**。

---

### 2. 容器表示割裂
你的 `PyDict` 存的是 `Value`，不是统一对象指针，也不是统一轻量值。

这导致：

- key lookup 走 `ValueHash/ValueEq`
- 取值还要 `from_value`
- 插入还要 `to_value`

这条链在 trie / attr / globals / kwargs 里到处都是。

---

### 3. 调用协议割裂
你的 AOT 调用虽然已经避免了部分 tuple 分配，但仍然要经过：

- `call_method_raw_ptrs`
- `Value` 栈数组
- `call_raw`
- AOT/native function wrapper
- `rt_value_array_get`

所以调用本身仍然比 CPython 的成熟向量调用链更重。

---

# 五、从你给的代码能直接看出的关键证据

---

## 证据 1：算术已 fast，但热点仍在 `flatten/from_value/ValueEq`
这说明问题不在“有没有 arithmetic fast path”，  
而在**快路径周围的转换和容器协议**。

你给的 vtune：

- `py::RtValue::flatten`
- `py::RtValue::from_value`
- `py::ValueEq::operator()`

这三个一起出现，已经很说明问题：

## 真正烧 CPU 的不是 `mul`，而是“值进出容器/协议”的成本

---

## 证据 2：`rt_getattr_fast` 仍然走实例 dict + `Value`
`rt_getattr_fast` 里：

```cpp
auto it = map.find(py::Value(name));
if (it != map.end()) { return py::RtValue::from_value(it->second).as_pyobject_raw(); }
```

这说明一次属性访问仍然要走：

- `Value(name)`
- `map.find`
- `ValueHash/ValueEq`
- `from_value`

而 benchmark 里 `children`、`terminal`、`limit`、`prime` 都是高频属性。

---

## 证据 3：`PyDict` 的 key eq/hash 是完全泛型的
`PyDict` 用的是：

```cpp
tsl::ordered_map<Value, Value, ValueHash, ValueEq>
```

这对正确性很好，但对你的 benchmark 非常不友好。

因为 benchmark 的高频 key 实际上是：

- interned attribute names
- 单字符字符串
- 小整数

这些都应该有**极轻的特化路径**，  
但现在仍被塞进通用 `ValueHash/ValueEq`。

---

## 证据 4：`call_raw` 仍然围绕 `Value` 运转
`PyNativeFunction::call_raw`、`PyBoundMethod::call_raw`、`PyObject::call_raw`  
都还是围绕 `Value` 数组/tuple 创建在转。

这说明你的 AOT 调用已经比最原始方案好，但**离 CPython 的 vectorcall 那种极薄调用层还有差距**。

---

# 六、所以“核心问题”到底该怎么定性

我现在更愿意这样定性：

# **不是数值语义太动态，也不是类调用太动态。**
# **而是你的“动态基础设施”本身太贵。**

具体说，是这三块最贵：

---

## 1. `ValueHash / ValueEq` 主导的泛型字典层
这是 trie、attr、kwargs、globals、instance dict 的公共瓶颈。

### 这是当前最像“系统级根因”的点。

---

## 2. `RtValue <-> Value <-> PyObject*` 的表示边界成本
这解释了：

- `flatten`
- `from_value`
- `box`
- `ensure_box`

为什么这么热。

### 这是第二根因。

---

## 3. 调用 / 属性 / 容器 API 的包装层太厚
这解释了为什么即使有 fast op，整体仍慢。

### 这是第三根因。

---

# 七、如果只问“最核心一刀砍在哪里”

如果只让我选一个最核心点，我会选：

# **`PyDict<Value, Value>` 这条泛型容器链过重，是当前性能塌陷的中心。**

原因：

1. trie 大量依赖它
2. attribute access 依赖它
3. globals / kwargs 依赖它
4. `ValueEq/Hash` 正好在热点里
5. 它还把 `RtValue` 的收益吃回去了，因为每次进出 dict 都要转表示

---

# 八、和 CPython 的真正差距在哪里

真正差距不是：

- “你没静态化，CPython 静态化了”  
  这不对，CPython 也没静态化。

真正差距是：

# **CPython 的动态热点优化主要集中在对象模型基础设施；而你现在的优化主要集中在算术叶子节点。**

这就是方向偏差。

你已经优化了：
- `RtValue::mul/add/mod/compare`

但 CPython 赢你的地方更多在：
- dict
- str
- attr
- method call
- call convention
- object layout

---

# 九、所以接下来最该分析/优化的不是 `RtValue::mul`

而是这三个方向：

## 1. `ValueEq / ValueHash` 的专门化
优先支持：
- interned `PyString*`
- 单字符数字字符串
- `PyObject*` identity
- tagged int

不要让 dict key equality 每次都走 `equals(...).truthy(...)` 这种完整协议。

---

## 2. `PyDict` 的热路径专门化
至少区分：
- 属性字典
- 普通 Python dict
- 单字符 trie 字典

因为这三种 workload 完全不同。  
现在全压成一个 `ordered_map<Value, Value>` 太贵。

---

## 3. 降低 `Value / RtValue / PyObject*` 边界往返
特别是：
- dict find 结果
- attr get/set
- method 参数传递
- 容器元素访问

---

# 十、最终结论

我这次给一个更收敛的结论：

# **当前程序性能差的核心，不是“缺少 RtValue fast path”，而是“快路径只在算术节点上，真正的大头却在泛型对象基础设施上”。**

最核心的性能根因是：

1. **泛型 `PyDict<Value, Value>` 太重**
2. **`Value / RtValue / PyObject*` 三层表示反复切换**
3. **属性访问、方法调用、容器访问的公共胶水层过厚**

所以：

- 不是要优先把一切静态化
- 也不是优先做更激进的数值分析
- 而是要先把 **动态对象基础设施** 做到接近 CPython 的级别

---

如果你愿意，我下一步可以继续只做一件更具体的事：

## 我可以直接把当前 test.py 的热点路径拆成：
- `Sieve` 阶段
- `Trie` 阶段

然后给出**每个阶段真正最贵的 3 条调用链**，明确指出：
- 哪条是 `dict` 开销
- 哪条是 `attr` 开销
- 哪条是 `RtValue` 边界开销