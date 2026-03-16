(base) lux@Lux:~/code/language/python-cpp/src/runtime$ tree
.
├── AruntimeDIR.md
├── AssertionError.cpp
├── AssertionError.hpp
├── AttributeError.cpp
├── AttributeError.hpp
├── BaseException.cpp
├── BaseException.hpp
├── ClassBuilder.cpp
├── ClassBuilder.hpp
├── CustomPyObject.cpp
├── CustomPyObject.hpp
├── Exception.cpp
├── Exception.hpp
├── ExceptionTransport.hpp
├── GeneratorInterface.cpp
├── GeneratorInterface.hpp
├── Import.cpp
├── Import.hpp
├── ImportError.cpp
├── ImportError.hpp
├── IndexError.cpp
├── IndexError.hpp
├── KeyError.cpp
├── KeyError.hpp
├── KeyVersionTracker.hpp
├── Lock.hpp
├── LookupError.cpp
├── LookupError.hpp
├── MemoryError.cpp
├── MemoryError.hpp
├── ModuleMangler.hpp
├── ModuleNotFoundError.cpp
├── ModuleNotFoundError.hpp
├── ModuleRegistry.cpp
├── ModuleRegistry.hpp
├── NameError.cpp
├── NameError.hpp
├── NotImplemented.cpp
├── NotImplemented.hpp
├── NotImplementedError.cpp
├── NotImplementedError.hpp
├── OSError.cpp
├── OSError.hpp
├── PyArgParser.hpp
├── PyAsyncGenerator.cpp
├── PyAsyncGenerator.hpp
├── PyBool.cpp
├── PyBool.hpp
├── PyBoundMethod.cpp
├── PyBoundMethod.hpp
├── PyBuiltInMethod.cpp
├── PyBuiltInMethod.hpp
├── PyByteArray.cpp
├── PyByteArray.hpp
├── PyBytes.cpp
├── PyBytes.hpp
├── PyCell.cpp
├── PyCell.hpp
├── PyClassMethod.cpp
├── PyClassMethod.hpp
├── PyClassMethodDescriptor.cpp
├── PyClassMethodDescriptor.hpp
├── PyCode.cpp
├── PyCode.hpp
├── PyComplex.cpp
├── PyComplex.hpp
├── PyCoroutine.cpp
├── PyCoroutine.hpp
├── PyDict.cpp
├── PyDict.hpp
├── PyDict_tests.cpp
├── PyEllipsis.cpp
├── PyEllipsis.hpp
├── PyEnumerate.cpp
├── PyEnumerate.hpp
├── PyFloat.cpp
├── PyFloat.hpp
├── PyFrame.cpp
├── PyFrame.hpp
├── PyFrozenSet.cpp
├── PyFrozenSet.hpp
├── PyFunction.cpp
├── PyFunction.hpp
├── PyGenerator.cpp
├── PyGenerator.hpp
├── PyGenericAlias.cpp
├── PyGenericAlias.hpp
├── PyGetSetDescriptor.cpp
├── PyGetSetDescriptor.hpp
├── PyInteger.cpp
├── PyInteger.hpp
├── PyInteger_refactor_tests.cpp
├── PyIterator.cpp
├── PyIterator.hpp
├── PyLLVMFunction.cpp
├── PyLLVMFunction.hpp
├── PyList.cpp
├── PyList.hpp
├── PyMap.cpp
├── PyMap.hpp
├── PyMappingProxy.cpp
├── PyMappingProxy.hpp
├── PyMemberDescriptor.cpp
├── PyMemberDescriptor.hpp
├── PyMemoryView.cpp
├── PyMemoryView.hpp
├── PyMethodDescriptor.cpp
├── PyMethodDescriptor.hpp
├── PyModule.cpp
├── PyModule.hpp
├── PyNamespace.cpp
├── PyNamespace.hpp
├── PyNone.cpp
├── PyNone.hpp
├── PyNumber.cpp
├── PyNumber.hpp
├── PyNumber_tests.cpp
├── PyObject.cpp
├── PyObject.hpp
├── PyProperty.cpp
├── PyProperty.hpp
├── PyRange.cpp
├── PyRange.hpp
├── PyReversed.cpp
├── PyReversed.hpp
├── PySet.cpp
├── PySet.hpp
├── PySlice.cpp
├── PySlice.hpp
├── PySlotWrapper.cpp
├── PySlotWrapper.hpp
├── PyStaticMethod.cpp
├── PyStaticMethod.hpp
├── PyString.cpp
├── PyString.hpp
├── PyString_tests.cpp
├── PySuper.cpp
├── PySuper.hpp
├── PyTraceback.cpp
├── PyTraceback.hpp
├── PyTuple.cpp
├── PyTuple.hpp
├── PyType.cpp
├── PyType.hpp
├── PyType_tests.cpp
├── PyZip.cpp
├── PyZip.hpp
├── RuntimeContext.cpp
├── RuntimeContext.hpp
├── RuntimeError.cpp
├── RuntimeError.hpp
├── StopIteration.cpp
├── StopIteration.hpp
├── SyntaxError.cpp
├── SyntaxError.hpp
├── TypeError.cpp
├── TypeError.hpp
├── UnboundLocalError.cpp
├── UnboundLocalError.hpp
├── Value.cpp
├── Value.hpp
├── ValueError.cpp
├── ValueError.hpp
├── builtinTypeInit.cpp
├── builtinTypeInit.hpp
├── compat.hpp
├── compat_tests.cpp
├── concepts.hpp
├── export
│   ├── CMakeLists.txt
│   ├── export.hpp
│   ├── rt_attr.cpp
│   ├── rt_class.cpp
│   ├── rt_cmp.cpp
│   ├── rt_common.hpp
│   ├── rt_convert.cpp
│   ├── rt_create.cpp
│   ├── rt_error.cpp
│   ├── rt_func.cpp
│   ├── rt_interpreter_stubs.cpp
│   ├── rt_lifecycle.cpp
│   ├── rt_module.cpp
│   ├── rt_op.cpp
│   ├── rt_singleton.cpp
│   ├── rt_subscr.cpp
│   └── rt_tuple.cpp
├── forward.hpp
├── frozen
│   ├── importlib.h
│   └── importlib_external.h
├── modules
│   ├── BuiltinsModule.cpp
│   ├── CodecsModule.cpp
│   ├── IOModule.cpp
│   ├── ImpModule.cpp
│   ├── MarshalModule.cpp
│   ├── Modules.hpp
│   ├── PosixModule.cpp
│   ├── README.md
│   ├── SysModule.cpp
│   ├── WarningsModule.cpp
│   ├── collections
│   │   ├── DefaultDict.cpp
│   │   ├── DefaultDict.hpp
│   │   ├── Deque.cpp
│   │   ├── Deque.hpp
│   │   └── module.cpp
│   ├── config.hpp
│   ├── errno
│   │   └── module.cpp
│   ├── itertools
│   │   ├── Chain.cpp
│   │   ├── Chain.hpp
│   │   ├── Count.cpp
│   │   ├── Count.hpp
│   │   ├── ISlice.cpp
│   │   ├── ISlice.hpp
│   │   ├── Permutations.cpp
│   │   ├── Permutations.hpp
│   │   ├── Product.cpp
│   │   ├── Product.hpp
│   │   ├── Repeat.cpp
│   │   ├── Repeat.hpp
│   │   ├── StarMap.cpp
│   │   ├── StarMap.hpp
│   │   └── module.cpp
│   ├── math
│   │   └── module.cpp
│   ├── paths.hpp.in
│   ├── signal
│   │   └── module.cpp
│   ├── sre
│   │   ├── Match.cpp
│   │   ├── Match.hpp
│   │   ├── Pattern.cpp
│   │   ├── Pattern.hpp
│   │   └── module.cpp
│   ├── struct
│   │   └── module.cpp
│   ├── thread
│   │   ├── Lock.hpp
│   │   ├── RLock.hpp
│   │   └── module.cpp
│   ├── time
│   │   └── module.cpp
│   └── weakref
│       ├── PyCallableProxyType.cpp
│       ├── PyCallableProxyType.hpp
│       ├── PyWeakProxy.cpp
│       ├── PyWeakProxy.hpp
│       ├── PyWeakRef.cpp
│       ├── PyWeakRef.hpp
│       ├── WeakRefRegistry.hpp
│       └── module.cpp
├── tests
│   ├── ArenaObject_tests.cpp
│   ├── CompileIsolation_tests.cpp
│   ├── Import_tests.cpp
│   ├── InterpreterCore_tests.cpp
│   ├── KeyVersionTracker_tests.cpp
│   ├── ModuleInit_tests.cpp
│   ├── ModuleMangler_tests.cpp
│   ├── ModuleRegistry_tests.cpp
│   ├── ProviderChain_tests.cpp
│   ├── PyModule_add_symbol_tests.cpp
│   ├── PyModule_version_tests.cpp
│   ├── RuntimeAPI_tests.cpp
│   ├── RuntimeContext_tests.cpp
│   ├── RuntimeContext_truthy_tests.cpp
│   ├── Truthy_fallback_tests.cpp
│   └── Value_truthy_tests.cpp
├── types
│   ├── api.hpp
│   ├── builtin.cpp
│   └── builtin.hpp
├── utilities.hpp
└── warnings
    ├── DeprecationWarning.cpp
    ├── DeprecationWarning.hpp
    ├── ImportWarning.cpp
    ├── ImportWarning.hpp
    ├── PendingDeprecationWarning.cpp
    ├── PendingDeprecationWarning.hpp
    ├── ResourceWarning.cpp
    ├── ResourceWarning.hpp
    ├── Warning.cpp
    └── Warning.hpp

17 directories, 271 files