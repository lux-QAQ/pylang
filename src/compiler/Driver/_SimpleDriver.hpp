#pragma once

#include "_SimpleDriver_option.hpp"// [New]
#include "compiler/Codegen/PylangCodegen.hpp"
#include "compiler/RuntimeLinker/RuntimeLinker.hpp"
#include "compiler/Support/Error.hpp"
#include "compiler/Support/LLVMUtils.hpp"
#include "compiler/Support/Log.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/Target/TargetMachine.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace pylang {

class SimpleDriver
{
  public:
	// 类型别名以保持兼容性
	using OutputKind = pylang::OutputKind;
	using Options = pylang::SimpleDriverOptions;

	// =========================================================================
	// 工厂方法
	// =========================================================================
	static Result<SimpleDriver> create(Options opts, llvm::LLVMContext &ctx);

	// =========================================================================
	// 编译入口
	// =========================================================================

	/// 完整管道: AST → 可执行文件/目标文件
	Result<std::filesystem::path> compile_module(ast::Module &module, std::string_view module_name);

	/// 部分管道: AST → 优化后 IR 文本（用于高级测试）
	Result<std::string> compile_to_ir(ast::Module &module, std::string_view module_name);

	/// 部分管道: AST → 未优化 IR 文本（用于 codegen 单元测试）
	Result<std::string> emit_unoptimized_ir(ast::Module &module, std::string_view module_name);

	/// 访问 RuntimeLinker（供测试使用）
	RuntimeLinker &linker() { return m_linker; }
	llvm::LLVMContext &context() { return *m_ctx; }

  private:
	SimpleDriver(Options opts,
		llvm::LLVMContext &ctx,
		LLVMModuleLoader loader,
		RuntimeLinker linker,
		std::unique_ptr<llvm::TargetMachine> tm,
		std::filesystem::path cached_runtime_obj = {});// [Change]

	// =========================================================================
	// 管道阶段（每个阶段返回 Result<T>，可组合）
	// =========================================================================

	/// Stage 1: AST → LLVM Module (未优化, 未链接 runtime)
	Result<std::unique_ptr<llvm::Module>> stage_codegen(ast::Module &module, std::string_view name);

	/// Stage 2: 将 runtime.bc 链接到用户 Module
	Result<std::unique_ptr<llvm::Module>> stage_link_runtime(
		std::unique_ptr<llvm::Module> user_mod);

	/// Stage 3: LLVM 中端优化
	VoidResult stage_optimize(llvm::Module &module);

	/// Stage 4a: 输出 IR 文本
	static Result<std::string> stage_to_ir_string(llvm::Module &module);

	/// Stage 4b: 输出目标文件
	Result<std::filesystem::path> stage_to_object(llvm::Module &module,
		const std::filesystem::path &output);

	/// Stage 4c: 输出 bitcode
	static Result<std::filesystem::path> stage_to_bitcode(llvm::Module &module,
		const std::filesystem::path &output);

	/// Stage 5: 系统链接器 → 可执行文件
	Result<std::filesystem::path> stage_link_executable(const std::filesystem::path &obj_path);

	// =========================================================================
	// 辅助
	// =========================================================================

	/// LLVM 目标初始化（call_once）
	static VoidResult initialize_targets();

	/// 创建宿主机 TargetMachine
	static Result<std::unique_ptr<llvm::TargetMachine>> create_host_target_machine(int opt_level);

	/// 预编译 Runtime 辅助函数
	static Result<std::filesystem::path> precompile_runtime_module(
		const std::filesystem::path &bc_path,
		llvm::LLVMContext &ctx,
		llvm::TargetMachine &tm);

	std::filesystem::path infer_output_path(std::string_view module_name, OutputKind kind) const;

	// =========================================================================
	// 成员
	// =========================================================================
	Options m_opts;
	llvm::LLVMContext *m_ctx;
	LLVMModuleLoader m_loader;
	RuntimeLinker m_linker;
	std::unique_ptr<llvm::TargetMachine> m_target_machine;
	std::filesystem::path m_cached_runtime_obj;// [New]
};

}// namespace pylang