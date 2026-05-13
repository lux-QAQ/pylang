当前 `test7` 慢的根因不是一个点，而是 AOT IR 仍然在热循环里“像解释器一样”跑 Python 对象协议。

最关键的证据在 `test7.ll` 的 `bench_small_int`：循环里每次都在调用：

- `rt_integer_from_i64`
- `rt_binary_mul/add/sub/and/xor/lshift/rshift`
- `rt_compare_*_bool`

也就是说，`x/y/i/checksum/z` 这些明显的小整数局部变量仍然是 `PyObject*`，常量也在循环里反复创建。例如 `1103515245`、`2147483647`、`1`、`255` 都是每轮 `rt_integer_from_i64`。这直接解释了 `perf.md` 里的：

- `GC_malloc_kind` 8.0%
- `GC_mark_from` 6.0%
- `PyInteger::create`
- `RtValue::flatten`
- `rt_binary_and/add/xor`

这些对象分配和 GC 成本大多来自小整数热循环装箱，而不是“大整数本身”。

**最优先改进方向**
1. **Codegen 做 typed integer lowering**
   对 `bench_small_int`、`bench_functions` 这类局部整数循环，生成 `i64` SSA，而不是 `PyObject*`。
   安全做法是：进入循环前 guard exact int，循环内用 `i64`，溢出或非 exact-int 时 fallback 到现有 runtime。像 `& 0x7fffffff` 这种模式可以保证回到小整数范围，收益会非常大。

2. **至少先 hoist 循环内整数常量**
   现在 IR 每轮创建常量对象。即使暂时不做完整 typed lowering，也应该把 `rt_integer_from_i64(1/255/2147483647/...)` 提到函数 entry 或模块初始化里复用。这个改动语义安全，能直接减少 `PyInteger::create + GC_malloc`。

3. **扩展 raw/tagged ABI**
   现在 `rt_binary_and(PyObject*, PyObject*)` 仍然太重。需要增加类似：
   - `rt_binary_and_i64`
   - `rt_binary_xor_i64`
   - `rt_binary_add_i64_checked`
   - `rt_compare_lt_i64`
   
   或者让 Codegen 直接发 LLVM integer op。现有 tagged pointer 重构只降低了一部分边界成本，但没有贯穿热循环。

4. **内建函数和方法调用需要专用路径**
   `bench_containers` 里仍有很多：
   - `rt_load_global(len/range/tuple/set/bytearray)`
   - `rt_call_method_ic_ptrs(d.get/st.add/stack.append/stack.pop)`
   
   对 `len(list/dict/set)`、`dict.get`、`dict.pop`、`set.add`、`list.append/pop` 应增加 direct export fast path，并让 Codegen 在已知内建名未被覆盖时直接发专用调用。

5. **dict/set 哈希表现在为正确性付了性能税**
   `RtValueHash/RtValueEq` 出现在热点里，尤其是 `dict.get` 和 `set contains`。这是之前修复 boxed/tagged int 正确性后的代价。下一步可以给 exact int key、exact str key 加专门 fast path，避免每次走完整 `richcompare/hash`。

另外，`perf.md` 里还有 `_pylang_debug_log_alloc`，说明这次 profile 仍带 debug allocation trace 或 debug build 噪声。正式性能对比要用 release，并确保：

```bash
PYLANG_ALLOC_TRACE_FREQ=0 PYLANG_ALLOC_LOG_FREQ=0
```

**结论**
当前最大问题已经不在单个 runtime 函数，而在 Codegen 仍把纯整数热循环生成为对象级 Python ABI。要大幅超过现在，优先级应是：先做小整数 typed lowering/常量 hoist，再做内建容器 direct fast path，最后再细抠 GC 和 dict/set 哈希实现。