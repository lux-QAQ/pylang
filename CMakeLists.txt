cmake_minimum_required(VERSION 3.25)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
include(FetchContent)
include(ExternalProject)
include(CheckCXXSourceCompiles)

project(python++)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

option(PYLANG_EMBED_LIBRARY_RESOURCES "Embed runtime bitcode and static library resources into pyc" OFF)
option(PYLANG_REQUIRE_SHARED_DEPENDENCIES
    "In non-embedded mode, require dependencies used by pyc-generated binaries to be shared libraries"
    ON)
set(PYLANG_LLVM_VERSION "20" CACHE STRING "LLVM/MLIR major version used by pylang")

set(PYLANG_USE_SHARED_DEPENDENCIES OFF)
if(NOT PYLANG_EMBED_LIBRARY_RESOURCES AND PYLANG_REQUIRE_SHARED_DEPENDENCIES)
    set(PYLANG_USE_SHARED_DEPENDENCIES ON)
endif()

if(PYLANG_USE_SHARED_DEPENDENCIES)
    message(STATUS "Pylang dependency mode: shared libraries required")
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "Build spdlog as a shared library" FORCE)
    set(CPPTRACE_BUILD_SHARED ON CACHE BOOL "Build cpptrace as a shared library" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_ADDR2LINE ON CACHE BOOL "Use addr2line instead of nested static libdwarf in shared mode" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "Disable nested libdwarf in shared mode" FORCE)
    set(ZSTD_BUILD_SHARED ON CACHE BOOL "Build zstd shared library" FORCE)
    set(ZSTD_BUILD_STATIC OFF CACHE BOOL "Do not build zstd static library in dynamic mode" FORCE)
    set(BUILD_SHARED ON CACHE BOOL "Build libdwarf shared library when libdwarf is pulled by dependencies" FORCE)
    set(BUILD_NON_SHARED OFF CACHE BOOL "Do not build libdwarf static library in dynamic mode" FORCE)
else()
    message(STATUS "Pylang dependency mode: embedded/static resources allowed")
endif()

if(PYLANG_USE_SHARED_DEPENDENCIES)
    set(_PYLANG_ZSTD_BUILD_STATIC OFF)
else()
    set(_PYLANG_ZSTD_BUILD_STATIC ON)
endif()

function(pylang_resolve_aliased_target OUT TARGET_NAME)
    if(TARGET "${TARGET_NAME}")
        get_target_property(_aliased "${TARGET_NAME}" ALIASED_TARGET)
        if(_aliased)
            set("${OUT}" "${_aliased}" PARENT_SCOPE)
        else()
            set("${OUT}" "${TARGET_NAME}" PARENT_SCOPE)
        endif()
    else()
        set("${OUT}" "" PARENT_SCOPE)
    endif()
endfunction()

function(pylang_require_shared_target TARGET_NAME HINT)
    if(NOT PYLANG_USE_SHARED_DEPENDENCIES)
        return()
    endif()
    pylang_resolve_aliased_target(_real_target "${TARGET_NAME}")
    if(NOT _real_target)
        return()
    endif()
    get_target_property(_target_type "${_real_target}" TYPE)
    if(_target_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
            "Dependency target '${TARGET_NAME}' was configured as STATIC_LIBRARY, but "
            "PYLANG_EMBED_LIBRARY_RESOURCES is OFF and generated binaries default to dynamic linking.\n"
            "Fix: ${HINT}\n"
            "Alternative: configure with -DPYLANG_EMBED_LIBRARY_RESOURCES=ON and use pyc --static.")
    endif()
endfunction()

function(pylang_find_required_shared_library OUT DEP_NAME)
    if(NOT PYLANG_USE_SHARED_DEPENDENCIES)
        set("${OUT}" "" PARENT_SCOPE)
        return()
    endif()

    set(_old_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_search_var "_pylang_shared_${OUT}")
    unset(${_search_var} CACHE)
    unset(${_search_var})
    find_library(${_search_var} NAMES ${ARGN})
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${_old_suffixes}")

    if(NOT ${_search_var})
        message(FATAL_ERROR
            "Could not find shared library for ${DEP_NAME}. "
            "Install the system dynamic development package for ${DEP_NAME}, or configure "
            "with -DPYLANG_EMBED_LIBRARY_RESOURCES=ON for embedded/static mode.")
    endif()

    set("${OUT}" "${${_search_var}}" CACHE FILEPATH "Shared library for ${DEP_NAME}" FORCE)
    set("${OUT}" "${${_search_var}}" PARENT_SCOPE)
