#pragma once

#include "RuntimeLinker_option.hpp"
#include "compiler/Support/Error.hpp"
#include "compiler/Support/LLVMUtils.hpp"
#include "compiler/Support/Log.hpp"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pylang {

// =============================================================================
// RuntimeFunction
// =============================================================================
struct RuntimeFunction
{
	std::string category;
	std::string name;
	std::string return_type;
	std::string param_types;

	llvm::Function *llvm_func = nullptr;
	llvm::FunctionType *llvm_func_type = nullptr;

	[[nodiscard]] std::vector<std::string_view> params() const;
};

// =============================================================================
// RuntimeLinker
// =============================================================================
class RuntimeLinker
{
  public:
	static Result<RuntimeLinker> create(LLVMModuleLoader &loader,
		const std::filesystem::path &bitcode_path);

	static Result<RuntimeLinker> create_from_memory(LLVMModuleLoader &loader,
		std::string_view bc_data);

	Result<const RuntimeFunction *> get_function(std::string_view name) const;

	std::vector<const RuntimeFunction *> list_by_category(std::string_view category) const;

	/// 在用户 Module 中声明函数（lazy）
	llvm::Function *declare_in(llvm::Module *user_module, std::string_view name);

	llvm::Type *pyobject_ptr_type() const;
	llvm::Module *runtime_module() const { return m_runtime_module.get(); }
	size_t function_count() const { return m_functions.size(); }
	void print_registry() const;
	VoidResult link_into(llvm::Module *user_module);

	// ========== 缓存管理 ==========

	/// 获取当前选项（可修改）
	RuntimeLinkerOptions &options() { return m_options; }

	/// 清除特定模块的缓存（当 Module 析构时调用）
	void forget_module(llvm::Module *module);

	/// 清空所有缓存（测试 TearDown 时调用）
	void clear_cache();

  private:
	explicit RuntimeLinker(std::unique_ptr<llvm::Module> mod);

	RuntimeLinker() = default;
	VoidResult scan_annotations();
	Result<RuntimeFunction> parse_annotation(std::string_view annotation_str, llvm::Function *func);

	std::unique_ptr<llvm::Module> m_runtime_module; 
	std::unordered_map<std::string, RuntimeFunction> m_functions;

	RuntimeLinkerOptions m_options;

	// 缓存: Module* -> (Logical Name -> FunctionDecl*)
	// 使用 Module* 作为一级 Key 隔离不同模块，避免逻辑名冲突
	std::unordered_map<llvm::Module *, std::unordered_map<std::string, llvm::Function *>> m_cache;
};
;

}// namespace pylang