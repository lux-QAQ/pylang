#### 2. AOT 栈帧缺失引发的潜在问题分析

你提到的关键点是：**AOT 编译后的程序在运行时没有解释器栈帧 (`PyFrame`)**。

目前 `PylangCodegen` 生成的是纯 LLVM IR，函数调用使用的是机器栈（Machine Stack）。Runtime 中的许多功能如果依赖 `VirtualMachine::the().interpreter()->execution_frame()`，在 AOT 模式下都会失败。

以下是 Python 中依赖栈帧的主要特性及其在当前 Codegen 中的风险分析：

| 特性 | 依赖机制 | AOT 现状 | 风险等级 | 解决方案 |
| :--- | :--- | :--- | :--- | :--- |
| **`super()` (0 args)** | 检查栈帧的 locals (`self`) 和 freevars (`__class__`) | **已崩溃 (Fixed by rewrite)** | 🔴 高 | 编译期重写为 `super(__class__, self)` (已实施) |
| **`locals()`** | 动态收集当前栈帧的局部变量 | **不支持** | 🔴 高 | 编译器需要显式生成字典构建代码，或者 Runtime 需要特殊支持 (Shadow Stack) |
| **`globals()`** | 获取当前模块的全局字典 | 安全 | 🟢 低 | `PylangCodegen` 始终持有 `module` 对象，可以传递给 Runtime |
| **`vars()` (无参)** | 等同于 `locals()` | **不支持** | 🔴 高 | 同 `locals()` |
| **`dir()` (无参)** | 等同于 `locals().keys()` | **不支持** | 🔴 高 | 同 `locals()` |
| **`eval()` / `exec()`** (暂时不实现) | 默认使用当前 `globals` 和 `locals` | **不完全** | 🟠 中 | 如果不传参数，无法获取当前上下文 |
| **Exceptions (Traceback)** | 异常抛出时记录栈回溯 | **缺失** | 🟠 中 | C++ 异常解卷很快，但不会自动生成 Python Traceback 对象。需要 landingpad 手动构建 Frame info |
| **`sys._getframe()`** | 直接检查栈帧 | **不支持** | ⚪ 忽略 | 通常用于调试/黑魔法，AOT 环境可暂不支持 |
| **`import` (relative)** | 需要知道当前 `__package__` | 安全 | 🟢 低 | `__package__` 是模块级全局变量，AOT 模块初始化时已设置 |
