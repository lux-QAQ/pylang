# 该脚本在构建时运行，读取二进制文件并生成 .hpp

# 参数:
# OUTPUT_HEADER: 输出头文件路径
# FILES: 文件列表 (格式: 变量名=文件路径)

# 获取文件名和变量名
set(CONTENT "")
set(INCLUDES "")

list(LENGTH FILES len)
math(EXPR loop_len "${len} - 1")

foreach(item ${FILES})
    # 解析 "VAR_NAME=FILE_PATH"
    string(FIND "${item}" "=" eq_idx)
    string(SUBSTRING "${item}" 0 ${eq_idx} VAR_NAME)
    math(EXPR file_start "${eq_idx} + 1")
    string(SUBSTRING "${item}" ${file_start} -1 FILE_PATH)

    if(NOT EXISTS "${FILE_PATH}")
        message(FATAL_ERROR "File to embed not found: ${FILE_PATH}")
    endif()

    # 读取文件为 HEX 字符串
    file(READ "${FILE_PATH}" HEX_DATA HEX)
    
    # 将 HEX 转换为 C 数组格式 (0x.., 0x.., ...)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," ARRAY_DATA "${HEX_DATA}")
    
    # 获取文件大小
    file(SIZE "${FILE_PATH}" FILE_SIZE)

    # 追加到头文件内容
    string(APPEND CONTENT "
// Source: ${FILE_PATH}
static const unsigned char ${VAR_NAME}_data[] = {
${ARRAY_DATA}
};
static const std::size_t ${VAR_NAME}_size = ${FILE_SIZE};
static const std::string_view ${VAR_NAME}() {
    return {reinterpret_cast<const char*>(${VAR_NAME}_data), ${VAR_NAME}_size};
}
")
endforeach()

# 写入最终文件
file(WRITE "${OUTPUT_HEADER}" "#pragma once\n#include <string_view>\n#include <cstdint>\nnamespace embedded {\n${CONTENT}\n}\n")