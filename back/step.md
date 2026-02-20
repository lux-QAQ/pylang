# Pylang 重构详细路线图 (Step-by-Step Roadmap)

本文档详细拆解了将 `python-cpp` 重构为 `Pylang` 编译器的具体步骤。
**核心原则：每一步都必须通过测试验证。在当前步骤的测试通过之前，严禁进入下一步。**

## 第一阶段：基础设施与环境准备 (Phase 1: Infrastructure)

此阶段目标是建立 LLVM 开发环境，并确保现有的非 VM 依赖代码（如 Parser）是稳健的。


## 第三阶段：编译器前端实现 (Phase 3: Compiler Frontend)

此阶段目标：实现 `src/compiler`，将 AST 转换为 LLVM IR，并链接 Runtime。

### 步骤 3.1: 编译器骨架 (Skeleton)
- [ ] **行动 (Action)**: 创建 `src/compiler/Main.cpp` 和 `Compiler.hpp`。
- [ ] **行动 (Action)**: 初始化 `llvm::LLVMContext`, `llvm::Module`, `llvm::IRBuilder`。
- [ ] **行动 (Action)**: 移植 `src/executable/bytecode/codegen/VariablesResolver.cpp` 到 `src/compiler/VariablesResolver.cpp`。这是为了复用现有的作用域分析逻辑（LEGB规则）。
- [ ] **验证 (Verification)**: 编译并运行编译器，输出一个空的 `.ll` 文件。

### 3.2: Runtime 链接器 (Runtime Linker)
这是连接 Runtime 和编译器的桥梁。
- [ ] **行动 (Action)**: 实现 `RuntimeLinker` 类。
    -   加载 `runtime.bc` 文件。
    -   遍历 Module 中的所有 Function。
    -   建立 `std::map<std::string, llvm::Function*>` 符号表，仅包含带有 `pylang_runtime_export` 属性的函数。
- [ ] **验证 (Verification)**: 编译器启动时打印出所有检测到的 Runtime 导出函数名（如 `py_add`, `PyInteger_create`）。

### 步骤 3.3: CodeGen - 移植核心逻辑
**策略**: 不要从头重写 AST 访问器。参考 `src/executable/bytecode/codegen/BytecodeGenerator.cpp` 的逻辑，将其移植到 `LLVMGenerator`。
- [ ] **行动 (Action)**: 移植 `visit(Constant*)`。参考 `BytecodeGenerator::load_const`，改为生成 `PyInteger_create` 等 Runtime 调用。
- [ ] **行动 (Action)**: 实现 `ASTVisitor` 的 `visit(Constant*)`。
    -   对于整数：生成调用 Runtime 函数 `PyInteger::create(int64_t)` 的 IR 指令。
- [ ] **行动 (Action)**: 创建简单的测试用例 `test_int.py` (内容: `42`)。
- [ ] **验证 (Verification)**: 
    1. 编译器生成 IR。
    2. 生成的 IR 链接 `runtime.bc`。
    3. 使用 `lli` (LLVM Interpreter) 或编译为可执行文件运行，不崩溃。

### 步骤 3.4: CodeGen - 移植控制流与运算
**策略**: 借鉴 `BytecodeGenerator` 的语义处理逻辑，直接在 AST Visitor 中生成等价的 LLVM IR。
- [ ] **行动 (Action)**: 在 Runtime 中实现 `builtin_print` 并导出。
- [ ] **行动 (Action)**: 实现 `Call` 节点的 CodeGen。
- [ ] **验证 (Verification)**: 编译并运行 `print(123)`，终端输出 `123`。

### 3.5: CodeGen - 移植循环与异常
- [ ] **行动 (Action)**: 实现 `BinaryExpr` 的 CodeGen。
- [ ] **行动 (Action)**: 查找 Runtime 中的 `py_add` (或类似分发函数) 并生成调用。
- [ ] **行动 (Action)**: 移植 `For` 循环逻辑。参考 `ForIter.cpp` 指令，将其中的 `StopIteration` 异常捕获逻辑转换为 LLVM 的 BasicBlock 跳转 (invoke/landingpad)。
- [ ] **验证 (Verification)**: 编译并运行 `print(1 + 2)`，输出 `3`。

---

## 第四阶段：集成与验证 (Phase 4: Integration)

### 步骤 4.1: 端到端测试套件修复
- [ ] **行动 (Action)**: 重写 `run_integration_tests.sh`。
    -   旧流程：`./python-cpp input.py`
    -   新流程：`./pylangc input.py -o output && ./output`
- [ ] **验证 (Verification)**: 运行 `integration/fibonacci/fibo.py`，确保结果正确。

### 步骤 4.2: 性能基准测试
- [ ] **行动 (Action)**: 对比 Pylang 编译版 vs CPython 解释版的斐波那契数列计算时间。
- [ ] **行动 (Action)**: 验证是否存在内存泄漏 (使用 Valgrind 或 ASAN)。

---

## 第五阶段：高级特性补全 (Phase 5: Advanced Features)

- [ ] 实现控制流 (`if`, `while`, `for`) 的 IR 生成。
- [ ] 实现函数定义 (`def`) 的 IR 生成（涉及闭包和栈帧结构，较为复杂）。
- [ ] 实现类定义 (`class`)。
