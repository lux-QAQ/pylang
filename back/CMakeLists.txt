# =============================================================================
# Runtime Bitcode Generation
#
# 将 export/*.cpp 编译为 LLVM Bitcode，
# 最终合并为单个 runtime.bc，供编译器前端的 RuntimeLinker 加载。
# =============================================================================

# 强制清除缓存，避免使用上次缓存的 ccache 路径
unset(CLANGXX_EXECUTABLE CACHE)
unset(LLVM_LINK_EXECUTABLE CACHE)

# 优先在 /usr/bin 找真实二进制（跳过 ccache）
find_program(CLANGXX_EXECUTABLE
  NAMES clang++-20 clang++-19 clang++-18 clang++
  PATHS /usr/bin /usr/local/bin
  NO_DEFAULT_PATH
  DOC "Clang++ for bitcode generation"
)
if(NOT CLANGXX_EXECUTABLE)
  find_program(CLANGXX_EXECUTABLE NAMES clang++-20 clang++-19 clang++-18 clang++)
endif()

find_program(LLVM_LINK_EXECUTABLE
  NAMES llvm-link-20 llvm-link-19 llvm-link-18 llvm-link
  PATHS /usr/bin /usr/local/bin
  NO_DEFAULT_PATH
  DOC "llvm-link for merging bitcode"
)
if(NOT LLVM_LINK_EXECUTABLE)
  find_program(LLVM_LINK_EXECUTABLE NAMES llvm-link-20 llvm-link-19 llvm-link-18 llvm-link)
endif()

if(NOT CLANGXX_EXECUTABLE)
  message(STATUS "clang++ not found — runtime.bc target disabled")
  return()
endif()

if(NOT LLVM_LINK_EXECUTABLE)
  message(STATUS "llvm-link not found — runtime.bc target disabled")
  return()
endif()

message(STATUS "Bitcode: clang++ = ${CLANGXX_EXECUTABLE}")
message(STATUS "Bitcode: llvm-link = ${LLVM_LINK_EXECUTABLE}")

