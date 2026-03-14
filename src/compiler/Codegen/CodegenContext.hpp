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
// TryContext — try 块的异常处理上下文
// =============================================================================
struct TryContext
{
	/// landingpad BB — invoke 的 unwind 目标
	llvm::BasicBlock *landingpad_bb;

	/// finally BB — 正常路径也要跳到这里
	llvm::BasicBlock *finally_bb;

	/// 状态变量: 0=normal, 1=exception, 2=return, 3=break, 4=continue
	llvm::AllocaInst *state_alloca;

	/// 保存 return value（如果 try/except 中有 return）
	llvm::AllocaInst *retval_alloca;

	/// 保存 landingpad token（用于 resume）
	llvm::AllocaInst *lp_alloca;

	/// 保存捕获的 Python 异常对象
	llvm::AllocaInst *exc_alloca;
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
	/// 查找最近的有 finally 的 try 上下文（向外搜索）
	const TryContext *nearest_finally_try() const
	{
		for (auto it = m_tries.rbegin(); it != m_tries.rend(); ++it) {
			if (it->finally_bb) { return &*it; }
		}
		return nullptr;
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
	// llvm::AllocaInst *create_local(const std::string &name, llvm::Type *type)
	// {
	// 	auto &scope = current_scope();
	// 	ASSERT(scope.llvm_func);

	// 	// 在函数入口 block 创建 alloca (LLVM mem2reg 友好)
	// 	auto &entry_bb = scope.llvm_func->getEntryBlock();
	// 	llvm::IRBuilder<> entry_builder(&entry_bb, entry_bb.begin());
	// 	auto *alloca = entry_builder.CreateAlloca(type, nullptr, name);
	// 	scope.locals[name] = alloca;
	// 	return alloca;
	// }

	llvm::AllocaInst *create_local(const std::string &name, llvm::Type *type)
	{
		auto *func = current_function();
		auto &entry_bb = func->getEntryBlock();

		// 在 entry block 开头创建 alloca
		llvm::IRBuilder<> entry_builder(&entry_bb, entry_bb.begin());
		auto *alloca = entry_builder.CreateAlloca(type, nullptr, name);

		// 初始化: 使用 null/zero
		// 这比 get_none() 更正确：
		// - get_none() 在当前插入点生成 call，跨 BB 会导致 "does not dominate"
		// - null 初始化符合 Python 语义：未赋值变量为 null → runtime 抛 UnboundLocalError
		// - get_none() 初始化会掩盖 UnboundLocalError（错误地返回 None）
		if (type->isPointerTy()) {
			entry_builder.CreateStore(
				llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type)), alloca);
		} else if (type->isIntegerTy()) {
			entry_builder.CreateStore(llvm::ConstantInt::get(type, 0), alloca);
		}

		// 修复: m_scope_stack → m_scopes
		current_scope().locals[name] = alloca;
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


	//  Try 管理
	void push_try(TryContext ctx) { m_tries.push_back(std::move(ctx)); }
	void pop_try() { m_tries.pop_back(); }
	const TryContext *current_try() const { return m_tries.empty() ? nullptr : &m_tries.back(); }
	bool in_try_block() const { return !m_tries.empty(); }

	/// 获取最内层 try 的 landingpad BB（供 invoke 使用）
	llvm::BasicBlock *unwind_dest() const
	{
		return m_tries.empty() ? nullptr : m_tries.back().landingpad_bb;
	}

	/// 收集当前 scope 中所有 CELL 和 FREE 变量名
	/// @param[out] cell_vars  当前函数中需要创建 cell 的变量
	/// @param[out] free_vars  当前函数中需要从闭包提取的变量
    void collect_cell_and_free_vars(VariablesResolver::Scope *scope,
        std::vector<std::string> &cell_vars,
        std::vector<std::string> &free_vars) const
    {
        if (!scope) { return; }

        // 修复: SymbolMap 不是 std::map，需要迭代内部的 .symbols vector
        // Symbol 结构: { std::string name; Visibility visibility; SourceLocation source_location; }
        for (const auto &sym : scope->symbol_map.symbols) {
            if (sym.visibility == Visibility::CELL) {
                cell_vars.push_back(sym.name);
            } else if (sym.visibility == Visibility::FREE) {
                free_vars.push_back(sym.name);
            }
        }

        // 排序确保闭包 tuple 索引确定性（AOT 模式下替代 CPython 的 co_freevars 数组）
        std::sort(cell_vars.begin(), cell_vars.end());
        std::sort(free_vars.begin(), free_vars.end());
    }

  private:
	llvm::IRBuilder<> &m_builder;
	const VisibilityMap &m_visibility;
	std::vector<ScopeContext> m_scopes;
	std::vector<LoopContext> m_loops;
	std::vector<TryContext> m_tries;
};

}// namespace pylang