endfunction()

function(pylang_configure_llvm_package_dirs)
    if(NOT PYLANG_LLVM_VERSION)
        return()
    endif()

    set(_prefix_candidates
        "/usr/lib/llvm-${PYLANG_LLVM_VERSION}"
        "/usr/local/opt/llvm@${PYLANG_LLVM_VERSION}"
        "/opt/homebrew/opt/llvm@${PYLANG_LLVM_VERSION}"
    )

    foreach(_prefix IN LISTS _prefix_candidates)
        if((NOT DEFINED LLVM_DIR OR LLVM_DIR STREQUAL "")
            AND EXISTS "${_prefix}/lib/cmake/llvm/LLVMConfig.cmake")
            set(LLVM_DIR "${_prefix}/lib/cmake/llvm" CACHE PATH
                "LLVM CMake package directory" FORCE)
        endif()
        if((NOT DEFINED MLIR_DIR OR MLIR_DIR STREQUAL "")
            AND EXISTS "${_prefix}/lib/cmake/mlir/MLIRConfig.cmake")
            set(MLIR_DIR "${_prefix}/lib/cmake/mlir" CACHE PATH
                "MLIR CMake package directory" FORCE)
        endif()
    endforeach()
endfunction()

# 移除旧的 add_compile_definitions，改用更强力的全局 Flags 注入
# add_compile_definitions(ALWAYS_SMALL_CLEAR_STACK)
if(MSVC)
    add_compile_options("$<$<CONFIG:Release>:/O2>")
else()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
endif()
# [强制全局定义]
# 直接修改 C_FLAGS 是 100% 确保宏传递给 bdwgc (包括其内部 misc.c) 的方法
# 这将强制 GC_clear_stack 使用 memset (BZERO) 而非递归清除，彻底根除 Stack Overflow
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DALWAYS_SMALL_CLEAR_STACK -DSMALL_CLEAR_SIZE=4")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DALWAYS_SMALL_CLEAR_STACK -DSMALL_CLEAR_SIZE=4")


include(cmake/CPM.cmake)

CPMAddPackage(
    NAME zstd
    GITHUB_REPOSITORY facebook/zstd
    GIT_TAG v1.5.5
    SOURCE_SUBDIR build/cmake
    OPTIONS
    "ZSTD_BUILD_PROGRAMS OFF" # 不编译命令行工具
    "ZSTD_BUILD_SHARED ${PYLANG_USE_SHARED_DEPENDENCIES}"
    "ZSTD_BUILD_STATIC ${_PYLANG_ZSTD_BUILD_STATIC}"
    "ZSTD_BUILD_CONTRIB OFF"
)

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.8.5
    OPTIONS
    "SPDLOG_BUILD_SHARED ${PYLANG_USE_SHARED_DEPENDENCIES}"
)
CPMAddPackage("gh:google/googletest@1.17.0")
CPMAddPackage("gh:jarro2783/cxxopts@3.3.1")
CPMAddPackage(
    NAME ordered_map
    GITHUB_REPOSITORY Tessil/ordered-map
    GIT_TAG v1.2.0
)
CPMAddPackage("gh:python/cpython@3.9.25")
CPMAddPackage("gh:antirez/linenoise#2.0")
CPMAddPackage("gh:aminya/project_options@0.41.0")
CPMAddPackage(
    NAME cpptrace
    GITHUB_REPOSITORY jeremy-rifkin/cpptrace
    GIT_TAG v0.8.3
    OPTIONS
    "CPPTRACE_BUILD_SHARED ${PYLANG_USE_SHARED_DEPENDENCIES}"
)

# [新增]: 通过 CPM 引入阿里 yalantinglibs (提供编译期反射 ylt/reflection)
CPMAddPackage(
    NAME yalantinglibs
    GITHUB_REPOSITORY alibaba/yalantinglibs
    GIT_TAG 0.6.0 # 或选择您需要的版本
)

