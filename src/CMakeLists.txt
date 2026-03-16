set(AST_SOURCE_FILES # cmake-format: sortable
  ast/AST.cpp ast/optimizers/ConstantFolding.cpp)

set(BYTECODE_SOURCE_FILES
  # cmake-format: sortable
  executable/bytecode/Bytecode.cpp
  executable/bytecode/BytecodeProgram.cpp
  executable/bytecode/codegen/BytecodeGenerator.cpp
  executable/bytecode/codegen/VariablesResolver.cpp
  executable/bytecode/instructions/BinaryOperation.cpp
  executable/bytecode/instructions/BinarySubscript.cpp
  executable/bytecode/instructions/BuildDict.cpp
  executable/bytecode/instructions/BuildList.cpp
  executable/bytecode/instructions/BuildTuple.cpp
  executable/bytecode/instructions/BuildSet.cpp
  executable/bytecode/instructions/BuildSlice.cpp
  executable/bytecode/instructions/BuildString.cpp
  executable/bytecode/instructions/CompareOperation.cpp
  executable/bytecode/instructions/ClearExceptionState.cpp
  executable/bytecode/instructions/ClearTopCleanup.cpp
  executable/bytecode/instructions/DictAdd.cpp
  executable/bytecode/instructions/DictMerge.cpp
  executable/bytecode/instructions/DictUpdate.cpp
  executable/bytecode/instructions/DeleteAttr.cpp
  executable/bytecode/instructions/DeleteDeref.cpp
  executable/bytecode/instructions/DeleteFast.cpp
  executable/bytecode/instructions/DeleteGlobal.cpp
  executable/bytecode/instructions/DeleteName.cpp
  executable/bytecode/instructions/DeleteSubscript.cpp
  executable/bytecode/instructions/ForIter.cpp
  executable/bytecode/instructions/FormatValue.cpp
  executable/bytecode/instructions/FunctionCall.cpp
  executable/bytecode/instructions/FunctionCallEx.cpp
  executable/bytecode/instructions/FunctionCallWithKeywords.cpp
  executable/bytecode/instructions/GetAwaitable.cpp
  executable/bytecode/instructions/GetIter.cpp
  executable/bytecode/instructions/GetYieldFromIter.cpp
  executable/bytecode/instructions/InplaceOp.cpp
  executable/bytecode/instructions/Instructions.cpp
  executable/bytecode/instructions/ImportFrom.cpp
  executable/bytecode/instructions/ImportName.cpp
  executable/bytecode/instructions/ImportStar.cpp
  executable/bytecode/instructions/Jump.cpp
  executable/bytecode/instructions/JumpForward.cpp
  executable/bytecode/instructions/JumpIfFalse.cpp
  executable/bytecode/instructions/JumpIfFalseOrPop.cpp
  executable/bytecode/instructions/JumpIfExceptionMatch.cpp
  executable/bytecode/instructions/JumpIfNotExceptionMatch.cpp
  executable/bytecode/instructions/JumpIfTrue.cpp
  executable/bytecode/instructions/JumpIfTrueOrPop.cpp
  executable/bytecode/instructions/LeaveExceptionHandling.cpp
  executable/bytecode/instructions/ListAppend.cpp
  executable/bytecode/instructions/ListExtend.cpp
  executable/bytecode/instructions/ListToTuple.cpp
  executable/bytecode/instructions/LoadAssertionError.cpp
  executable/bytecode/instructions/LoadAttr.cpp
  executable/bytecode/instructions/LoadBuildClass.cpp
  executable/bytecode/instructions/LoadClosure.cpp
  executable/bytecode/instructions/LoadConst.cpp
  executable/bytecode/instructions/LoadDeref.cpp
  executable/bytecode/instructions/LoadFast.cpp
  executable/bytecode/instructions/LoadGlobal.cpp
  executable/bytecode/instructions/LoadMethod.cpp
  executable/bytecode/instructions/LoadName.cpp
  executable/bytecode/instructions/MakeFunction.cpp
  executable/bytecode/instructions/MethodCall.cpp
  executable/bytecode/instructions/Move.cpp
  executable/bytecode/instructions/Pop.cpp
  executable/bytecode/instructions/Push.cpp
  executable/bytecode/instructions/RaiseVarargs.cpp
  executable/bytecode/instructions/ReRaise.cpp
  executable/bytecode/instructions/ReturnValue.cpp
  executable/bytecode/instructions/SetAdd.cpp
  executable/bytecode/instructions/SetUpdate.cpp
  executable/bytecode/instructions/SetupExceptionHandling.cpp
  executable/bytecode/instructions/SetupWith.cpp
  executable/bytecode/instructions/StoreAttr.cpp
  executable/bytecode/instructions/StoreDeref.cpp
  executable/bytecode/instructions/StoreFast.cpp
  executable/bytecode/instructions/StoreGlobal.cpp
  executable/bytecode/instructions/StoreName.cpp
  executable/bytecode/instructions/StoreSubscript.cpp
  executable/bytecode/instructions/ToBool.cpp
  executable/bytecode/instructions/Unary.cpp
  executable/bytecode/instructions/UnpackExpand.cpp
  executable/bytecode/instructions/UnpackSequence.cpp
  executable/bytecode/instructions/WithExceptStart.cpp
  executable/bytecode/instructions/YieldFrom.cpp
  executable/bytecode/instructions/YieldLoad.cpp
  executable/bytecode/instructions/YieldValue.cpp
  executable/Mangler.cpp)

