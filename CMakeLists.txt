cmake_minimum_required(VERSION 3.25)
include(FetchContent)
include(ExternalProject)
include(CheckCXXSourceCompiles)

project(python++)

set(CMAKE_CXX_STANDARD 20)

include(cmake/CPM.cmake)

CPMAddPackage("gh:gabime/spdlog@1.8.5")
CPMAddPackage("gh:google/googletest@1.17.0")
CPMAddPackage("gh:jarro2783/cxxopts@3.3.1")
CPMAddPackage("gh:Tessil/ordered-map@1.2.0")
CPMAddPackage("gh:python/cpython@3.9.25")
CPMAddPackage("gh:antirez/linenoise#2.0")
CPMAddPackage("gh:aminya/project_options@0.41.0")

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
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_library(GTest::GTest ALIAS gtest)
add_library(GTest::GMock ALIAS gmock)
add_library(GTest::Main ALIAS gtest_main)

option(ENABLE_CACHE "Enable build cache" ON)
option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
option(ENABLE_LLVM_BACKEND "Enable LLVM as a Python execution backend" OFF)

# Enable building runtime in shared_ptr (RAII) compatibility mode. When ON
# the compile definition PYLANG_USE_SHARED_PTR will be set for the runtime
# target so tests exercising the shared_ptr-path are built.
option(ENABLE_RUNTIME_SHARED_PTR "Enable runtime shared_ptr compatibility (defines PYLANG_USE_SHARED_PTR)" OFF)

if (${ENABLE_CACHE})
	set(ENABLE_CACHE "ENABLE_CACHE")
endif()

if (${ENABLE_SANITIZER_ADDRESS})
	set(ENABLE_SANITIZER_ADDRESS "ENABLE_SANITIZER_ADDRESS")
endif()

if (${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR})
	set(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "ENABLE_SANITIZER_UNDEFINED_BEHAVIOR")
endif()

project_options(
	WARNINGS_AS_ERRORS
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
	${ENABLE_SANITIZER_ADDRESS}
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
    -Wno-maybe-uninitialized   # GCC 特有
    -Wno-stringop-overflow     # GCC 特有
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

enable_testing()
add_subdirectory(src)
add_subdirectory(integration)
