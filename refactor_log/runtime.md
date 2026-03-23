# 运行时架构 (Runtime Architecture)

本项目实现了一个兼容 Python 3.10 语义的运行时环境。核心设计采用了混合对象模型：一是用于虚拟机寄存器的高效 C++ 值类型 (`py::Value`)，二是用于动态 Python 对象的传统堆分配对象系统。

## 1. 对象模型 (Object Model)

对象系统的核心围绕两个主要类型构建：`py::PyObject`（堆对象基类）和 `py::Value`（作为 Tagged Union 的值类型）。

### 1.1 `PyObject` (堆对象)
`PyObject` 是所有堆上分配的 Python 对象的根基类。
*   **继承体系**: `PyObject` 继承自 `Cell` 类（用于垃圾回收）。
*   **类型指针 (`ob_type`)**: 每个对象都持有一个指向其类型对象 (`PyType`) 的指针（或引用），这定义了对象的行为特征。
*   **属性字典 (`__dict__`)**: 支持动态属性的对象包含一个 `PyDict*` 指针。
*   **`PyBaseObject`**: 这是一个具体的 C++ 基类，大多数内置类型（如 `PyList`, `PyTuple`, `PyFunction`）都继承自它，以便复用通用的对象行为。

### 1.2 `Value` 类型 (立即数优化)
为了减少小对象（如整数、浮点数、短字符串）的堆分配开销，虚拟机使用了 `py::Value` 类型。这是一个基于 `std::variant` 的 Tagged Union。
*   **支持的类型**:
    *   `Number`：内部封装了 `double` 或 `BigIntType` (MPIR/GMP)，支持任意精度整数和浮点数。
    *   `String` / `Bytes`：直接存储字符串数据。
    *   `Tuple`：`Value` 的向量，用于轻量级元组。
    *   `NoneType`, `Ellipsis`, `bool`。
    *   `PyObject*`：指向堆上完整对象的指针。
*   **寄存器存储**: VM 的寄存器 (`std::vector<py::Value>`) 直接存储这些值，使得基础数学运算可以直接在 CPU 寄存器或栈上完成，无需间接寻址。

## 2. 类与继承的实现 (Classes and Inheritance)

### 2.1 类型系统 (`PyType`)
Python 中的“类”在运行时由 `PyType` 对象表示。`PyType` 本身也是一个 `PyObject`（元类为 `type`）。
*   **`TypePrototype`**: 这是一个底层 C++ 结构，类似于虚函数表 (vtable)。它存储了该类型的所有 **Slot (槽)** 函数（如 `__new__`, `__init__`, `__add__` 等）以及成员定义。`PyType` 负责管理和初始化这个结构。
*   **API 定义**: 在 C++ 代码中，通过 `klass<T>` 模板（位于 `types/api.hpp`）以声明式的方式定义 Python 类型及其方法绑定到 C++ 成员函数。

### 2.2 继承与方法解析顺序 (MRO)
运行时完整实现了 Python 的多重继承机制。
*   **MRO 计算**: 在类型创建时（`PyType::initialize`），运行时会计算 **MRO (Method Resolution Order)**。
*   **C3 线性化**: 使用 C3 线性化算法（`merge` 函数）来合并基类的 MRO 列表，确保在复杂的多重继承图（如菱形继承）中保持单调性并正确处理优先级。
*   **存储**: 计算出的 MRO 存储在类型的 `__mro__` 属性中（一个 `PyTuple`）。

### 2.3 属性与方法查找
属性访问 (`globals`, `locals`, `getattr`) 遵循标准的 Python 描述符协议：
1.  **Data Descriptor (数据描述符)**: 首先在类型及其 MRO 中查找是否有名为该属性的数据描述符（如 `property`）。如果是，则调用其 `__get__`。
2.  **Instance Dictionary (实例字典)**: 查找对象自身的 `__dict__`。
3.  **Non-Data Descriptor (非数据描述符)**: 查找类型及其 MRO 中的非数据描述符（如普通方法 `PyFunction`）。如果是，调用其 `__get__` 进行绑定（生成 `PyBoundMethod`）。
4.  **Class Attribute (类属性)**: 查找类型及其 MRO 中的普通值。
5.  **`__getattr__`**: 如果上述步骤失败，则尝试调用 `__getattr__`。

## 3. 内存管理 (Memory Management)

内存管理由 **Heap (堆)** 和 **Garbage Collector (垃圾回收器)** 子系统实现，采用自动内存管理。

### 3.1 堆分配 (`Heap`)
*   **统一接口**: 所有 Python 对象 (`PyObject` 及其子类) 必须通过 `VirtualMachine::the().heap().allocate<T>(...)` 创建。
*   **对象生命周期**: 对象在 C++ 堆上分配，但其生命周期不由 C++ 的作用域（RAII）控制，而是由 GC 控制。

### 3.2 垃圾回收 (`GarbageCollector`)
项目采用 **Tracing GC (追踪式垃圾回收)**，具体为 **Mark-and-Sweep (标记-清除)** 算法，能够处理循环引用问题。
*   **`Cell` 基类**: `PyObject` 继承自 `Cell`。`Cell` 包含 GC 头部信息（如标记颜色/状态位）。
*   **Visit Graph (图遍历)**: 每个 `PyObject` 子类都必须实现虚方法 `visit_graph(Visitor &visitor)`。
    *   例如，`PyList` 的 `visit_graph` 会遍历其包含的所有元素。
    *   `PyFunction` 会遍历其闭包 (`closure`) 和全局变量引用。
*   **GC 过程**:
    1.  **Roots (根集合)**: GC 从 VM 的调用栈（寄存器中的 `Value`）、全局解释器状态、内置模块等根对象开始。
    2.  **Mark (标记)**: 递归调用 `visit_graph` 标记所有可达对象。
    3.  **Sweep (清除)**: 遍历堆中的所有对象，释放未被标记的对象内存。

## 4. 运算处理 (Operations Handling)

运算（如加减乘除、位运算、比较）采用了双层分发机制，以平衡性能与动态性。

### 4.1 快速路径 (`py::Value` 层)
在 `src/runtime/Value.cpp` 中定义了一系列运算函数（如 `py::add`, `py::less_than`）。
*   **立即数优化**: 当两个操作数都是 `Number` 类型的 `Value` 时，VM 直接执行 C++ 层的数学运算。
    *   **大数支持**: `Number` 类型自动处理 C++ `double` 和 GMP `BigIntType` 之间的转换。如果整数溢出或需要高精度，会自动使用大数运算。
*   这避免了创建完整的 `PyInteger` 或 `PyFloat` 堆对象的开销。

### 4.2 通用路径 (`PyObject` 层)
如果操作数包含 `PyObject*`（或者是 `Tuple` 等复杂结构），运算会回退到 Python 的魔术方法（Magic Methods）机制：
1.  **左操作数分发**: 检查左操作数类型的 `TypePrototype` 是否定义了对应的 Slot（如 `__add__`）。
2.  **右操作数分发 (反射)**: 如果左侧不支持（返回 `NotImplemented`），则检查右操作数的反射 Slot（如 `__radd__`）。
3.  **Slot Wrapper**: 对于在 Python 中定义的方法，运行时使用 `PySlotWrapper` 将 C++ 的 Slot调用转换为 Python 函数调用。

### 4.3 虚拟机指令
VM 的指令集（如 `BINARY_ADD`）被设计为调用 `py::add(Value, Value)`。这使得 VM 指令本身不需要关心操作数是立即数还是堆对象，由 `Value` 层自动路由到最高效的实现路径。

