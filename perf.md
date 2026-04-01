当前 12s 的主因不是算法本身，而是“生成出来的程序仍在大量执行 Python 运行时语义”，尤其是对象创建、属性访问、方法调用、`Value`/`PyObject` 来回转换和字典哈希比较。`vtune` 里的热点和代码能对得很齐。

**核心结论**

1. 筛法内层循环没有被编译成原生数值循环，仍然在做动态方法调用。
在 [test.ll](/home/lux/code/language/python-cpp/test.ll#L802) 到 [test.ll](/home/lux/code/language/python-cpp/test.ll#L842) 里，`loop_y` 每轮都在调用三次 `rt_call_method_raw_ptrs(self, "step1/2/3", ...)`，而不是直接内联 `step1/2/3`。`loop_x` 也一样调用 `self.loop_y(x)`，见 [test.ll](/home/lux/code/language/python-cpp/test.ll#L865) 到 [test.ll](/home/lux/code/language/python-cpp/test.ll#L909)。这意味着每次迭代都要走：
`descriptor lookup -> instance dict 检查 -> Value 数组构造 -> call_raw -> 参数解包`
对应运行时实现在 [rt_func.cpp](/home/lux/code/language/python-cpp/src/runtime/export/rt_func.cpp#L244) 到 [rt_func.cpp](/home/lux/code/language/python-cpp/src/runtime/export/rt_func.cpp#L324)。这类成本在 Atkin sieve 这种几百万次迭代里是致命的。

2. `RtValue` 的 tagged int 优化只优化了“算术结果”，没有优化“程序结构”。
`rt_binary_mul/mod/add/...` 最终会进 `RtValue` 快路径，见 [RtValue.cpp](/home/lux/code/language/python-cpp/src/runtime/taggered_pointer/RtValue.cpp#L110) 起；但每轮循环前后仍然要频繁 `flatten` / `box` / `PyObject* -> Value -> PyObject*`。`vtune` 第一热点就是 `py::RtValue::flatten`，这和 [RtValue.cpp](/home/lux/code/language/python-cpp/src/runtime/taggered_pointer/RtValue.cpp#L75) 完全一致。说明你现在是在“用很快的整数实现，支撑一个仍然很慢的对象执行模型”。

3. `Value` 的哈希和相等比较非常重，直接拖慢 `dict`/`children` 访问。
`vtune` 里有 `ValueEq::operator()`, `ValueHash::operator()`, `ordered_hash`, `PyObject::hash`, `PyObject::richcompare`。对应代码在 [PyObject.cpp](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L83) 到 [PyObject.cpp](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L125) 和 [PyObject.cpp](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L554) 到 [PyObject.cpp](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L712)。  
问题本质是：`head.children[ch]` 这种本该是“单字符 key 的简单 map 访问”，现在要走 variant 访问、对象 hash、对象 richcompare、truthy 判断，成本远超 CPython 对 unicode/int 的高度专门化实现。

4. `generate_trie()` 阶段存在海量对象分配，而且每个 `Node` 都触发完整的 Python 对象初始化链。
`Node.__init__` 在 [test.py](/home/lux/code/language/python-cpp/test.py#L4) 到 [test.py](/home/lux/code/language/python-cpp/test.py#L7)，而 `generate_trie` 在 [test.py](/home/lux/code/language/python-cpp/test.py#L72) 到 [test.py](/home/lux/code/language/python-cpp/test.py#L84)。  
日志直接证明了这里在狂分配：
`PyDict`, `PyTuple`, `Type`, `PyString`, `PyInteger` 持续暴涨，且堆栈都指向 `generate_trie -> Node() -> __init__ -> __setattribute__`。  
对应慢路径在 [PyType::call_raw](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L1293)、[PyObject::init_raw](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L1358)、[PyObject::__setattribute__](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L1636)。  
尤其是第一次给实例赋属性时会分配 `m_attributes` 字典，见 [PyObject.cpp](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L1660) 到 [PyObject.cpp](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L1674)。  
这会让每个 `Node` 至少带来：
实例对象 + `__dict__` + 类型相关对象访问 + 两次属性写入。  
你这里 trie 节点数很可能是几十万量级，所以这一块单独就足以吃掉数秒。

5. 生成的 IR 在 trie 遍历阶段仍然充满容器对象构造。
在 [test.ll](/home/lux/code/language/python-cpp/test.ll#L1240) 到 [test.ll](/home/lux/code/language/python-cpp/test.ll#L1264)，初始化 `queue, result = [(head, str_prefix)], []` 被编译成 `build_tuple + build_list + build_list + build_tuple + unpack_sequence`。  
在 [test.ll](/home/lux/code/language/python-cpp/test.ll#L1417) 到 [test.ll](/home/lux/code/language/python-cpp/test.ll#L1451)，`queue.insert(0, (v, new_prefix))` 每次都在构造新 tuple。  
这不是“小损耗”，而是把 Python 容器协议原封不动搬进了热路径。

6. 你的测试环境本身也很可能放大了慢速。
[compat.hpp](/home/lux/code/language/python-cpp/src/runtime/compat.hpp#L8) 到 [compat.hpp](/home/lux/code/language/python-cpp/src/runtime/compat.hpp#L54) 表明当前不是 `NDEBUG` 构建时会启用分配日志和堆栈追踪；`log.log` 里也确实出现了大量 `[DEBUG_ALLOC_TRACE]`。如果 12s 是在这个配置下测得，那这个数字本身就被“调试日志 + cpptrace”严重污染，不能和 release CPython 2s 公平比较。

**为什么 CPython 反而更快**

不是因为 CPython 算法更好，而是因为它对这些热点有专门优化，而你当前 AOT 输出没有真正“脱 Python”：

- CPython 对 `int`、`str`、`dict`、`list`、方法绑定、属性访问都有高度专门化实现。
- 你的 AOT 程序虽然是 native binary，但逻辑层面仍在频繁调用通用 runtime API。
- 结果就是“编译成了 ELF，但执行模型还是解释器式对象机”。

**怎么才能真正做到 2s 甚至更少**

先说结论：只修补 `RtValue`、再加几个 fast path，不够。想稳定打到 2s，必须做“编译器级去动态化”。

1. 先把性能测试改成干净基线。
必须用 `-DNDEBUG`、关闭 `PYLANG_ALLOC_LOG_FREQ/PYLANG_ALLOC_TRACE_FREQ`、关闭测试里的 `print`。不然你测到的是“调试运行时性能”，不是 AOT 性能。

2. 对 `Sieve` 做方法内联和对象字段去虚拟化。
`self.step1/2/3`、`self.loop_y`、`self.limit`、`self.prime` 都是静态可知的。
理想输出应把 [test.py](/home/lux/code/language/python-cpp/test.py#L49) 到 [test.py](/home/lux/code/language/python-cpp/test.py#L69) 直接降成一个原生循环：
- `x`, `y`, `n`, `limit` 变成裸 `i64`
- `prime` 变成裸数组/bitset 指针
- 不再出现 `rt_call_method_raw_ptrs`
- 不再出现 `rt_getattr_fast(self, "limit"/"prime")`

这是最重要的一步，单这一项就可能带来数量级收益。

3. 给 `list[bool]` 或筛法布尔数组专门表示。
现在 `self.prime = [False] * (limit + 1)` 仍然是 Python list 语义。  
如果编译器能证明这个 list 只用于整数下标读写布尔值，就应降成：
- `uint8_t*`
- 或 bitset
- 或 `std::vector<uint8_t>`
这样 `self.prime[n] = not self.prime[n]` 就是一次 load/xor/store，而不是 Python 下标协议。

4. 不要构建 `Node` Python 对象 trie，改成专门前缀过滤算法。
这段 workload 的目标只是找以 `32338` 开头的素数，根本不需要建整棵 trie。  
最直接的高性能方案：
- 在 `to_list()` 或筛结果遍历时，直接判断十进制前缀
- 或维护 `[prefix*10^k, (prefix+1)*10^k)` 区间
- 或把 prime 转字符串前先做数值过滤

如果保留 trie，也必须把 `Node` 降成原生 struct：
- `bool terminal`
- 固定 10-way child 索引数组，或紧凑 `small_vector<pair<char,node_id>>`
- 不要 `children = {}` 的 Python dict
- 不要实例 `__dict__`
- 不要 `Node()` 走 `PyType::call_raw`

这一项对当前测试的收益会非常大，因为现在日志已经显示 `generate_trie` 是对象分配地狱。

5. 给 AOT 类实例提供“静态字段布局”，禁用通用 `__dict__`。
像 `Node.children`, `Node.terminal`, `Sieve.limit`, `Sieve.prime` 这些字段在编译期已知。  
应当给 AOT class 生成固定 layout，而不是在 [PyObject::__setattribute__](/home/lux/code/language/python-cpp/src/runtime/PyObject.cpp#L1636) 里通过 `m_attributes` 字典存。  
也就是：
- 读取字段直接偏移 load
- 写字段直接偏移 store
- 无需 `type()->lookup(attribute)`
- 无需 `PyDict::create()`
- 无需 `ValueHash/ValueEq`

这是你运行时当前最大的结构性浪费之一。

6. 给单字符字符串和小整数 key 做专门 dict fast path。
如果短期内还不能把 trie 改写成原生结构，至少要把 `children[ch]` 优化掉：
- intern 后的单字符字符串可用 pointer identity 比较
- hash 可缓存
- `ValueEq` 不要回到 `richcompare`
- `dict[str->PyObject*]` 单独走专门模板，不走通用 `Value`

否则 `ValueHash/ValueEq/richcompare` 会一直是热点。

7. 消灭 `rt_value_array_get -> PyObject::from(Value)` 这类参数装箱往返。
[rt_value_array_get](/home/lux/code/language/python-cpp/src/runtime/export/rt_func.cpp#L106) 每次取参数都会把 `Value` 再转成 `PyObject*`。  
如果函数已知参数是整数/对象，应直接按 lowering 后的原生 ABI 传参，不该再走 `Value` 容器。

**优先级建议**

如果你想最快接近 CPython 2s，我建议按这个顺序做：

1. `release` 测试，去掉调试日志和打印。
2. 编译器把 `Sieve.loop_x/loop_y/step1/2/3/omit_squares` 全部内联成原生循环。
3. `prime` 降成裸布尔数组/bitset。
4. 不再构建 Python `Node` trie，直接做前缀筛选。
5. 若必须保留 trie，再做 AOT struct + 静态字段布局。
6. 最后才是继续抠 `RtValue`、`ValueHash`、`call_raw` 这类运行时微优化。

**一句话判断**

你现在慢，不是因为少了几个 fast path；而是因为 AOT 编译器还没有把这个程序从“Python 对象程序”降成“原生数据程序”。  
只要 `test.ll` 里还到处是 `rt_call_method_raw_ptrs`、`rt_getattr_fast`、`rt_build_tuple/list`、`rt_value_array_get`，就很难打过 CPython。想进 2s，必须让热路径里这些调用基本消失。

如果你愿意，我下一步可以继续直接帮你做两件事中的一个：

1. 先只做“分析落地版”，列出你编译器需要新增的 lowering/inlining/speculation 规则。
2. 直接在当前代码库里挑一个收益最大的点开改，比如“AOT 类静态字段布局”或“筛法循环原生化”。