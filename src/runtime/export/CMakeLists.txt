# =============================================================================
# Runtime Bitcode Generation — 使用 Clang 原生 -emit-llvm 和 ccache
# =============================================================================

# 1. 查找依赖工具 (替换 wllvm 为 clang++)
find_program(CLANG_CXX NAMES clang++-20 clang++)
find_program(LLVM_LINK_EXEC NAMES llvm-link-20 llvm-link)
find_program(CCACHE_PROGRAM ccache) # 查找 ccache

if(NOT CLANG_CXX OR NOT LLVM_LINK_EXEC)
  message(STATUS "clang++ / llvm-link not found — runtime.bc disabled")
  return()
endif()

message(STATUS "clang++    = ${CLANG_CXX}")
message(STATUS "llvm-link  = ${LLVM_LINK_EXEC}")
if(CCACHE_PROGRAM)
  message(STATUS "ccache     = ${CCACHE_PROGRAM} (Enabled for Bitcode generation)")
else()
  message(STATUS "ccache     = Not found")
endif()

# ---------------------------------------------------------------------------
# GCC toolchain（解决 clang 找不到 stdlib.h 的问题）
# ---------------------------------------------------------------------------
execute_process(COMMAND gcc -dumpmachine OUTPUT_VARIABLE GCC_MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND gcc -dumpversion OUTPUT_VARIABLE GCC_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
set(GCC_INCLUDE_DIR "/usr/lib/gcc/${GCC_MACHINE}/${GCC_VERSION}/include")
message(STATUS "Bitcode GCC include: ${GCC_INCLUDE_DIR}")

# ---------------------------------------------------------------------------
# 收集源文件
# ---------------------------------------------------------------------------
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..") # => src/

file(GLOB EXPORT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB CACHE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cache/*.cpp")
file(GLOB TAGGERED_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/runtime/taggered_pointer/*.cpp")

set(ALL_SOURCES ${EXPORT_SOURCES} ${CACHE_SOURCES} ${TAGGERED_SOURCES})
foreach(src ${RUNTIME_SOURCE_FILES} ${MEMORY_SOURCE_FILES})
  list(APPEND ALL_SOURCES "${SRC_DIR}/${src}")
endforeach()

list(LENGTH ALL_SOURCES _count)
message(STATUS "Bitcode: ${_count} source files")

if(_count EQUAL 0)
  message(WARNING "Bitcode: ALL_SOURCES is empty — runtime.bc will not be generated")
  return()
endif()

# ---------------------------------------------------------------------------
# Include 路径
# ---------------------------------------------------------------------------
set(BC_INC
  "-I" "${SRC_DIR}"
  "-I" "${CMAKE_BINARY_DIR}/src"
  "--gcc-toolchain=/usr"
  "-isystem" "${GCC_INCLUDE_DIR}"
)

get_target_property(_pub_inc python-cpp INCLUDE_DIRECTORIES)
if(_pub_inc)
  foreach(dir ${_pub_inc})
    if(NOT dir MATCHES "^\\$<" AND NOT dir STREQUAL "")
      list(APPEND BC_INC "-I" "${dir}")
    endif()
  endforeach()
endif()

if(DEFINED spdlog_SOURCE_DIR)
  list(APPEND BC_INC "-isystem" "${spdlog_SOURCE_DIR}/include")
endif()

foreach(_var "ordered-map_SOURCE_DIR" "tsl-ordered-map_SOURCE_DIR" "ordered_map_SOURCE_DIR")
  if(DEFINED ${_var})
    list(APPEND BC_INC "-isystem" "${${_var}}/include")
    break()
  endif()
endforeach()

if(DEFINED bdwgc_SOURCE_DIR)
  list(APPEND BC_INC "-isystem" "${bdwgc_SOURCE_DIR}/include")
endif()

if(DEFINED cpptrace_SOURCE_DIR)
  list(APPEND BC_INC "-isystem" "${cpptrace_SOURCE_DIR}/include")
endif()

if(DEFINED yalantinglibs_SOURCE_DIR)
  list(APPEND BC_INC "-isystem" "${yalantinglibs_SOURCE_DIR}/include")
  list(APPEND BC_INC "-isystem" "${yalantinglibs_SOURCE_DIR}/include/ylt/thirdparty")
endif()

if(GMP_INCLUDES)
  list(APPEND BC_INC "-I" "${GMP_INCLUDES}")
endif()

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
set(BC_DEFS "-DSPDLOG_COMPILED_LIB")
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND BC_DEFS "-DDEBUG")
endif()

list(APPEND BC_DEFS "-DPYLANG_AOT_MODE")
if(ENABLE_RUNTIME_ARENA)
  list(APPEND BC_DEFS "-DPYLANG_USE_ARENA")
  if(ENABLE_BOEHM_GC)
    list(APPEND BC_DEFS "-DPYLANG_USE_Boehm_GC")
    list(APPEND BC_DEFS "-DGC_THREADS")
  endif()
endif()

# ---------------------------------------------------------------------------
# 每个 .cpp → .bc (直接生成，跳过 .o)
# ---------------------------------------------------------------------------
set(BC_DIR "${CMAKE_BINARY_DIR}/bitcode")
file(MAKE_DIRECTORY "${BC_DIR}")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(BC_OPT_LEVEL "-O3")
  message(STATUS "Bitcode: Release build — using -O3")
else()
  set(BC_OPT_LEVEL "-O1")
  message(STATUS "Bitcode: ${CMAKE_BUILD_TYPE} build — using -O1")
endif()

# 组装编译器调用命令 (集成 ccache)
set(COMPILER_CMD "")
if(CCACHE_PROGRAM)
  list(APPEND COMPILER_CMD "${CCACHE_PROGRAM}")
endif()
list(APPEND COMPILER_CMD "${CLANG_CXX}")

set(ALL_BC_FILES "")

foreach(src ${ALL_SOURCES})
  file(RELATIVE_PATH rel "${SRC_DIR}" "${src}")
  string(REPLACE "/" "_" name "${rel}")
  string(REPLACE ".cpp" "" name "${name}")

  set(bc "${BC_DIR}/${name}.bc")

  # 直接使用 clang++ -emit-llvm -c 生成 .bc
  add_custom_command(
    OUTPUT "${bc}"
    COMMAND ${COMPILER_CMD}
    -std=c++23 ${BC_OPT_LEVEL} -g -c -emit-llvm
    -fno-discard-value-names
    ${BC_DEFS}
    ${BC_INC}
    -Wno-everything
    -o "${bc}" "${src}"
    DEPENDS "${src}"
    COMMENT "clang++ bitcode: ${rel}"
    VERBATIM
  )

  list(APPEND ALL_BC_FILES "${bc}")
endforeach()

# ---------------------------------------------------------------------------
# llvm-link 合并为 runtime.bc
# ---------------------------------------------------------------------------
set(RUNTIME_BC "${CMAKE_BINARY_DIR}/runtime.bc")

add_custom_command(
  OUTPUT "${RUNTIME_BC}"
  COMMAND ${LLVM_LINK_EXEC} -o "${RUNTIME_BC}" ${ALL_BC_FILES}
  DEPENDS ${ALL_BC_FILES}
  COMMENT "Linking runtime.bc (${_count} modules)"
  VERBATIM
)

add_custom_target(runtime_bitcode ALL DEPENDS "${RUNTIME_BC}")

set(PYLANG_RUNTIME_BC "${RUNTIME_BC}" CACHE FILEPATH "Path to runtime.bc" FORCE)
message(STATUS "runtime.bc → ${RUNTIME_BC}")
