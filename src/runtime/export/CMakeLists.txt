# =============================================================================
# Runtime Bitcode Generation — 使用 wllvm
# =============================================================================

find_program(WLLVM_CXX wllvm++)
find_program(EXTRACT_BC extract-bc)
find_program(LLVM_LINK_EXEC llvm-link-20 llvm-link)

if(NOT WLLVM_CXX OR NOT EXTRACT_BC OR NOT LLVM_LINK_EXEC)
  message(STATUS "wllvm++ / extract-bc / llvm-link not found — runtime.bc disabled")
  return()
endif()

message(STATUS "wllvm++ = ${WLLVM_CXX}")
message(STATUS "extract-bc = ${EXTRACT_BC}")
message(STATUS "llvm-link  = ${LLVM_LINK_EXEC}")

# ---------------------------------------------------------------------------
# GCC toolchain（解决 clang 找不到 stdlib.h 的问题）
# ---------------------------------------------------------------------------
execute_process(COMMAND gcc -dumpmachine OUTPUT_VARIABLE GCC_MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND gcc -dumpversion  OUTPUT_VARIABLE GCC_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
set(GCC_INCLUDE_DIR "/usr/lib/gcc/${GCC_MACHINE}/${GCC_VERSION}/include")
message(STATUS "Bitcode GCC include: ${GCC_INCLUDE_DIR}")

# ---------------------------------------------------------------------------
# 收集源文件（这是关键！）
# ---------------------------------------------------------------------------
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")   # => src/

# export 层自身
file(GLOB EXPORT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# runtime 核心 + memory（RUNTIME_SOURCE_FILES / MEMORY_SOURCE_FILES 由父目录定义）
set(ALL_SOURCES ${EXPORT_SOURCES})
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
# Include 路径（-isystem 和路径必须分开传）
# ---------------------------------------------------------------------------
set(BC_INC
  "-I" "${SRC_DIR}"
  "-I" "${CMAKE_BINARY_DIR}/src"
  "--gcc-toolchain=/usr"
  "-isystem" "${GCC_INCLUDE_DIR}"
)

# python-cpp target 的 include 路径（过滤 generator expression）
get_target_property(_pub_inc python-cpp INCLUDE_DIRECTORIES)
if(_pub_inc)
  foreach(dir ${_pub_inc})
    if(NOT dir MATCHES "^\\$<" AND NOT dir STREQUAL "")
      list(APPEND BC_INC "-I" "${dir}")
    endif()
  endforeach()
endif()

# spdlog（CPM 设置的变量）
if(DEFINED spdlog_SOURCE_DIR)
  list(APPEND BC_INC "-isystem" "${spdlog_SOURCE_DIR}/include")
  #message(STATUS "Bitcode spdlog: ${spdlog_SOURCE_DIR}/include")
endif()

# ordered-map（CPM 设置的变量，名字可能不同）
foreach(_var "ordered-map_SOURCE_DIR" "tsl-ordered-map_SOURCE_DIR" "ordered_map_SOURCE_DIR")
  if(DEFINED ${_var})
    list(APPEND BC_INC "-isystem" "${${_var}}/include")
    #message(STATUS "Bitcode ordered-map: ${${_var}}/include")
    break()
  endif()
endforeach()

if(GMP_INCLUDES)
  list(APPEND BC_INC "-I" "${GMP_INCLUDES}")
endif()

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
set(BC_DEFS "-DSPDLOG_COMPILED_LIB")
if(ENABLE_RUNTIME_ARENA)
  list(APPEND BC_DEFS "-DPYLANG_USE_ARENA")
endif()

# ---------------------------------------------------------------------------
# 每个 .cpp → .o → .bc
# ---------------------------------------------------------------------------
set(BC_OBJ_DIR "${CMAKE_BINARY_DIR}/bitcode_obj")
set(BC_DIR     "${CMAKE_BINARY_DIR}/bitcode")
file(MAKE_DIRECTORY "${BC_OBJ_DIR}" "${BC_DIR}")

set(ALL_BC_FILES "")

foreach(src ${ALL_SOURCES})
  file(RELATIVE_PATH rel "${SRC_DIR}" "${src}")
  string(REPLACE "/" "_" name "${rel}")
  string(REPLACE ".cpp" "" name "${name}")

  set(obj "${BC_OBJ_DIR}/${name}.o")
  set(bc  "${BC_DIR}/${name}.bc")

  add_custom_command(
    OUTPUT "${obj}"
    COMMAND ${CMAKE_COMMAND} -E env
      LLVM_COMPILER=clang
      LLVM_COMPILER_PATH=/usr/bin
      ${WLLVM_CXX}
        -std=c++20 -O2 -c
        ${BC_DEFS}
        ${BC_INC}
        -Wno-everything
        -o "${obj}" "${src}"
    DEPENDS "${src}"
    COMMENT "wllvm: ${rel}"
    VERBATIM
  )

  add_custom_command(
    OUTPUT "${bc}"
    COMMAND ${EXTRACT_BC} -o "${bc}" "${obj}"
    DEPENDS "${obj}"
    COMMENT "extract-bc: ${name}.bc"
    VERBATIM
  )

  list(APPEND ALL_BC_FILES "${bc}")
endforeach()

#message(STATUS "Bitcode: ${CMAKE_CURRENT_FUNCTION_LIST_DIR} → ${_count} .bc files to link")

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