# ---------------------------------------------------------------------------
# 获取 clang++ 自身的 resource-dir（解决 stdlib.h 找不到的问题）
# ---------------------------------------------------------------------------
execute_process(
  COMMAND ${CLANGXX_EXECUTABLE} -print-resource-dir
  OUTPUT_VARIABLE CLANG_RESOURCE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(CLANG_RESOURCE_DIR)
  message(STATUS "Bitcode: clang resource-dir = ${CLANG_RESOURCE_DIR}")
endif()

# 获取 clang++ 默认的系统 include 路径
# 这解决 #include_next <stdlib.h> 找不到的问题
execute_process(
  COMMAND ${CLANGXX_EXECUTABLE} -xc++ -E -v /dev/null
  OUTPUT_QUIET
  ERROR_VARIABLE CLANG_VERBOSE_OUTPUT
)

# ---------------------------------------------------------------------------
# 收集需要编译为 bitcode 的源文件
# ---------------------------------------------------------------------------
file(GLOB EXPORT_BC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Runtime 核心源文件 — 拼出绝对路径
set(RUNTIME_BC_SOURCES_ABS "")
foreach(src ${RUNTIME_SOURCE_FILES})
  list(APPEND RUNTIME_BC_SOURCES_ABS "${CMAKE_CURRENT_SOURCE_DIR}/../../${src}")
endforeach()

# Memory 源文件
set(MEMORY_BC_SOURCES_ABS "")
foreach(src ${MEMORY_SOURCE_FILES})
  list(APPEND MEMORY_BC_SOURCES_ABS "${CMAKE_CURRENT_SOURCE_DIR}/../../${src}")
endforeach()

# 合并所有源文件
set(ALL_BC_SOURCES
  ${EXPORT_BC_SOURCES}
  ${RUNTIME_BC_SOURCES_ABS}
  ${MEMORY_BC_SOURCES_ABS}
)

# ---------------------------------------------------------------------------
# 构建 include 和 define 参数
# ---------------------------------------------------------------------------
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")  # => src/

set(BC_INCLUDE_FLAGS
  "-I${SRC_DIR}"
  "-I${CMAKE_BINARY_DIR}/src"
)

# ---------------------------------------------------------------------------
# 辅助函数：安全提取 target 的 include 目录
# 过滤掉 generator expression（$<INSTALL_INTERFACE:...> 等）
# ---------------------------------------------------------------------------
function(collect_target_includes target_name out_var)
  set(_result "")
  get_target_property(_dirs ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
  if(_dirs)
    foreach(dir ${_dirs})
      # 跳过 generator expression
      if(dir MATCHES "^\\$<")
        # 仅提取 $<BUILD_INTERFACE:path>
        if(dir MATCHES "\\$<BUILD_INTERFACE:([^>]+)>")
          list(APPEND _result "${CMAKE_MATCH_1}")
        endif()
        # 忽略 $<INSTALL_INTERFACE:...> 和其他 genex
      elseif(NOT dir STREQUAL "")
        list(APPEND _result "${dir}")
      endif()
    endforeach()
  endif()
  set(${out_var} ${_result} PARENT_SCOPE)
endfunction()

# spdlog
if(TARGET spdlog)
  collect_target_includes(spdlog _spdlog_dirs)
  foreach(dir ${_spdlog_dirs})
    list(APPEND BC_INCLUDE_FLAGS "-isystem" "${dir}")
  endforeach()
endif()

# tsl::ordered_map
if(TARGET tsl::ordered_map)
  collect_target_includes(tsl::ordered_map _tsl_dirs)
  foreach(dir ${_tsl_dirs})
    list(APPEND BC_INCLUDE_FLAGS "-isystem" "${dir}")
  endforeach()
endif()

# GMP
if(GMP_INCLUDES)
  list(APPEND BC_INCLUDE_FLAGS "-I${GMP_INCLUDES}")
endif()

# ICU
find_package(ICU QUIET COMPONENTS uc data)
if(ICU_INCLUDE_DIRS)
  foreach(dir ${ICU_INCLUDE_DIRS})
    if(NOT dir STREQUAL "")
      list(APPEND BC_INCLUDE_FLAGS "-isystem" "${dir}")
    endif()
  endforeach()
endif()

set(BC_COMPILE_DEFS "")
if(ENABLE_RUNTIME_ARENA)
  list(APPEND BC_COMPILE_DEFS "-DPYLANG_USE_ARENA")
endif()
list(APPEND BC_COMPILE_DEFS "-DSPDLOG_COMPILED_LIB")

# 打印最终的 include flags 用于调试
message(STATUS "Bitcode include flags: ${BC_INCLUDE_FLAGS}")

# ---------------------------------------------------------------------------
# 为每个 .cpp 生成 .bc
# ---------------------------------------------------------------------------
set(BC_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bitcode")
file(MAKE_DIRECTORY "${BC_OUTPUT_DIR}")

set(ALL_BC_FILES "")

foreach(source_file ${ALL_BC_SOURCES})
  # 计算唯一输出名
  file(RELATIVE_PATH rel_path "${SRC_DIR}" "${source_file}")
  string(REPLACE "/" "_" bc_name "${rel_path}")
  string(REPLACE ".cpp" ".bc" bc_name "${bc_name}")
  set(bc_output "${BC_OUTPUT_DIR}/${bc_name}")

  add_custom_command(
    OUTPUT "${bc_output}"
    COMMAND ${CLANGXX_EXECUTABLE}
      -std=c++20
      -emit-llvm -c
      -O2
      -resource-dir=${CLANG_RESOURCE_DIR}
      ${BC_COMPILE_DEFS}
      ${BC_INCLUDE_FLAGS}
      -Wno-everything
      -o "${bc_output}"
      "${source_file}"
    DEPENDS "${source_file}"
    COMMENT "BC: ${rel_path}"
    VERBATIM
  )

  list(APPEND ALL_BC_FILES "${bc_output}")
endforeach()

# ---------------------------------------------------------------------------
# 合并为 runtime.bc
# ---------------------------------------------------------------------------
set(RUNTIME_BC "${CMAKE_BINARY_DIR}/runtime.bc")

add_custom_command(
  OUTPUT "${RUNTIME_BC}"
  COMMAND ${LLVM_LINK_EXECUTABLE}
    -o "${RUNTIME_BC}"
    ${ALL_BC_FILES}
  DEPENDS ${ALL_BC_FILES}
  COMMENT "Linking runtime.bc"
  VERBATIM
)

add_custom_target(runtime_bitcode ALL
  DEPENDS "${RUNTIME_BC}"
)

set(PYLANG_RUNTIME_BC "${RUNTIME_BC}" CACHE FILEPATH "Path to runtime.bc" FORCE)
message(STATUS "runtime.bc → ${RUNTIME_BC}")