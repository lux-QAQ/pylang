Summary
当前瓶颈不是单一 GC 参数，而是三层问题叠加：

perf.md 里的 2GB 第一名主要来自 rt_init() 的硬编码 GC_expand_hp(256MB * 7)，这是人为假峰值。
Finalizer Table 预膨胀能变快，是因为 BDWGC 在 GC_grow_table() 扩 finalizer 表前会触发 full GC；根因是 runtime 仍在大量注册 finalizer，尤其 PyString(std::string)。
真正业务热点仍在 test.py/test6.py：[False] * N 大 list、self.prime[n] 下标、str(int) 生成大量 PyString、trie 的 Node + dict + string key 对象洪水。
Key Changes
替换 rt_init() 的奇怪 GC hack：

删除 500 多万 dummy finalizer 注册/注销。
删除或缩小硬编码 GC_expand_hp(256MB * 7)，改为环境变量控制初始 heap，例如 PYLANG_GC_INITIAL_HEAP_MB，默认小值或 0。
增加 GC_set_allocd_bytes_per_finalizer(0)，避免 finalizer 数量直接触发 full GC，用它替代 finalizer table 预膨胀。
保留 GC_set_free_space_divisor，但也做成 env 可调，默认先保持 3。
大幅减少 finalizer 来源：

第一优先改 PyString：不要让热路径字符串依赖 std::string 析构。
引入 GC 托管字符串存储：短字符串 inline 或 GC_MALLOC_ATOMIC 字节缓冲，PyString 变成可 PYLANG_GC_FORCE_TRIVIAL 的对象。
create_raw(str(int)) 和单字符字符串都走 finalizer-free 路径。
保留 Python 3.9 str 语义：内容不可变、hash/eq/iter/slice/repr 行为不变。
优化 PyList 大对象 churn：

修改 PyList::__mul__，不要先构造 std::vector<Value> 再复制进 GCVector。
新增内部构造路径：直接在 PyList::m_elements 中 reserve/resize/fill。
对 [False] * (limit + 1) 增加 packed-bool list 表示：读取、写入 bool 时不物化 PyBool*，遇到非 bool 写入/append/公开 elements() 时 promote 为普通 list。
rt_list_getitem_i64_truthy 对 packed bool 直接返回 bool，rt_list_setitem_i64 对 bool 直接写 bit/byte。
继续压低 tagged/object 边界税：

在 export 层新增 raw bool/int API，但保持旧 ABI 不变。
rt_compare_le_bool、rt_is_true_fast、rt_list_getitem_i64_truthy 尽量返回 C++ bool，避免 py_true()/py_false()。
将 py_true/py_false/py_none 的热路径改为初始化后全局 raw 指针访问，减少函数调用和 static guard 成本。
针对 trie 的 runtime-only 优化：

给 PyDict 增加 small-char-key 模式：只针对单字符 str key，保存 256 槽索引 + insertion order。
遇到非单字符 key、复杂删除或需要通用 dict 行为时 promote 回 ordered_map。
items()/keys()/values() 保持 Python 3.9 插入顺序语义。
Test Plan
GC/内存：

对比删除 dummy finalizer 后 test.py/test6.py 的 wall time、RSS、GC_mark_from。
分别测试 GC_set_allocd_bytes_per_finalizer(0) 开关。
确认 perf.md 不再被 GC_expand_hp 和 cpptrace/libdwarf 噪声主导。
Python 语义：

str(int)、单字符字符串、字符串 hash/eq/slice/iter/repr。
list repeat、get/set、负下标、slice、append、insert、sort、repr。
packed bool list promotion：写入 1、None、自定义对象后行为正确。
dict 单字符 key：插入顺序、覆盖、删除、items 迭代、普通 key 混用。
回归：

SimpleDriverTest.Feature_StressTest
SimpleDriverTest.Feature_DataStructures
RuntimeAPITest.ListOperations
RuntimeAPITest.IndexOutOfRange
PylangCodegenTest.StoreSubscript