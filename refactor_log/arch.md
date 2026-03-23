# Architecture
# Architecture Documentation

This document provides a high-level overview of the `python-cpp` project architecture.

## Overview

`python-cpp` is an experimental Python 3.10+ compatible interpreter written in C++20. It implements a custom runtime with a hybrid object model, a register-based virtual machine, and a mark-and-sweep garbage collector.

The project is structured into three main phases:
1.  **Parsing**: Source code -> AST (`src/parser`, `src/ast`)
2.  **Compilation**: AST -> Bytecode (`src/executable/bytecode`)
3.  **Execution**: Bytecode execution by the VM (`src/vm`, `src/interpreter`)

## Core Components

### 1. Object Model & Type System (`src/runtime`)

The runtime uses a **hybrid object model** to balance performance and Python compatibility.

*   **`py::Value` (`src/runtime/Value.hpp`)**:
  *   A tagged union (conceptually similar to a variant) that stores small, immutable values directly on the stack or in VM registers.
  *   **Types supported**: `double`, `int64_t` (via GMP `mpz_class` for big ints), `bool`, `std::string`, `NoneType`.
  *   This avoids heap allocation for common primitives.

*   **`py::PyObject` (`src/runtime/PyObject.hpp`)**:
  *   The base class for all heap-allocated Python objects (Lists, Dicts, Class Instances, Functions).
  *   Inherits from `Cell` to participate in Garbage Collection.
  *   Implements the Python protocol methods (e.g., `__add__`, `__repr__`, `__getattribute__`).

*   **Built-in Types**:
  *   Implemented in `src/runtime` (e.g., `PyList`, `PyDict`, `PyString`, `PyFunction`).
  *   Types are represented by instances of `PyType` (which is itself a `PyObject`).
  *   The hierarchy mimics CPython: `object` -> `type`, `int`, `str`, etc.

### 2. Memory Management (`src/memory`)

*   **Garbage Collection**: A custom **Mark-and-Sweep** GC.
*   **Heap**: All `PyObject`s are allocated via the `Heap`.
*   **Tracing**: Objects must implement `visit_graph(Visitor &)` to expose their references to the GC.
*   `Value`s stored in registers act as GC roots.

### 3. Parser & AST (`src/parser`, `src/ast`)

*   **Parser**: A recursive descent parser that produces an Abstract Syntax Tree (AST).
*   **AST Nodes** (`src/ast/AST.hpp`):
  *   Hierarchy rooted at `ASTNode`.
  *   Includes nodes for Control Flow (`If`, `While`), Data Structures (`List`, `Dict`), and Definitions (`FunctionDefinition`).
  *   **`Constant` Node**: Wraps a `py::Value`, bridging the AST and Runtime directly.
*   **Code Generation**:
  *   Uses the Visitor pattern (`CodeGenerator` interface).
  *   AST nodes dispatch `codegen()` calls to the generator.

### 4. Virtual Machine (`src/vm`)

*   **Architecture**: **Register-based VM**.
*   **Registers**: A standard `std::vector<py::Value>` represents the register file for the current stack frame.
*   **Stack Frame (`StackFrame`)**:
  *   Contains its own set of registers.
  *   Manages local variables and return addresses.
*   **Execution**:
  *   The VM executes `BytecodeProgram`s.
  *   Instructions operate on registers rather than a value stack (unlike CPython).

### 5. Build System

*   **CMake**: The project uses CMake with `CPM` for dependency management.
*   **Dependencies**:
  *   **LLVM / MLIR**: Used for the backend code generation infrastructure.
  *   **GMP**: For arbitrary-precision integers (`mpz_class`).
  *   **fmt**: For string formatting.
  *   **ICU**: For Unicode support.

## Relationship to Standard Python

*   **Compatibility**: Aims for Python 3.10 syntax and semantics.
*   **Differences**:
  *   Register-based VM vs Stack-based VM (CPython).
  *   Direct support for `int64` and `double` primitives in the runtime `Value` type.
## Goals and scope
- C++ implementation of a Python interpreter targeting Python 3.10 syntax compatibility.
- Register-based virtual machine (VM) rather than CPython's stack-based VM.
- Runtime implemented as C++ classes for Python objects to ease native integration.
- Optional MLIR/LLVM backend and future JIT support (currently marked as TODO).