set(EXECUTABLE_SOURCE_FILES # cmake-format: sortable
  executable/Program.cpp)

set(LEXER_SOURCE_FILES # cmake-format: sortable
  lexer/Lexer.cpp)

set(LLVM_BACKEND_FILES # cmake-format: sortable
  executable/llvm/LLVMGenerator.cpp executable/llvm/LLVMProgram.cpp)

set(INTERPRETER_SOURCE_FILES
  # cmake-format: sortable
  interpreter/Interpreter.cpp interpreter/InterpreterSession.cpp)

set(MEMORY_SOURCE_FILES # cmake-format: sortable
  memory/Arena.cpp
  memory/ArenaManager.cpp
  memory/Boehm_GCSetup.cpp
  memory/GarbageCollector.cpp
  memory/Heap.cpp)

set(PARSER_SOURCE_FILES # cmake-format: sortable
  parser/Parser.cpp)

set(RUNTIME_SOURCE_FILES
  # cmake-format: sortable
  runtime/CustomPyObject.cpp
  runtime/modules/BuiltinsModule.cpp
  runtime/modules/CodecsModule.cpp
  runtime/modules/ImpModule.cpp
  runtime/modules/IOModule.cpp
  runtime/modules/collections/module.cpp
  runtime/modules/collections/Deque.cpp
  runtime/modules/collections/DefaultDict.cpp
  runtime/modules/errno/module.cpp
  runtime/modules/itertools/module.cpp
  runtime/modules/itertools/Chain.cpp
  runtime/modules/itertools/Count.cpp
  runtime/modules/itertools/ISlice.cpp
  runtime/modules/itertools/Permutations.cpp
  runtime/modules/itertools/Product.cpp
  runtime/modules/itertools/Repeat.cpp
  runtime/modules/itertools/StarMap.cpp
  runtime/modules/math/module.cpp
  runtime/modules/signal/module.cpp
  runtime/modules/thread/module.cpp
  runtime/modules/time/module.cpp
  runtime/modules/weakref/module.cpp
  runtime/modules/weakref/PyCallableProxyType.cpp
  runtime/modules/weakref/PyWeakProxy.cpp
  runtime/modules/weakref/PyWeakRef.cpp
  runtime/modules/MarshalModule.cpp
  runtime/modules/PosixModule.cpp
  runtime/modules/sre/module.cpp
  runtime/modules/sre/Match.cpp
  runtime/modules/sre/Pattern.cpp
  runtime/modules/struct/module.cpp
  runtime/modules/SysModule.cpp
  runtime/modules/WarningsModule.cpp
  runtime/types/builtin.cpp
  runtime/warnings/DeprecationWarning.cpp
  runtime/warnings/ImportWarning.cpp
  runtime/warnings/PendingDeprecationWarning.cpp
  runtime/warnings/ResourceWarning.cpp
  runtime/warnings/Warning.cpp
  runtime/RuntimeContext.cpp
  runtime/AssertionError.cpp
  runtime/AttributeError.cpp
  runtime/builtinTypeInit.cpp
  runtime/BaseException.cpp
  runtime/ClassBuilder.cpp
  runtime/Exception.cpp
  runtime/GeneratorInterface.cpp
  runtime/IndexError.cpp
  runtime/Import.cpp
  runtime/ImportError.cpp
  runtime/KeyError.cpp
  runtime/LookupError.cpp
  runtime/MemoryError.cpp
  runtime/ModuleNotFoundError.cpp
  runtime/ModuleRegistry.cpp
  runtime/NameError.cpp
  runtime/NotImplemented.cpp
  runtime/NotImplementedError.cpp
  runtime/OSError.cpp
  runtime/PyAsyncGenerator.cpp
  runtime/PyBoundMethod.cpp
  runtime/PyBool.cpp
  runtime/PyBuiltInMethod.cpp
  runtime/PyBytes.cpp
  runtime/PyByteArray.cpp
  runtime/PyCell.cpp
  runtime/PyClassMethod.cpp
  runtime/PyClassMethodDescriptor.cpp
  runtime/PyCode.cpp
  runtime/PyComplex.cpp
  runtime/PyCoroutine.cpp
  runtime/PyDict.cpp
  runtime/PyEllipsis.cpp
  runtime/PyEnumerate.cpp
  runtime/PyFloat.cpp
  runtime/PyFrame.cpp
  runtime/PyFrozenSet.cpp
  runtime/PyFunction.cpp
  runtime/PyGenerator.cpp
  runtime/PyGenericAlias.cpp
  runtime/PyGetSetDescriptor.cpp
  runtime/PyInteger.cpp
  runtime/PyIterator.cpp
  runtime/PyList.cpp
  runtime/PyLLVMFunction.cpp
  runtime/PyMap.cpp
  runtime/PyMappingProxy.cpp
  runtime/PyMemberDescriptor.cpp
  runtime/PyMemoryView.cpp
  runtime/PyMethodDescriptor.cpp
  runtime/PyModule.cpp
  runtime/PyNamespace.cpp
  runtime/PyNone.cpp
  runtime/PyNumber.cpp
  runtime/PyObject.cpp
  runtime/PyProperty.cpp
  runtime/PyRange.cpp
  runtime/PyReversed.cpp
  runtime/PySet.cpp
  runtime/PySlice.cpp
  runtime/PySlotWrapper.cpp
  runtime/PyString.cpp
  runtime/PySuper.cpp
  runtime/PyStaticMethod.cpp
  runtime/PyTraceback.cpp
  runtime/PyTuple.cpp
  runtime/PyType.cpp
  runtime/PyZip.cpp
  runtime/RuntimeError.cpp
  runtime/StopIteration.cpp
  runtime/SyntaxError.cpp
  runtime/TypeError.cpp
  runtime/UnboundLocalError.cpp
  runtime/Value.cpp
  runtime/ValueError.cpp)

