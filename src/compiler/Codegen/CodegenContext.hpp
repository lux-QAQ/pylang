#pragma once

// =============================================================================
// CodegenContext — 编译器代码生成上下文
//
// 管理:
//   - 变量: 局部变量的 alloca、全局变量经模块对象访问
//   - 作用域: scope 栈（模块/函数/类），RAII 管理进出
//   - 控制流: break/continue 点的标签栈
//   - 当前函数: 正在编译的 LLVM Function 和模块对象
//
// 设计原则:
//   - 不依赖 AST 类型（只用 std::string 变量名和 Visibility 枚举）
//   - 不依赖 IREmitter（纯数据结构 + LLVM 基础 IR）
//   - 可独立测试
// =============================================================================

#include "executable/bytecode/codegen/VariablesResolver.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace pylang {

using Visibility = VariablesResolver::Visibility;
using VisibilityMap = VariablesResolver::VisibilityMap;

// =============================================================================
// LoopContext — break/continue 的目标 BB
// =============================================================================
struct LoopContext
{
	llvm::BasicBlock *break_bb;
	llvm::BasicBlock *continue_bb;
};

// =============================================================================
// ScopeContext — 单层作用域
// =============================================================================
struct ScopeContext
{
	enum class Type { MODULE, FUNCTION, CLASS };

	Type type;
	std::string name;
	std::string mangled_name;

	/// 局部变量 → alloca 指针 (LOCAL visibility)
	std::unordered_map<std::string, llvm::AllocaInst *> locals;

	/// cell 变量 → cell 对象的 alloca 指针 (CELL visibility)
	std::unordered_map<std::string, llvm::AllocaInst *> cells;

	/// free 变量 → cell 对象的 alloca 指针 (FREE visibility)
	std::unordered_map<std::string, llvm::AllocaInst *> free_vars;

	/// 当前函数的 LLVM Function（MODULE scope 中为模块初始化函数）
	llvm::Function *llvm_func = nullptr;

	/// 当前模块的 PyModule* 值（rt_import 返回的模块对象）
	llvm::Value *module_obj = nullptr;

	/// VariablesResolver 的 Scope（用于查询变量 Visibility）
	VariablesResolver::Scope *var_scope = nullptr;

	/// CLASS scope 专用: namespace dict 的 LLVM Value*
	/// 类体中 NAME visibility 的变量通过 dict_setitem_str/dict_getitem_str 存取
	/// 非 CLASS scope 为 nullptr
	llvm::Value *class_namespace = nullptr;
};

// =============================================================================
// CodegenContext — 作用域栈 + 辅助查询
// =============================================================================
class CodegenContext
{
  public:
	CodegenContext(llvm::IRBuilder<> &builder, const VisibilityMap &visibility)
		: m_builder(builder), m_visibility(visibility)
	{}

	//  作用域管理 

	/// 进入新作用域
	void push_scope(ScopeContext scope) { m_scopes.push_back(std::move(scope)); }

	/// 离开当前作用域
	void pop_scope()
	{
		ASSERT(!m_scopes.empty());
		m_scopes.pop_back();
	}

	/// 当前作用域
	ScopeContext &current_scope()
	{
		ASSERT(!m_scopes.empty());
		return m_scopes.back();
	}

	const ScopeContext &current_scope() const
	{
		ASSERT(!m_scopes.empty());
		return m_scopes.back();
	}

	/// 最近的函数作用域（跳过 CLASS）
	ScopeContext *enclosing_function_scope()
	{
		for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
			if (it->type == ScopeContext::Type::FUNCTION
				|| it->type == ScopeContext::Type::MODULE) {
				return &*it;
			}
		}
		return nullptr;
	}

	bool in_module_scope() const { return m_scopes.size() == 1; }

	//  变量可见性查询 

	/// 查找变量的 Visibility（从 VariablesResolver 的结果）
	std::optional<Visibility> get_visibility(const std::string &var_name) const
	{
		auto *scope = current_scope().var_scope;
		if (!scope) { return std::nullopt; }

		if (auto sym = scope->symbol_map.get_visible_symbol(var_name)) {
			return sym->get().visibility;
		}
		return std::nullopt;
	}

	//  局部变量管理 

	/// 为局部变量创建 alloca（在函数入口 block 中）
	llvm::AllocaInst *create_local(const std::string &name, llvm::Type *type)
	{
		auto &scope = current_scope();
		ASSERT(scope.llvm_func);

		// 在函数入口 block 创建 alloca (LLVM mem2reg 友好)
		auto &entry_bb = scope.llvm_func->getEntryBlock();
		llvm::IRBuilder<> entry_builder(&entry_bb, entry_bb.begin());
		auto *alloca = entry_builder.CreateAlloca(type, nullptr, name);
		scope.locals[name] = alloca;
		return alloca;
	}

	/// 查找局部变量的 alloca
	llvm::AllocaInst *find_local(const std::string &name) const
	{
		for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
			if (auto found = it->locals.find(name); found != it->locals.end()) {
				return found->second;
			}
			// 不跨函数边界查找 LOCAL
			if (it->type == ScopeContext::Type::FUNCTION) { break; }
		}
		return nullptr;
	}

	/// 注册 cell 变量
	void register_cell(const std::string &name, llvm::AllocaInst *alloca)
	{
		current_scope().cells[name] = alloca;
	}

	/// 查找 cell 变量
	llvm::AllocaInst *find_cell(const std::string &name) const
	{
		auto &scope = current_scope();
		if (auto found = scope.cells.find(name); found != scope.cells.end()) {
			return found->second;
		}
		return nullptr;
	}

	/// 注册 free 变量
	void register_free_var(const std::string &name, llvm::AllocaInst *alloca)
	{
		current_scope().free_vars[name] = alloca;
	}

	/// 查找 free 变量
	llvm::AllocaInst *find_free_var(const std::string &name) const
	{
		auto &scope = current_scope();
		if (auto found = scope.free_vars.find(name); found != scope.free_vars.end()) {
			return found->second;
		}
		return nullptr;
	}

	//  循环管理 

	void push_loop(LoopContext loop) { m_loops.push_back(loop); }
	void pop_loop() { m_loops.pop_back(); }

	const LoopContext *current_loop() const { return m_loops.empty() ? nullptr : &m_loops.back(); }

	//  模块对象 

	llvm::Value *module_object() const
	{
		// 从栈底（模块 scope）获取模块对象
		for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
			if (it->module_obj) { return it->module_obj; }
		}
		return nullptr;
	}

	/// 当前正在编译的 LLVM Function
	llvm::Function *current_function() const { return current_scope().llvm_func; }

	/// VisibilityMap 引用（供 PylangCodegen 传给嵌套函数编译）
	const VisibilityMap &visibility_map() const { return m_visibility; }

  private:
	llvm::IRBuilder<> &m_builder;
	const VisibilityMap &m_visibility;
	std::vector<ScopeContext> m_scopes;
	std::vector<LoopContext> m_loops;
};

}// namespace pylang