# [新增]: 通过 CPM 按序引入 BDWGC 的依赖项和 BDWGC
CPMAddPackage(
    NAME libatomic_ops
    GITHUB_REPOSITORY ivmai/libatomic_ops
    GIT_TAG v7.8.2
)
CPMAddPackage(
    NAME bdwgc
    GITHUB_REPOSITORY ivmai/bdwgc
    GIT_TAG v8.2.6
    OPTIONS
    "enable_threads ON" # 允许无 GIL 的多线程分配
    "enable_parallel_mark ON" # 强制开启并行标记（如果系统支持）
    "enable_handle_fork ON" # 确保多线程环境下的进程安全
    "enable_cplusplus OFF" # 启用兼容性
    "build_tests OFF" # 不要编译它的测试
    "build_docs OFF"
    # [删除] 之前建议的 target 补丁代码现在是多余的了，因为全局 Flag 已经覆盖了
)

if(PYLANG_USE_SHARED_DEPENDENCIES)
    # BUILD_SHARED_LIBS is needed while configuring dependencies such as bdwgc,
    # but the in-tree python-cpp/python-mlir libraries have cyclic references
    # that are only valid for static/object-style targets.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Keep project-internal libraries non-shared" FORCE)
endif()

# 将 ALWAYS_SMALL_CLEAR_STACK 强制注入到 bdwgc 的编译目标
# add_compile_definitions 只影响当前项目的 target，不影响 CPM 子项目
if(TARGET gc)
    target_compile_definitions(gc PRIVATE ALWAYS_SMALL_CLEAR_STACK)
endif()
# 同时处理可能的静态库目标名
if(TARGET gc-lib)
    target_compile_definitions(gc-lib PRIVATE ALWAYS_SMALL_CLEAR_STACK)
endif()

if(linenoise_ADDED)
    add_library(linenoise ${linenoise_SOURCE_DIR}/linenoise.c)
    target_include_directories(linenoise SYSTEM PUBLIC ${linenoise_SOURCE_DIR})
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(ZSTD)
if(NOT TARGET zstd::libzstd_shared)
    # 尝试寻找系统库并手动创建 Target
    find_library(ZSTD_LIB NAMES zstd)
    find_path(ZSTD_INCLUDE NAMES zstd.h)
    if(ZSTD_LIB AND ZSTD_INCLUDE)
        add_library(zstd::libzstd_shared INTERFACE IMPORTED)
        set_target_properties(zstd::libzstd_shared PROPERTIES
            INTERFACE_LINK_LIBRARIES "${ZSTD_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE}"
        )
    endif()
endif()
find_package(GMP REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc data)
pylang_configure_llvm_package_dirs()
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
string(REGEX MATCH "^[0-9]+" _pylang_found_llvm_major "${LLVM_PACKAGE_VERSION}")
string(REGEX MATCH "^[0-9]+" _pylang_required_llvm_major "${PYLANG_LLVM_VERSION}")
if(NOT _pylang_found_llvm_major STREQUAL _pylang_required_llvm_major)
    message(FATAL_ERROR
        "Found LLVM ${LLVM_PACKAGE_VERSION}, but pylang is configured for LLVM "
        "${PYLANG_LLVM_VERSION}. Set -DPYLANG_LLVM_VERSION, -DLLVM_DIR, and "
        "-DMLIR_DIR to matching package directories.")
endif()
find_package(MLIR REQUIRED CONFIG)
message(STATUS "Found MLIR package in: ${MLIR_DIR}")

pylang_require_shared_target(spdlog::spdlog "Build spdlog shared, or install a shared spdlog package.")
pylang_require_shared_target(cpptrace::cpptrace "Build cpptrace shared, or install a shared cpptrace package.")
pylang_require_shared_target(gc "Build Boehm GC shared, or install libgc.so development files.")
pylang_require_shared_target(zstd::libzstd_shared "Build zstd shared, or install libzstd.so development files.")

pylang_find_required_shared_library(PYLANG_SHARED_GMP gmp gmp)
pylang_find_required_shared_library(PYLANG_SHARED_GMPXX gmpxx gmpxx)
pylang_find_required_shared_library(PYLANG_SHARED_ICUUC ICU uc icuuc)
pylang_find_required_shared_library(PYLANG_SHARED_ICUDATA ICU data icudata)
pylang_find_required_shared_library(PYLANG_SHARED_ZSTD zstd zstd)
pylang_find_required_shared_library(PYLANG_SHARED_Z zlib z)

add_library(GTest::GTest ALIAS gtest)
add_library(GTest::GMock ALIAS gmock)
add_library(GTest::Main ALIAS gtest_main)

option(ENABLE_CACHE "Enable build cache" ON)
option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ON)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
option(ENABLE_LLVM_BACKEND "Enable LLVM as a Python execution backend" OFF)

