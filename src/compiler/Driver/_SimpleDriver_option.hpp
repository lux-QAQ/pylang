#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace pylang {

enum class OutputKind : uint8_t {
	IRText,// .ll — 人类可读 IR
	Bitcode,// .bc — LLVM bitcode
	ObjectFile,// .o  — 原生目标文件
	Executable,// 二进制可执行文件
};

struct SimpleDriverOptions
{
	std::filesystem::path runtime_bc_path;
	std::filesystem::path output_path;
	OutputKind output_kind = OutputKind::Executable;

	int opt_level = 0;// 0-3

	// [New] 分离编译开关
	// true: 链接 .o 文件 (快，调试/开发默认)
	// false: 合并 IR (慢，但允许跨模块内联优化)
	bool separate_runtime_linking = true;

	bool dump_ir_before_opt = false;
	bool dump_ir_after_opt = false;
	bool verify_module = true;

	std::string linker_cmd = "c++";
	std::vector<std::string> extra_link_flags;
};

}// namespace pylang