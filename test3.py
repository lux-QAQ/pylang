# 1. 字面量格式
# 0b1101 = 13, 0x1A = 26, 1.5e2 = 150.0
# mix_list = [0b1101, 0x1A, 1.5e2]
# print("literals", mix_list[0], mix_list[1], mix_list[2])


# 3. 字节与原始字符串
data = b"hello"
raw = r"\n"
print("bytes_len", len(data))
print("raw_check", raw == "\\n")
print("hex_check", 0x10 == 16)
print("is_none", None is None)

# 4. 字典推导式 (如果 Codegen 已支持)
squares = {x: x**2 for x in [1, 2, 3]}
print("dict_comp", squares[2], squares[3])