# Enable Arena/Region-based memory management in the runtime.
# When ON the compile definition PYLANG_USE_ARENA will be set, replacing
# the Heap + GC allocation path with Arena bump allocation.
option(ENABLE_RUNTIME_ARENA "Enable runtime Arena memory management (defines PYLANG_USE_ARENA)" ON)

# [新增]: 添加 Boehm GC 开启选项
option(ENABLE_BOEHM_GC "Enable Boehm GC instead of Arena (defines PYLANG_USE_Boehm_GC)" ON)

if(ENABLE_RUNTIME_ARENA)
    add_compile_definitions(PYLANG_USE_ARENA)
    message(STATUS "Global Macro Defined: PYLANG_USE_ARENA")

    # [新增]
    if(ENABLE_BOEHM_GC)
        add_compile_definitions(PYLANG_USE_Boehm_GC)
        message(STATUS "Global Macro Defined: PYLANG_USE_Boehm_GC")
    endif()
endif()

if(${ENABLE_CACHE})
    set(ENABLE_CACHE "ENABLE_CACHE")
endif()

if(${ENABLE_SANITIZER_ADDRESS})
    set(ENABLE_SANITIZER_ADDRESS "ENABLE_SANITIZER_ADDRESS")
endif()

if(${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR})
    set(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "ENABLE_SANITIZER_UNDEFINED_BEHAVIOR")
endif()

project_options(
    #WARNINGS_AS_ERRORS
    # ENABLE_COVERAGE
    # ENABLE_CPPCHECK
    # ENABLE_CLANG_TIDY
    # ENABLE_VS_ANALYSIS
    # ENABLE_INCLUDE_WHAT_YOU_USE
    ${ENABLE_CACHE}
    # ENABLE_PCH
    # ENABLE_CONAN
    # ENABLE_VCPKG
    # ENABLE_DOXYGEN
    # ENABLE_INTERPROCEDURAL_OPTIMIZATION
    # ENABLE_NATIVE_OPTIMIZATION
    # ENABLE_USER_LINKER
    # ENABLE_BUILD_WITH_TIME_TRACE
    # ENABLE_UNITY
    #${ENABLE_SANITIZER_ADDRESS}
    # ENABLE_SANITIZER_LEAK
    ${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}
    # ENABLE_SANITIZER_THREAD
    # ENABLE_SANITIZER_MEMORY
)

# TODO: Address all the warning below. This should be only temporary...
include(CheckCXXCompilerFlag)

# 定义所有你希望使用的警告屏蔽参数
set(DESIRED_WARNING_FLAGS
    -Wno-sign-conversion
    -Wno-shadow
    -Wno-implicit-fallthrough
    -Wno-old-style-cast
    -Wno-deprecated-copy
    -Wno-missing-field-initializers
    -Wno-null-dereference
    -Wno-maybe-uninitialized # GCC 特有
    -Wno-stringop-overflow # GCC 特有
)

# 创建一个列表来存储当前编译器支持的参数
set(SUPPORTED_WARNING_FLAGS "")

foreach(flag ${DESIRED_WARNING_FLAGS})
    # 将参数名转为合法的变量名（如 -Wno-shadow -> HAS_Wno_shadow）
    string(REPLACE "-" "_" safe_name ${flag})
    check_cxx_compiler_flag(${flag} HAS${safe_name})

    if(HAS${safe_name})
        list(APPEND SUPPORTED_WARNING_FLAGS ${flag})
    endif()
endforeach()

# 如果是 Clang，建议额外加上这个，防止未来遇到类似问题直接挂掉
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND SUPPORTED_WARNING_FLAGS "-Wno-unknown-warning-option")
endif()

# 修改后的 target_compile_options
target_compile_options(project_warnings
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${SUPPORTED_WARNING_FLAGS}>
)

check_cxx_source_compiles(
    "#include <bit>
  constexpr double f64v = 19880124.0;
  constexpr auto u64v = std::bit_cast<std::uint64_t>(f64v);"
    STL_SUPPORTS_BIT_CAST)

find_library(MATH_LIBRARY m)
if(NOT MATH_LIBRARY)
    message(FATAL_ERROR "Could not find math library")
endif()

# 传递 build 目录给测试
add_compile_definitions(CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")

enable_testing()
add_subdirectory(src)
add_subdirectory(integration)
