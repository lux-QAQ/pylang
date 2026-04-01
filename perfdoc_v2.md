# Pylang AOT 编译器极速性能重构指南 (v2.0)

基于对 Intel VTune 分析报告 (`r048.csv`) 及向下调用树 (`top-down-tree.md`) 的深入数据剖析，我们发现 `test.py` AOT 编译后执行需 12s 的**根本原因不再被误认为是“缺少完全的整型静态化”**，而是**AOT 生成的 IR 代码在运行期过度依赖了极其沉重的动态抽象和派发层**。

核心罪魁祸首如下：
1. **高达 63.1% 的 CPU 时间**耗费在 `rt_call_method_raw_ptrs` 上。这意味着每一次热点循环内部的方法调用（如 `self.step1(x, y)`）都在高频执行极其昂贵的哈希查表、字符串比对及动态类型字典遍历。
2. **高达 91.9% 的总瓶颈**压在了以 `std::variant<...>`（即当前框架中的 `py::Value`）为介质的传参解包与装箱流程上（`py::PyNativeFunction::call_raw`、`flatten`、`from_value`、`__memset` 等）。

为了达成甚至超越 CPython 2s 的性能目标，且**绝不破坏 Python 3.9 的动态语义**，我们需要摒弃“局部修补 RtValue”，转而在 AOT IR 层与 Runtime 数据流转层实施以下三大核心重构。

---

## 核心改造一：引入多态内联缓存 (PIC) 消除方法查找开销

**目标**：解决 63.1% 的 `rt_call_method_raw_ptrs` 热点。通过类型版本守卫（Type guards），将高频的字典查找降维为仅凭机器级的 O(1) 指针验证与直接函数分配调用。

### 1. Runtime 层支持 (版本守卫与缓存槽)
- 在 `PyType` 中增加原子或普通的计数器 `uint64_t type_version{1}`，任意使得类字典或继承链改变的操作均使得 `version++`。
- 新增内联缓存数据结构，专门存放在 AOT 中的 Global 区域供快速访问：
  ```cpp
  struct MethodCallCache {
      py::PyType* expected_type; // 期望的对象类
      uint64_t type_version;     // 期望的类所处版本
      void* fast_func_ptr;       // 提取出的目标底层 C 函数指针
  };
  ```

### 2. AOT 编译层 (`IREmitter` & `Codegen`) 重写
- 针对每一次方法调用 `self.step1(...)`，不再简单生成一个统一的 `rt_call_method`。而是于 LLVM Module 级别为该处调用的 AST 节点分配一个独立的全局 `MethodCallCache`。
- 生成的 IR 逻辑如下（相当于给方法调用穿上一层 Type Guard）：
  ```llvm
  ; 快速路径: 对象类型和形貌版本必须 100% 齐平
  %valid1 = icmp eq %obj.type, %cache.expected_type
  %valid2 = icmp eq %obj.type.version, %cache.type_version
  %valid_all = and i1 %valid1, %valid2
  br i1 %valid_all, label %fast_path, label %slow_path
  
  fast_path:
    ; 缓存命中：直接利用函数指针发出调用（0 次哈希，0 次成员查表）
    %fn = load %cache.fast_func_ptr
    call %fn(%obj, %x, %y)
    br label %merge_block
    
  slow_path:
    ; 缓存未命中：退回到标准的哈希检索，并在此次查找结束时把结果写入 cache
    call @rt_call_method_with_cache(...)
    br label %merge_block
  ```
**预期收益**：对于 Sieve 中的千万次循环，其方法分发开销将从“大段复杂逻辑 + Hash + C++ Variant”被削减至相当于两句 IF 判断的机器级极速开销。

---

## 核心改造二：根除 `std::variant<...>` 传参，纯 C ABI 签名

**目标**：铲除 `RtValue::flatten`、`__memset_avx2_unaligned_erms` (4.7%) 以及 `PyResult` 分配所累积出的另外极大一部分负担，这占到了总体时间线最前列的严重退化点。