set(VM_SOURCE_FILES # cmake-format: sortable
  vm/VM.cpp)

# Runtime export layer — auto-collected
file(GLOB RUNTIME_EXPORT_FILES CONFIGURE_DEPENDS runtime/export/*.cpp)

set(UNITTEST_SOURCES
  # cmake-format: sortable
  ast/optimizers/Optimizers_tests.cpp
  executable/bytecode/Bytecode_tests.cpp
  executable/bytecode/BytecodeProgram_tests.cpp
  executable/bytecode/codegen/BytecodeGenerator_tests.cpp
  executable/bytecode/codegen/VariablesResolver_tests.cpp
  lexer/Lexer_tests.cpp
  memory/Arena_tests.cpp
  memory/GarbageCollector_tests.cpp
  memory/Heap_tests.cpp
  parser/Parser_tests.cpp
  runtime/PyDict_tests.cpp
  runtime/PyNumber_tests.cpp
  runtime/PyString_tests.cpp
  runtime/PyType_tests.cpp
  runtime/compat_tests.cpp
  runtime/PyInteger_refactor_tests.cpp
  runtime/tests/ArenaObject_tests.cpp
  runtime/tests/CompileIsolation_tests.cpp
  runtime/tests/Import_tests.cpp
  runtime/tests/InterpreterCore_tests.cpp
  runtime/tests/KeyVersionTracker_tests.cpp
  runtime/tests/ModuleInit_tests.cpp
  runtime/tests/ModuleMangler_tests.cpp
  runtime/tests/ModuleRegistry_tests.cpp
  runtime/tests/ProviderChain_tests.cpp
  runtime/tests/PyModule_add_symbol_tests.cpp
  runtime/tests/PyModule_version_tests.cpp
  runtime/tests/RuntimeAPI_tests.cpp
  runtime/tests/RuntimeContext_tests.cpp
  runtime/tests/RuntimeContext_truthy_tests.cpp
  runtime/tests/Truthy_fallback_tests.cpp
  runtime/tests/Value_truthy_tests.cpp
  testing/main.cpp)

set(PYTHON_LIB_PATH ${cpython_SOURCE_DIR}/Lib)
set(PYTHON_INSTALL_PATH ${CMAKE_INSTALL_PREFIX})

configure_file(runtime/modules/paths.hpp.in runtime/modules/paths.hpp)

add_library(
  python-cpp
  ${AST_SOURCE_FILES}
  ${BYTECODE_SOURCE_FILES}
  ${EXECUTABLE_SOURCE_FILES}
  ${LEXER_SOURCE_FILES}
  ${INTERPRETER_SOURCE_FILES}
  ${MEMORY_SOURCE_FILES}
  ${PARSER_SOURCE_FILES}
  ${RUNTIME_SOURCE_FILES}
  ${RUNTIME_EXPORT_FILES}
  ${VM_SOURCE_FILES})

if(ENABLE_RUNTIME_ARENA)
  target_compile_definitions(python-cpp PUBLIC PYLANG_USE_ARENA)
  # [新增链接]
  if(ENABLE_BOEHM_GC)
    target_link_libraries(python-cpp PUBLIC gc)
    # [关键修复]: 手动将 bdwgc 的头文件目录暴露给 python-cpp 及下游目标
    target_include_directories(python-cpp PUBLIC ${bdwgc_SOURCE_DIR}/include)

    # ✅ [修复]: 增加 GC_THREADS 宏，让 C++ 代码能看到多线程 API
    target_compile_definitions(python-cpp PUBLIC PYLANG_USE_Boehm_GC GC_THREADS)
  endif()
endif()

add_executable(unittests_ ${UNITTEST_SOURCES})

target_link_libraries(python-cpp
  PUBLIC spdlog m
  PRIVATE
  project_options
  project_warnings
  ICU::uc
  ICU::data
  ${GMPXX_LIBRARIES}
  ${GMP_LIBRARIES}
  tsl::ordered_map
)

target_include_directories(python-cpp
  PUBLIC .
  PRIVATE ${GMP_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_options(
  python-cpp
  PRIVATE
  -fno-omit-frame-pointer)

if(STL_SUPPORTS_BIT_CAST)
  target_compile_definitions(python-cpp PUBLIC "STL_SUPPORTS_BIT_CAST")
endif()

# MLIR backend
add_subdirectory(executable/mlir)

# Runtime bitcode generation
add_subdirectory(runtime/export)

target_link_libraries(python-cpp PUBLIC python-mlir)

# LLVM backend (JIT execution)
find_package(LLVM CONFIG 20.1)
if(ENABLE_LLVM_BACKEND AND NOT LLVM_FOUND)
  message(FATAL_ERROR "Could not find LLVM in the local environment")
elseif(ENABLE_LLVM_BACKEND AND LLVM_FOUND)
  message(STATUS "Configuring LLVM backend")
  add_library(python-cpp-llvm ${LLVM_BACKEND_FILES})

  target_compile_options(
    python-cpp-llvm
    PRIVATE -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
    -fno-omit-frame-pointer)

  add_library(llvm-interface INTERFACE)
  target_include_directories(llvm-interface INTERFACE .)
  target_include_directories(llvm-interface SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS})
  target_link_libraries(llvm-interface INTERFACE spdlog)
  separate_arguments(_llvm_backend_defs NATIVE_COMMAND ${LLVM_DEFINITIONS})
  target_compile_definitions(llvm-interface INTERFACE ${_llvm_backend_defs})

  if(STL_SUPPORTS_BIT_CAST)
    target_compile_definitions(llvm-interface INTERFACE "STL_SUPPORTS_BIT_CAST")
  endif()

  llvm_map_components_to_libnames(
    _llvm_backend_libs
    core
    orcjit
    x86asmparser
    x86codegen)
  target_link_libraries(llvm-interface INTERFACE ${_llvm_backend_libs} project_options project_warnings)
  set_property(TARGET python-cpp-llvm PROPERTY CXX_STANDARD 17)

  add_library(python-cpp-llvm-tests OBJECT "executable/llvm/LLVMGenerator_tests.cpp")
  add_library(python-cpp-llvm-glue OBJECT "executable/llvm/LLVMPyUtils.cpp" "executable/llvm/LLVMFunction.cpp")

  target_link_libraries(python-cpp-llvm-glue PRIVATE llvm-interface)
  target_link_libraries(python-cpp-llvm PRIVATE llvm-interface)
  target_sources(python-cpp-llvm PRIVATE $<TARGET_OBJECTS:python-cpp-llvm-glue>)
  target_link_libraries(python-cpp-llvm-tests PRIVATE python-cpp-llvm gtest llvm-interface)
  target_link_libraries(python-cpp PRIVATE python-cpp-llvm)
  target_compile_definitions(python-cpp PUBLIC USE_LLVM)

  target_sources(unittests_ PRIVATE $<TARGET_OBJECTS:python-cpp-llvm-tests>)
  target_link_libraries(unittests_ PRIVATE python-cpp-llvm)
endif()

target_link_libraries(unittests_ PRIVATE python-cpp gtest gtest_main cxxopts project_options project_warnings tsl::ordered_map)
gtest_discover_tests(unittests_)

add_executable(python repl/repl.cpp)
target_link_libraries(python PRIVATE linenoise cxxopts python-cpp project_options project_warnings stdc++)

add_executable(freeze utilities/freeze.cpp)
target_link_libraries(freeze PRIVATE python-cpp cxxopts project_options project_warnings)
target_include_directories(freeze SYSTEM PRIVATE ${MLIR_INCLUDE_DIRS})
# =========================================================================
# Compiler Frontend — header-only support library
# =========================================================================
add_library(pylang-support INTERFACE)
target_include_directories(pylang-support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(LLVM_FOUND)
  separate_arguments(_pylang_support_llvm_defs NATIVE_COMMAND ${LLVM_DEFINITIONS})
  target_compile_definitions(pylang-support INTERFACE ${_pylang_support_llvm_defs})

  # ✅ 新增 linker 组件（用于 llvm::Linker）
  llvm_map_components_to_libnames(_pylang_support_llvm_libs
    core
    irreader
    bitreader
    bitwriter
    support
    transformutils # CloneModule
    linker # llvm::Linker
    native
    nativecodegen
    target
  )
  target_link_libraries(pylang-support INTERFACE ${_pylang_support_llvm_libs})
endif()

target_link_libraries(pylang-support INTERFACE cpptrace::cpptrace spdlog)

# =========================================================================
# Compiler Frontend — RuntimeLinker
# =========================================================================
if(LLVM_FOUND)
  find_program(LLD_PATH lld)
  if(LLD_PATH)
    add_compile_definitions(PYLANG_HAS_LLD=1)
  endif()
  # 收集编译器所有源文件（排除测试文件）
  file(GLOB_RECURSE COMPILER_SOURCE_FILES
    CONFIGURE_DEPENDS
    compiler/**/*.cpp
  )
  list(FILTER COMPILER_SOURCE_FILES EXCLUDE REGEX ".*_test\\.cpp$")
  list(FILTER COMPILER_SOURCE_FILES EXCLUDE REGEX ".*_tests\\.cpp$")
  list(FILTER COMPILER_SOURCE_FILES EXCLUDE REGEX ".*_SimpleDriverExecEntry/.*")

  if(COMPILER_SOURCE_FILES)
    add_library(pylang-compiler OBJECT ${COMPILER_SOURCE_FILES})
    target_link_libraries(pylang-compiler PUBLIC pylang-support)
    target_include_directories(pylang-compiler PRIVATE ${LLVM_INCLUDE_DIRS})
  endif()

  # -------------------------------------------------------------------------
  # 定义辅助宏，减少测试目标的配置重复
  # -------------------------------------------------------------------------
  macro(setup_compiler_test_target TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} PRIVATE
      pylang-support
      pylang-compiler
      python-cpp
      gtest
      gtest_main
    )
    target_compile_definitions(${TARGET_NAME} PRIVATE
      PYLANG_RUNTIME_BC_DEFAULT="${CMAKE_BINARY_DIR}/runtime.bc"
    )
    if(TARGET runtime_bc)
      add_dependencies(${TARGET_NAME} runtime_bc)
    endif()
  endmacro()

  # =========================================================================
  # 1. 编译器单元测试 (Unit Tests) - 快速，基于内存的组件测试
  # =========================================================================
  add_executable(compiler_unit_tests
    compiler/Codegen/IREmitter_test.cpp
    compiler/RuntimeLinker/RuntimeLinker_test.cpp
    compiler/Support/LLVMUtils_test.cpp
    compiler/Support/StringUtils_test.cpp
  )
  setup_compiler_test_target(compiler_unit_tests)
  gtest_discover_tests(compiler_unit_tests PROPERTIES LABELS "Unit")

  # =========================================================================
  # 2. 代码生成测试 (Codegen Tests) - 中等速度，验证 IR 结构
  # =========================================================================
  add_executable(compiler_codegen_tests
    compiler/Codegen/PylangCodegen_test.cpp
  )
  setup_compiler_test_target(compiler_codegen_tests)
  gtest_discover_tests(compiler_codegen_tests PROPERTIES LABELS "Codegen")

  # =========================================================================
  # 3. 集成测试 (Integration Tests) - 慢速，涉及 AOT 编译、系统链接和子进程运行
  # =========================================================================
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/compiler/Driver/_SimpleDriver_test.cpp")
    add_executable(compiler_integration_tests
      compiler/Driver/_SimpleDriver_test.cpp
    )
    setup_compiler_test_target(compiler_integration_tests)
    # 集成测试设置较长的超时时间，并标记 Label
    gtest_discover_tests(compiler_integration_tests
      PROPERTIES
      LABELS "Integration"
      TIMEOUT 300
    )
  endif()

