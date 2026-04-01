# Pylang AOT 编译器关键性能重构指南 (v4.0)

针对最新的架构要求和设计底线调整：**缓存逻辑绝不应污染前端 IR，必须收敛至 Runtime 的 Export 侧；严格使用现有导出系统 (`PYLANG_EXPORT_...`) 暴露接口；必须完美契合 Python 3.9 语义。**

基于此，我们重新梳理了目前的性能杀手以及彻底的重构落地方案：

---

## 优化一：多态内联缓存 (PIC) 的纯 Runtime 化与严格语义

在之前的设计中，试图让前端 IR 生成判断缓存是否命中的逻辑是错误的。**真正的缓存机制应当对前端完全透明。**前端只需要为每个 Call Site 分配一块静默内存，缓存机制的核心下沉到 `src/runtime/export/cache/` 目录中。

### 1. 前端的极简任务 (Codegen & IREmitter)
前端在编译 `self.step1(x, y)` 时，仅在 LLVM Module 当中生成一个空的数据结构（例如长度为 4 的 `i64` 数组，或者特定的 `CallSiteCache` 空指针），随后直接生成一条对 runtime 的调用（不再自带分支）：
```llvm
%cache_ptr = getelementptr ... ; 获取当前方法调用点的专属全局变量
call @rt_call_method_ic_ptrs(%cache_ptr, %self, "step1", %args, %argc)
```

### 2. Runtime 的纯粹实现 (src/runtime/export/cache/rt_method_cache.cpp)
依靠宏暴露函数，并且在这个专门处理 Cache 的层内部实现“检查-命中-更新”的完整逻辑：

```cpp
#include "../rt_common.hpp"

struct MethodCache {
    py::PyType* expected_type;
    uint64_t type_version;
    // 还需要防范实例字典 __dict__ 覆盖类方法的情况
    uint64_t dict_version; 
    py::PyObject* resolved_func; // 解析好的 Callable (如 function 或 bound method)
};

PYLANG_EXPORT_FUNC("call_method_ic_ptrs", "obj", "ptr,obj,str,ptr,i32")
py::PyObject *rt_call_method_ic_ptrs(MethodCache* cache, py::PyObject* self, const char* name, py::PyObject** args, int32_t argc) {
    auto self_type = self->type();
    
    // 【快路径】: 严格匹配 Python 3.9 语义的 Type Guard 检查
    // 1. 类型指针必须相同
    // 2. 类型的字典版本不能变（类及其基类的属性未被覆盖）
    // 3. 实例的 __dict__ 版本不能变（针对 setattr 覆盖方法）
    if (cache->expected_type == self_type && 
        self_type->version == cache->type_version &&
        self->dict_version == cache->dict_version) {
        
        // 命中，直接转入最快的原始调用
        return cache->resolved_func->call_fast(args, argc);
    }
    
    // 【慢路径】未命中缓存：走标准 lookup 流程，并更新 cache
    return rt_call_method_update_cache(cache, self, name, args, argc);
}
```

**对 Python 3.9 语义的严格保证**：
通过 `type_version` 和 `dict_version` 的双重验证，如果代码中发生了 Monkey Patch (修改了基类方法)、或者对象通过 `__dict__` 动态赋值隐藏了类方法，Guard 条件自然失败，转入慢路径并重新生成 Cache，完全不牺牲任何动态性。

---

## 优化二：追究 `std::variant<...>` 来源与参数层清理

