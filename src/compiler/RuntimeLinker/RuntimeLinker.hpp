#pragma once

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

	Result<const RuntimeFunction *> get_function(std::string_view name) const;

	std::vector<const RuntimeFunction *> list_by_category(std::string_view category) const;

	/// 在用户 Module 中声明函数（lazy）
	/// 返回 nullptr 表示函数不存在（调用者应检查）
	llvm::Function *declare_in(llvm::Module *user_module, std::string_view name);

	/// 获取 PyObject* 的 LLVM 类型（opaque pointer）
	llvm::Type *pyobject_ptr_type() const;

	llvm::Module *runtime_module() const { return m_runtime_module; }

	size_t function_count() const { return m_functions.size(); }

	/// 诊断：列出所有已注册函数（按 category 分组）
	void print_registry() const;

	/// 将 runtime module 链接到用户 module（会克隆 runtime module）
    VoidResult link_into(llvm::Module *user_module);

  private:
	RuntimeLinker() = default;

	VoidResult scan_annotations();

	Result<RuntimeFunction> parse_annotation(std::string_view annotation_str, llvm::Function *func);

	llvm::Module *m_runtime_module = nullptr;
	std::unordered_map<std::string, RuntimeFunction> m_functions;
	std::unordered_map<std::string, llvm::Function *> m_declared;
};

}// namespace pylang