endif()

# =========================================================================
# 编译器 CLI 入口 (pyc)
# =========================================================================
if(LLVM_FOUND)

  # 1. 定位资源
  set(RUNTIME_BC_FILE "${CMAKE_BINARY_DIR}/runtime.bc")
  set(EMBEDDED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedResources.hpp")
  set(EMBED_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/EmbedResources.cmake")

  # 寻找静态库 (注意：确保这些库在你的系统上存在 .a 版本)
  find_library(LIB_GMP_STATIC NAMES libgmp.a gmp)
  find_library(LIB_GMPXX_STATIC NAMES libgmpxx.a gmpxx)
  find_library(LIB_ICUUC_STATIC NAMES libicuuc.a icuuc)
  find_library(LIB_ICUDATA_STATIC NAMES libicudata.a icudata)

  # 获取 spdlog 静态文件路径
  if(TARGET spdlog)
    set(LIB_SPDLOG "$<TARGET_FILE:spdlog>")
  else()
    find_library(LIB_SPDLOG NAMES libspdlog.a spdlog)
  endif()

  # ✅ [新增] 获取 Boehm GC 静态库路径
  if(TARGET gc)
    set(LIB_GC_STATIC "$<TARGET_FILE:gc>")
    set(GC_EMBED_ARG "LIB_GC=${LIB_GC_STATIC};")
    set(GC_DEPEND gc)
  else()
    set(GC_EMBED_ARG "")
    set(GC_DEPEND "")
  endif()

  # 2. 只有当必要的资源存在时才配置生成逻辑
  # 注意：这里 DEPENDS 必须包含生成 runtime.bc 的 TARGET 名：runtime_bitcode
  if(EXISTS "${EMBED_SCRIPT}")
    add_custom_command(
      OUTPUT ${EMBEDDED_HEADER}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
      COMMAND ${CMAKE_COMMAND}
      -DOUTPUT_HEADER=${EMBEDDED_HEADER}
      # ✅ [修改] 把 GC_EMBED_ARG 塞进传递给脚本的宏里
      "-DFILES=RUNTIME_BC=${RUNTIME_BC_FILE};LIB_SPDLOG=${LIB_SPDLOG};LIB_GMP=${LIB_GMP_STATIC};LIB_GMPXX=${LIB_GMPXX_STATIC};LIB_ICUUC=${LIB_ICUUC_STATIC};LIB_ICUDATA=${LIB_ICUDATA_STATIC};${GC_EMBED_ARG}"
      -P ${EMBED_SCRIPT}
      # ✅ [修改] 把 GC_DEPEND 加入依赖列表
      DEPENDS runtime_bitcode spdlog ${GC_DEPEND} ${LIB_GMP_STATIC} ${EMBED_SCRIPT}
      COMMENT "Embedding runtime libraries and bitcode into pyc header..."
      VERBATIM
    )

    add_library(pyc_resources INTERFACE)
    target_include_directories(pyc_resources INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated)
    target_compile_definitions(pyc_resources INTERFACE PYLANG_EMBEDDED_RES)

    add_custom_target(gen_pyc_embedded_header DEPENDS ${EMBEDDED_HEADER})
  endif()

  # 3. 构建 pyc
  add_executable(pyc compiler/Driver/_SimpleDriverExecEntry/CompilerEntry.cpp)

  target_link_libraries(pyc PRIVATE
    pylang-compiler python-cpp cxxopts cpptrace::cpptrace spdlog
  )

  # 确保头文件生成
  if(TARGET gen_pyc_embedded_header)
    add_dependencies(pyc gen_pyc_embedded_header)
    target_link_libraries(pyc PRIVATE pyc_resources)
  endif()

  target_include_directories(pyc PRIVATE ${LLVM_INCLUDE_DIRS})
  target_compile_definitions(pyc PRIVATE PYLANG_RUNTIME_BC_DEFAULT="${RUNTIME_BC_FILE}")

  if(PYLANG_HAS_LLD)
    target_compile_definitions(pyc PRIVATE PYLANG_HAS_LLD=1)
  endif()

  install(TARGETS pyc DESTINATION bin)
endif()