### 1. 废弃庞大且昂贵的 Value 实体通信
目前 AOT 与 Runtime 之间的函数边界全靠重达数十字节的 `std::variant<...>` 来交换引数。它带来的是不合理的 `memset` 阵列清零以及深层次的装箱 `flatten / from_value`。
- **重构 ABI 签名**：AOT 生成的函数（乃至原生的运行时 `rt_XXX` C++ 函数），在传参时不应再容纳 `Value` 数组。对于动态分发统一回归至轻量级裸指针 `PyObject*`，或直接利用你的 `RtValue` (一个纯 `uint64_t`)。
- 例如将原生与 AOT 方法的签名体系降级改造：
  ```cpp
  // 弃用
  PyResult<PyObject*> call_raw(std::span<std::variant<...>> args, PyDict* kwargs);
  // 启用类似 CPython Vectorcall / 纯数组的指针 ABI
  py::PyObject* call_fast(py::PyObject* self, py::PyObject** args, size_t nargs);
  ```

### 2. AOT 生成层的局部变量降解
在 `PylangCodegen` 生成 `loop_y` 之类的本地代码时，对于不逃逸的循环变量 `x`, `y` 彻底保持其作为 `RtValue` 整型的底层存在。向本代码库其它独立的小编译模块方法投递引数时，不要发起无谓的 `box()` 或包装入 `variant` 的指令。确保在“安全界限内”，数值和对象都在裸操作。

---

## 核心改造三：隐藏类形状 (Shape) 与槽位属性 (Slots) 的字典替代优化

**目标**：针对诸如 `self.prime[...]` 和 `head.terminal = True` 等实例熟悉调用实施手术，告别通用型的 `PyDict<Value, Value>` 全局检索噩梦。

### 1. 消除实例对象自带 `__dict__` 的泛型结构
- Python 对象不再一出生即默认实例化分配并挂载一个 C++ 的 `unordered_map` （这就产生大量的分配和清零时间耗费在字典维护上）。
- 引入 Shapes（隐藏类或称 Map）：普通对象应当持有的是一个轻量的指针 `ObjectShape* shape` 以及伴随内存一同分配在其后的连续槽位数组 `PyObject* attributes[]`。`ObjectShape` 在后台管理例如 `"limit"` 属性映射在数组的 Index 0、`"prime"` 在 Index 1 的关系。

### 2. 编译端与运行期的协同缓存内联
- 完全复用【改造一】里的思路，给 `rt_getattr` 和 `rt_setattr` 添加属性提取内联缓存。
- 缓存里记下：这个 `shape` 的 `"prime"` 字段在 `attributes` 数组里的 `offset` 为 1。
- AOT 命中该对象的 `shape` 一致性效验后，生成一条直接访问对象内存指定偏移位置读写数值的 `GetElementPtr` 及 `Load/Store`，彻底让耗时归零。

---

## 总结：实施路线与步骤优先级强建议

不要试图到处同步改代码，依照下述三部曲推进可取得立竿见影的成果：

1. **绝对优先度 0（马上做）：方法内联缓存 (Method PIC)** 
   - 前往 `Codegen/IREmitter.cpp`，注入对 `call_site` 上下文生成及方法缓存的守卫判定分支；将老旧的方法分发转移到底层增加并维护的 `rt_call_method_with_cache`。这是保底能立刻大幅斩去 60% 无用功的最核心步骤。
2. **优先度 1：清理参数变体 (Variant Abstraction Elimination)**
   - 重塑整个 `rt_func.cpp` 与 `PyNativeFunction` 体系的传参协议。将所有的 `std::variant / span<py::Value>` 坚决剔除出高频热点通信签名，改投 `PyObject*`。使 `__memset` 和 `flatten` 开销化为乌有。
3. **优先度 2：对象插槽化 (Hidden Class/Shapes)**
   - 对于普通对象的类定义与其实例化逻辑，实施 `m_attributes[]` 代替对象自带哈希字典的全面改造，终结通用字符串访问的时间消耗。

依照此“由上层调用降频 -> 到中间通信解压 -> 到底层访问瘦身”的路线，该 AOT 编译器能够原汁原味维持甚至强化 Python 3.9 的动态语义功能，且将如筛法之类的数学密集型程序的总耗时**极具把握地压缩至 `1s` 甚至更低大关**。