## High-level pipeline
1. Source text is tokenized by the lexer (CPython-compatible tokens).
2. The parser builds a CPython-like AST.
3. Frontends/lowerings:
   - Bytecode generation for the register-based VM.
   - MLIR/LLVM pipeline (optional backend).
4. The VM executes bytecode using runtime objects and the memory subsystem.

## Parser architecture
The parser is a PEG-style packrat parser implemented with C++ template-based
parser combinators. This keeps grammar rules concise and composable.

Key ideas:
- Parser combinators such as sequence, choice, optional, and repetition build the
  grammar from reusable templates.
- Packrat caching memoizes rule results per token position to make backtracking
  linear-time.
- Direct left-recursion is supported by a grow-left-recursion strategy.
- Grammar is modeled on CPython's official grammar; AST nodes mirror CPython.

## Parser-supported Python syntax
The following features are supported by the parser (based on grammar and tests):

### Literals and basic expressions
- Numeric literals (decimal, hex, octal, binary) and float literals.
- Strings, raw strings, bytes literals, and f-strings (with replacement fields).
- True/False, None, and Ellipsis.
- Unary ops: +, -, ~, not.
- Binary ops: +, -, *, /, //, %, **, @, <<, >>, &, |, ^.
- Comparisons: ==, !=, <, <=, >, >=, is, is not, in, not in (supports chaining).
- Boolean ops: and, or.
- Conditional expression: x if cond else y.
- Walrus operator: :=.

### Containers and subscripting
- List, tuple, dict, and set literals.
- List/dict/set comprehensions and generator expressions.
- Indexing and slicing, including extended slices.
- Attribute access and method calls.

### Calls and parameters
- Function calls with positional args, keyword args, *args, **kwargs.
- Function definitions with:
  - Type annotations for args/returns.
  - Positional-only args (/), keyword-only args (*).
  - Defaults, *args, **kwargs.
- Lambda expressions.

### Statements and control flow
- Assignment (including destructuring) and augmented assignment.
- if/elif/else, for/else, while/else.
- break, continue, pass, return.
- with and async with (parser-level support).
- try/except/else/finally, raise, assert.
- del, global, nonlocal.
- import and from-import (including dotted names and relative levels).
- async def, await, yield, yield from.

## Runtime architecture
The runtime models Python objects as C++ classes and provides the object model,
exceptions, and builtin types used by the VM and interpreter.

### Object model and execution
- Base object system: PyObject, PyType, and runtime descriptors.
- Function and execution frames: PyFunction, PyCode, PyFrame.
- Method binding: PyBoundMethod, PyBuiltInMethod, descriptor types.

### Builtin types (representative)
- Numbers: PyInteger, PyFloat, PyComplex, PyBool, PyNumber.
- Text and bytes: PyString, PyBytes, PyByteArray.
- Containers: PyList, PyTuple, PyDict, PySet, PyFrozenSet, PyMappingProxy.
- Other core types: PyNone, PyEllipsis, PySlice, PyRange, PyMemoryView.
- Utilities: PyGenericAlias, PyNamespace.

### Iteration and generators
- Iteration protocol: PyIterator.
- Generators and coroutines: PyGenerator, PyCoroutine, PyAsyncGenerator.
- Builtins for iteration: PyEnumerate, PyMap, PyZip, PyReversed.

### Exceptions
- BaseException and common subclasses: AssertionError, AttributeError,
  ImportError, IndexError, KeyError, LookupError, MemoryError,
  ModuleNotFoundError, NameError, NotImplementedError, OSError,
  RuntimeError, StopIteration, SyntaxError, TypeError, UnboundLocalError,
  ValueError, and others.

### Module system
- PyModule and import helpers in the runtime.
- Builtin modules include: builtins, codecs, io, imp, marshal, posix, sys,
  warnings, collections, errno, itertools, math, signal, sre, struct,
  thread, time, weakref.

### Memory management
- A custom heap and garbage collector are implemented in the memory subsystem.

## VM and backends
- Register-based VM executes bytecode programs and interacts with runtime objects.
- MLIR/LLVM backend exists for alternative lowering and execution paths.

## Known gaps and TODOs
- Some grammar TODOs exist (commit/"~" patterns, type comments).
- Async comprehensions appear partially specified but not fully implemented.
- JIT support is noted as future work.
- Pattern matching (match/case) is not present in the current parser.

## Testing
- Unit tests exist for lexer, parser, bytecode, runtime types, and other core
  components.
