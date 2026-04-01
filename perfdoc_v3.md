# Pylang AOT 编译器极速性能重构指南 (v3.0)

基于最新讨论与架构约束，针对 AOT 生成代码 12s 的性能瓶颈进行深度剖析与方案修正。

---

## 优化一：关于多态内联缓存 (PIC) 的架构边界纠正

前一版方案试图在 AOT (前端 IR) 中直接生成 `if-else` 的缓存判定逻辑，但这会引致极其庞大的 IR 膨胀且破坏前端与 Runtime 的抽象边界。正确的做法是将缓存中心下沉至 **Runtime Export 层**，并且**放到独立的文件中管理（如 `src/runtime/export/rt_cache.cpp`）**。

### 1. 修改思路
- **前端 (IR/Codegen)**：只需为每一个方法/属性调用点（Call Site）分配一个独立的、预初始化的 `InlineCache*` 全局变量指针，然后直接将它传给 Runtime。
- **后端 (Runtime/Export)**：接收缓存结构指针。如果在 C++ 侧判定命中，则直接在 Runtime 层面调用对应的 C 函数；如果未能命中，触发慢路径并自动更新该指针的内容。

### 2. 代码落地设计
- **`rt_cache.hpp / rt_cache.cpp` (独立拆分)**
  ```cpp
  struct MethodCache {
      py::PyType* expected_type;
      uint64_t type_version;
      py::NativeFuncPtr fast_func_ptr;
  };

  extern "C" py::PyObject* rt_call_method_ic(
      MethodCache* cache, py::PyObject* self, const char* name, py::PyObject** args, size_t argc) {
      if (self->type() == cache->expected_type && self->type()->version == cache->type_version) {
          // 极致快路径：纯 C++ 函数指针调用
          return cache->fast_func_ptr(self, args, argc);
      }
      // 慢路径与自身维护...
      return rt_call_method_fallback_and_update_cache(cache, self, name, args, argc);
  }
  ```

---

## 优化二：追根溯源 `std::variant<...>` 与“清理参数变体”

### 1. Variant 的根源究竟在哪？
在 `vtune` 报告中极其吸血的 `py::PyResult<std::variant<...>>` 和引发 `__memset_avx2` (~5% CPU) 的罪魁祸首，来源于 **`src/runtime/Value.hpp` 中的 `py::Value` 定义**。
在该文件中，`Value` 本质上就是一个多重变体类型（涵盖 `BigIntType, py::Number, py::String, py::Tuple, py::PyObject*` 等）。在项目初期（以及目前的 `Interpreter` 解释器体系中），这可能是为了优化基础数值在栈上的无堆分配。

### 2. 为何在 AOT 阶段它是毒瘤？
`std::variant` 虽然能包容万物，但其尺寸是最大的内部成员（例如 BigInt 或 String 类实体）的内存对齐结果，通常会达到几十个字节。
当 AOT 生成调用 `rt_call_raw_ptrs` 时，它要将轻量的指针打包封装进这长达 32~40字节的 `Value` 对象数组里。这就引入了：
- **大量无效的 memset**（填充对其内存）。
- **沉重的标签判定 (Tag check)**。
- 这也是 `RtValue::from_value` 和 `flatten` 霸榜 10% CPU 时间的原因——AOT 每次都要将这团沉重的 `variant` 重新降级为 8 字节的 `RtValue`。

### 3. 重构方案：全面拥抱 `PyObject*` 或 `RtValue` ABI
AOT 和 Runtime 间的传参既然已经有能存极速小整数兼顾指针的 `RtValue(8 bytes)`，就应彻底将 `variant` (即 `Value`) 踢出关键路径。
- **原生接口修改**：诸如 `PyNativeFunction::call_raw` 应当将 `std::span<std::variant<...>>` 签名，强制改写为 `py::PyObject** args` 或 `py::RtValue* args`。
- **废弃不必要的装箱**：AOT 直接生成装填 8 字节指针的数组，使得压栈/传流时间立减至少 50%。

---

## 优化三：对象插槽化 (Object Slots) 

### 1. 现状痛点
因为现阶段所有对象实例（哪怕是高频运算中的 `Node` 和 `Sieve`）都使用基于 `std::variant` 为 Key/Value 的 `PyDict`（即 `unordered_map<Value, Value>`）作为自身的 `__dict__`。
这意味着：执行 `self.prime[n]` 时不仅会有变体创建、哈希分配，还会发生繁重的内存链表查询。

### 2. 什么是插槽化（Hidden Classes / Shapes）？
V8 引擎和现代 Python 都采用此优化：不再使用字典，而是基于对象的**形状**。
- **第一步：拆分字典**。对象的属性分配结构提取出来成为 `Shape`，所有具备 `[x, y, prime]` 成员的 `Sieve` 共享同一个 `Shape` 指针。
- **第二步：数组化存储**。对象实例内部仅保留一个定长的普通指针数组：`py::PyObject* slots[3]`。
- **第三步：高速定位**。当 Runtime 通过 Cache (见优化一) 验证对象 `Shape` 一致后，直接告知底层：`prime` 存放在 `slots[2]`。从字典检索坍缩成了一句绝对的 O(1) 数组偏移拿装 `return self->slots[2];`。

### 总结
你完全可以在 `src/runtime/export/rt_cache.cpp` 这种独立中心构建起**纯 C++ 的高速指令缓存与形状缓存**，从而消灭前端生成复杂判断的怪异设计。彻底干掉作为过渡产物的 `std::variant`，转而以 `RtValue` 穿透所有的传参墙壁并配以 Shape 机制，这三管齐下必将实现 CPython 级别的颠覆级性能提升。