### 1. `std::variant<...>` 的真正幕后黑手
`vtune` 报告中消耗大量时间的 `__memset` 以及 `from_value`，直接来自你的 **`src/runtime/Value.hpp`**：
```cpp
// Value.hpp 中的定义
using Value = std::variant<std::monostate, NotImplementedType, Ellipsis, bool, BigIntType, Number, String, Bytes, NameConstant, Tuple, PyObject*>;
```
这在解释器（Interpreter）模式下是良好的，它将诸如小数字、布尔值保存在栈上的庞大变体结构中进行计算。
但是**在 AOT 模式下它是巨大的毒瘤**。因为 AOT 把代码翻译为了 LLVM IR，调用运行时的时候，如果是调用 `rt_call_raw_ptrs`：它被迫在每次传递参数前，将纯指针打包为长达 32~40 字节的 `py::Value` 数组。
在这个过程中：分配 40 字节引发零初始化 (`memset`) -> 强制写入 Tag -> Runtime 层收到后又通过 `ValueEq`, `from_value` 和 `get<PyObject*>` 强行拆包找回指针。这是造成 91% 负担的元凶。

### 2. 清理方法：重构 Runtime-AOT 通信 ABI
既然你要追求极致性能，且你已开发了极高效率的免分配 8字节标记指针 **`RtValue`**，那么必须：
- **消灭跨界 Variant**：凡是 `rt_op.cpp`, `rt_class.cpp`, `rt_func.cpp` 暴露给 AOT 的 API，绝对禁止接收 `std::span<std::variant<...>>` 或 `std::vector<Value>`。
- **纯指针数组化**：改写 Native / 快路调用的签名：
  ```cpp
  // 修改 PyFunction.hpp 中的 call 原型
  virtual PyObject* call_fast(PyObject** args, size_t argc, PyObject* kwargs) = 0;
  ```
  在生成的 IR 中只创建纯净的 `PyObject*` 数组 (或者是强转型的 `RtValue` 基础类型，因为 `RtValue` 只占用一个 `long` 64位）。

---

## 优化三：对象插槽化 (Object Shapes/Hidden Classes)

### 1. 对 `PyDict` 引发的次生灾难进一步分析
在 `perf2.md` 与 VTune 中能看到大量查表的开销。这是因为普通类的实例自身属性（如 `self.prime` 和 `head.terminal`），以及传递字典参数的场景，底层调用的竟然是基于刚才剖析的那个超重变体作为键值对的 `tsl::ordered_map<Value, Value>`。
这个机制去存几个有限且恒定的属性，杀鸡用牛刀到了极点。

### 2. 形态与插槽的极致替换方案
为了彻底摆脱对象通过字典存储成员属性并忍受哈希算法带来的恶劣响应：
- **拆分出 Shape 结构**：Python 对象在内存中不拥有独立的 `__dict__`（不主动触发分配 `unordered_map`）。而是持有一个 `Shape*`（在 V8 叫 Map，在 CPython 3.11 叫 Keys 数组）的指针。
  - `Shape` 结构内部维持从字符串（如 `"prime"`）映射到整数的机制：`{"prime": 1, "limit": 0}`。
  - 这个 `Shape` 会在其类型内部共享或构建链式转换。
- **改用连续存储 Slots**：实例本身只存放一个指针数组 `PyObject* slots[N]`（或 `RtValue slots[N]`）。
- **结合优化一的 Cache 体系协同工作**：
  我们在 `src/runtime/export/cache/rt_attr_cache.cpp` 编写同理的导出方法：
  ```cpp
  PYLANG_EXPORT_ATTR("getattr_ic", "obj", "ptr,obj,str")
  py::PyObject *rt_getattr_ic(AttrCache* cache, py::PyObject* obj, const char* name) {
      // 命中它的形态，这意味着属性在 slots 数组中的位置（offset）也是确定的
      if (obj->shape() == cache->expected_shape) {
          return obj->slots[cache->slot_offset]; // O(1) 直接内存数组读取！
      }
      // 慢路径处理...
  }
  ```

这三把刀结合：彻底把方法决议缓存在 C++、将臃肿的 `std::variant<...>` 驱逐出 Runtime 边界传参体系、把耗费时间的属性字典替换为 Shape/Slots + Offset Cache。这是任何现代动态脚本语言在走向极速 JIT / AOT 的唯一必由之路，能轻松带来成百上千倍的速度抬升。