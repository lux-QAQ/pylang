#include "PylangCodegen.hpp"

#include "compiler/Support/Log.hpp"
#include "compiler/Support/ScopeTimer.hpp"

#include "runtime/Value.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>

namespace pylang {

// =============================================================================
// PylangCodegen 构造 / compile 入口
// =============================================================================

PylangCodegen::PylangCodegen(llvm::LLVMContext &ctx,
	RuntimeLinker &linker,
	std::unique_ptr<llvm::Module> module,
	const VisibilityMap &visibility,
	std::string module_name)
	: m_ctx(ctx), m_module(std::move(module)), m_builder(ctx),
	  m_emitter(m_builder, linker, m_module.get()), m_codegen_ctx(m_builder, visibility),
	  m_module_name(std::move(module_name)), m_mangler(Mangler::default_mangler())
{}

Result<PylangCodegen::CompileResult> PylangCodegen::compile(ast::Module *module_node,
	std::string_view module_name,
	llvm::LLVMContext &ctx,
	RuntimeLinker &linker)
{
	PYLANG_TIMER("PylangCodegen::compile");

	// Step 1: 变量作用域分析
	auto visibility = VariablesResolver::resolve(module_node);

	// Step 2: 创建 LLVM Module
	auto llvm_module = std::make_unique<llvm::Module>(module_name, ctx);

	// Step 3: 创建 codegen 实例
	PylangCodegen codegen(
		ctx, linker, std::move(llvm_module), visibility, std::string(module_name));

	// Step 4: 创建模块初始化函数
	auto *init_func = codegen.create_module_init_function();

	// Step 5: 设置 VariablesResolver scope
	auto mangled = codegen.m_mangler.function_mangle(
		std::string(module_name), "<module>", module_node->source_location());
	auto it = visibility.find(mangled);
	VariablesResolver::Scope *var_scope = (it != visibility.end()) ? it->second.get() : nullptr;

	// Step 6: 进入模块 scope
	ScopeContext module_scope{};
	module_scope.type = ScopeContext::Type::MODULE;
	module_scope.name = std::string(module_name);
	module_scope.mangled_name = mangled;
	module_scope.llvm_func = init_func;
	module_scope.var_scope = var_scope;
	codegen.m_codegen_ctx.push_scope(std::move(module_scope));

	// Step 7: 生成模块初始化函数体
	//   %module = call ptr @rt_import("__main__", ...)
	auto *mod = codegen.m_emitter.call_import(module_name);
	codegen.m_codegen_ctx.current_scope().module_obj = mod;

	// Step 8: 遍历 AST，生成 IR
	codegen.generate_body(module_node->body());

	// Step 9: 模块初始化函数返回 void
	codegen.m_builder.CreateRetVoid();

	// Step 10: 离开模块 scope
	codegen.m_codegen_ctx.pop_scope();

	// Step 11: 创建 main()
	codegen.create_main_function(init_func);

	// Step 12: 验证模块
	std::string verify_err;
	llvm::raw_string_ostream verify_os(verify_err);
	if (llvm::verifyModule(*codegen.m_module, &verify_os)) {
		return MAKE_ERROR(
			ErrorKind::CodegenInternalError, "LLVM Module verification failed: {}", verify_err);
	}

	log::codegen()->info("Module '{}' compiled successfully", module_name);
	return CompileResult{ std::move(codegen.m_module) };
}

// =============================================================================
// 辅助方法
// =============================================================================

/// 为未实现节点生成 abort 调用
llvm::Value *PylangCodegen::emit_not_implemented(const char *feature)
{
	log::codegen()->warn("{} not yet implemented — will abort at runtime", feature);
	auto *msg = m_emitter.create_string(
		std::string("NotImplementedError: ") + feature + " is not supported in AOT mode");
	m_emitter.call_raise(msg);
	m_builder.CreateUnreachable();

	// 创建一个新的 BB 让后续代码有插入点（不可达但避免 LLVM 报错）
	auto *dead_bb =
		llvm::BasicBlock::Create(m_ctx, "unreachable", m_codegen_ctx.current_function());
	m_builder.SetInsertPoint(dead_bb);
	return m_emitter.get_none();
}
llvm::Value *PylangCodegen::generate(const ast::ASTNode *node)
{
	auto *result = node->codegen(this);
	if (!result) { return nullptr; }
	return static_cast<LLVMValue *>(result)->llvm_value();
}

void PylangCodegen::generate_body(const std::vector<std::shared_ptr<ast::ASTNode>> &body)
{
	for (const auto &stmt : body) {
		// 如果当前 basic block 已经有 terminator（如 return/break），停止生成
		if (m_builder.GetInsertBlock()->getTerminator()) { break; }
		generate(stmt.get());
	}
}

llvm::Function *PylangCodegen::create_module_init_function()
{
	auto *func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(m_ctx), false);
	auto name = "PyInit_" + m_module_name;
	auto *func =
		llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, m_module.get());
	auto *entry = llvm::BasicBlock::Create(m_ctx, "entry", func);
	m_builder.SetInsertPoint(entry);
	return func;
}

llvm::Function *PylangCodegen::create_main_function(llvm::Function *module_init)
{
	// int main(i32 argc, ptr argv)
	auto *i32_ty = llvm::Type::getInt32Ty(m_ctx);
	auto *ptr_ty = llvm::PointerType::getUnqual(m_ctx);
	auto *main_ty = llvm::FunctionType::get(i32_ty, { i32_ty, ptr_ty }, false);
	auto *main_func =
		llvm::Function::Create(main_ty, llvm::Function::ExternalLinkage, "main", m_module.get());

	auto *entry = llvm::BasicBlock::Create(m_ctx, "entry", main_func);
	m_builder.SetInsertPoint(entry);

	// call void @rt_init()
	m_emitter.emit_init();

	// call void @PyInit_<module>()
	m_builder.CreateCall(module_init);

	// call void @rt_shutdown()
	m_emitter.emit_shutdown();

	// ret i32 0
	m_builder.CreateRet(llvm::ConstantInt::get(i32_ty, 0));

	return main_func;
}

LLVMValue *PylangCodegen::make_value(llvm::Value *val, const std::string &name)
{
	auto v = std::make_unique<LLVMValue>(name, val);
	auto *ptr = v.get();
	m_values.push_back(std::move(v));
	return ptr;
}

// =============================================================================
// 变量操作
// =============================================================================

llvm::Value *PylangCodegen::load_variable(const std::string &name)
{
	// auto vis = m_codegen_ctx.get_visibility(name);
	// auto visibility = vis.value_or(Visibility::IMPLICIT_GLOBAL);
	auto visibility_opt = m_codegen_ctx.get_visibility(name);

	// var_scope 为空时（内层函数未解析 visibility）
	if (!visibility_opt) {
		if (auto *alloca = m_codegen_ctx.find_local(name)) {
			return m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), alloca, name);
		}
		if (m_codegen_ctx.current_scope().class_namespace) {
			return m_emitter.call_dict_getitem_str(
				m_codegen_ctx.current_scope().class_namespace, name);
		}
		return m_emitter.call_load_global(m_codegen_ctx.module_object(), name);
	}

	auto visibility = *visibility_opt;
	switch (visibility) {
	case Visibility::LOCAL: {
		auto *alloca = m_codegen_ctx.find_local(name);
		if (!alloca) {
			log::codegen()->error("LOCAL variable '{}' not found in scope", name);
			return m_emitter.get_none();
		}
		return m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), alloca, name);
	}
	case Visibility::CELL: {
		auto *cell_alloca = m_codegen_ctx.find_cell(name);
		if (!cell_alloca) {
			log::codegen()->error("CELL variable '{}' not found", name);
			return m_emitter.get_none();
		}
		auto *cell =
			m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), cell_alloca, name + ".cell");
		return m_emitter.call_cell_get(cell);
	}
	case Visibility::FREE: {
		auto *free_alloca = m_codegen_ctx.find_free_var(name);
		if (!free_alloca) {
			log::codegen()->error("FREE variable '{}' not found", name);
			return m_emitter.get_none();
		}
		auto *cell =
			m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), free_alloca, name + ".free");
		return m_emitter.call_cell_get(cell);
	}
	case Visibility::IMPLICIT_GLOBAL:
	case Visibility::EXPLICIT_GLOBAL: {
		auto *mod = m_codegen_ctx.module_object();
		return m_emitter.call_load_global(mod, name);
	}
	case Visibility::NAME: {
		// 类作用域: 从 namespace dict 读取
		if (m_codegen_ctx.current_scope().class_namespace) {
			return m_emitter.call_dict_getitem_str(
				m_codegen_ctx.current_scope().class_namespace, name);
		}
		// 模块作用域: 等同 GLOBAL
		return m_emitter.call_load_global(m_codegen_ctx.module_object(), name);
	}
	case Visibility::HIDDEN: {
		// Phase 2: 简化处理，当作全局
		auto *mod = m_codegen_ctx.module_object();
		return m_emitter.call_load_global(mod, name);
	}
	}
	return m_emitter.get_none();
}

void PylangCodegen::store_variable(const std::string &name, llvm::Value *value)
{
	// auto vis = m_codegen_ctx.get_visibility(name);
	// auto visibility = vis.value_or(Visibility::IMPLICIT_GLOBAL);
	auto visibility_opt = m_codegen_ctx.get_visibility(name);

	if (!visibility_opt) {
		if (auto *alloca = m_codegen_ctx.find_local(name)) {
			m_builder.CreateStore(value, alloca);
			return;
		}
		if (m_codegen_ctx.current_scope().class_namespace) {
			m_emitter.call_dict_setitem_str(
				m_codegen_ctx.current_scope().class_namespace, name, value);
			return;
		}
		m_emitter.call_store_global(m_codegen_ctx.module_object(), name, value);
		return;
	}

	auto visibility = *visibility_opt;
	switch (visibility) {
	case Visibility::LOCAL: {
		auto *alloca = m_codegen_ctx.find_local(name);
		if (!alloca) {
			// 在函数入口创建 alloca
			alloca = m_codegen_ctx.create_local(name, m_emitter.pyobject_ptr_type());
		}
		m_builder.CreateStore(value, alloca);
		break;
	}
	case Visibility::CELL: {
		auto *cell_alloca = m_codegen_ctx.find_cell(name);
		if (!cell_alloca) {
			log::codegen()->error("CELL variable '{}' not found for store", name);
			return;
		}
		auto *cell =
			m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), cell_alloca, name + ".cell");
		m_emitter.call_cell_set(cell, value);
		break;
	}
	case Visibility::FREE: {
		auto *free_alloca = m_codegen_ctx.find_free_var(name);
		if (!free_alloca) {
			log::codegen()->error("FREE variable '{}' not found for store", name);
			return;
		}
		auto *cell =
			m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), free_alloca, name + ".free");
		m_emitter.call_cell_set(cell, value);
		break;
	}
	case Visibility::IMPLICIT_GLOBAL:
	case Visibility::EXPLICIT_GLOBAL: {
		auto *mod = m_codegen_ctx.module_object();
		m_emitter.call_store_global(mod, name, value);
		break;
	}
	case Visibility::NAME: {
		// 类作用域: 写入 namespace dict
		if (m_codegen_ctx.current_scope().class_namespace) {
			m_emitter.call_dict_setitem_str(
				m_codegen_ctx.current_scope().class_namespace, name, value);
			return;
		}
		// 模块作用域: 等同 GLOBAL
		m_emitter.call_store_global(m_codegen_ctx.module_object(), name, value);
		return;
	}
	case Visibility::HIDDEN: {
		auto *mod = m_codegen_ctx.module_object();
		m_emitter.call_store_global(mod, name, value);
		break;
	}
	}
}

void PylangCodegen::delete_variable(const std::string &name)
{
	auto vis = m_codegen_ctx.get_visibility(name);
	auto visibility = vis.value_or(Visibility::IMPLICIT_GLOBAL);

	switch (visibility) {
	case Visibility::LOCAL: {
		// store null 表示变量已删除
		auto *alloca = m_codegen_ctx.find_local(name);
		if (alloca) {
			auto *null_val = llvm::ConstantPointerNull::get(
				llvm::cast<llvm::PointerType>(m_emitter.pyobject_ptr_type()));
			m_builder.CreateStore(null_val, alloca);
		}
		break;
	}
	case Visibility::IMPLICIT_GLOBAL:
	case Visibility::EXPLICIT_GLOBAL:
	case Visibility::NAME:
	case Visibility::HIDDEN: {
		auto *mod = m_codegen_ctx.module_object();
		m_emitter.call_delattr(mod, name);
		break;
	}
	default:
		log::codegen()->warn("delete not implemented for visibility type");
		break;
	}
}

void PylangCodegen::generate_store_target(const ast::ASTNode *target, llvm::Value *value)
{
	switch (target->node_type()) {
	case ast::ASTNodeType::Name: {
		auto *name = static_cast<const ast::Name *>(target);
		store_variable(name->ids()[0], value);
		break;
	}
	case ast::ASTNodeType::Tuple:
	case ast::ASTNodeType::List: {
		// 解包赋值: a, b = expr
		const std::vector<std::shared_ptr<ast::ASTNode>> *elements = nullptr;
		if (target->node_type() == ast::ASTNodeType::Tuple) {
			elements = &static_cast<const ast::Tuple *>(target)->elements();
		} else {
			elements = &static_cast<const ast::List *>(target)->elements();
		}
		auto count = static_cast<int32_t>(elements->size());

		// alloca 输出数组
		auto *ptr_ty = m_emitter.pyobject_ptr_type();
		auto *arr_ty = llvm::ArrayType::get(ptr_ty, count);
		auto *out_arr = m_builder.CreateAlloca(arr_ty, nullptr, "unpack_arr");

		m_emitter.call_unpack_sequence(
			value, count, m_builder.CreateBitCast(out_arr, llvm::PointerType::getUnqual(m_ctx)));

		for (int32_t i = 0; i < count; ++i) {
			auto *gep = m_builder.CreateConstGEP2_32(arr_ty, out_arr, 0, i);
			auto *elem = m_builder.CreateLoad(ptr_ty, gep);
			generate_store_target((*elements)[i].get(), elem);
		}
		break;
	}
	case ast::ASTNodeType::Subscript: {
		auto *sub = static_cast<const ast::Subscript *>(target);
		auto *obj = generate(sub->value().get());
		// 处理 Index slice
		if (std::holds_alternative<ast::Subscript::Index>(sub->slice())) {
			auto &idx = std::get<ast::Subscript::Index>(sub->slice());
			auto *key = generate(idx.value.get());
			m_emitter.call_setitem(obj, key, value);
		}
		break;
	}
	case ast::ASTNodeType::Attribute: {
		auto *attr = static_cast<const ast::Attribute *>(target);
		auto *obj = generate(attr->value().get());
		m_emitter.call_setattr(obj, attr->attr(), value);
		break;
	}
	default:
		log::codegen()->error(
			"Unsupported store target: {}", ast::node_type_to_string(target->node_type()));
		break;
	}
}

// =============================================================================
// visit() 实现 — 表达式
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Constant *node)
{
	auto *val = node->value();
	llvm::Value *result = nullptr;

	std::visit(overloaded{
				   [&](const py::Number &num) {
					   std::visit(overloaded{
									  [&](double d) { result = m_emitter.create_float(d); },
									  [&](const py::BigIntType &big) {
										  if (big.fits_slong_p()) {
											  result = m_emitter.create_integer(big.get_si());
										  } else {
											  // 大整数: 通过字符串创建
											  auto s = big.get_str();
											  auto *str = m_emitter.create_string(s);
											  // TODO: Phase 3+ 处理大整数
											  result = str;
										  }
									  },
								  },
						   num.value);
				   },
				   [&](const py::String &str) { result = m_emitter.create_string(str.s); },
				   [&](const py::Bytes &bytes) {
					   std::string_view data(
						   reinterpret_cast<const char *>(bytes.b.data()), bytes.b.size());
					   result = m_emitter.create_bytes(data);
				   },
				   [&](const py::Ellipsis &) { result = m_emitter.get_ellipsis(); },
				   [&](const py::NoneType &) { result = m_emitter.get_none(); },
				   [&](const py::NameConstant &nc) {
					   std::visit(overloaded{
									  [&](bool b) {
										  result = b ? m_emitter.get_true() : m_emitter.get_false();
									  },
									  [&](const py::NoneType &) { result = m_emitter.get_none(); },
								  },
						   nc.value);
				   },
				   [&](const py::Tuple &) {
					   // 常量 tuple: Phase 3+
					   result = m_emitter.get_none();
				   },
				   [&](const py::PyObject *) { result = m_emitter.get_none(); },
			   },
		*val);

	return make_value(result);
}

ast::Value *PylangCodegen::visit(const ast::Name *node)
{
	if (node->context_type() == ast::ContextType::LOAD) {
		return make_value(load_variable(node->ids()[0]), node->ids()[0]);
	}
	if (node->context_type() == ast::ContextType::DELETE) {
		delete_variable(node->ids()[0]);
		return nullptr;
	}
	// STORE context is handled by Assign
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::BinaryExpr *node)
{
	auto *lhs = generate(node->lhs().get());
	auto *rhs = generate(node->rhs().get());
	if (!lhs || !rhs) { return nullptr; }

	llvm::Value *result = nullptr;
	switch (node->op_type()) {
	case ast::BinaryOpType::PLUS:
		result = m_emitter.call_binary_add(lhs, rhs);
		break;
	case ast::BinaryOpType::MINUS:
		result = m_emitter.call_binary_sub(lhs, rhs);
		break;
	case ast::BinaryOpType::MULTIPLY:
		result = m_emitter.call_binary_mul(lhs, rhs);
		break;
	case ast::BinaryOpType::SLASH:
		result = m_emitter.call_binary_truediv(lhs, rhs);
		break;
	case ast::BinaryOpType::FLOORDIV:
		result = m_emitter.call_binary_floordiv(lhs, rhs);
		break;
	case ast::BinaryOpType::MODULO:
		result = m_emitter.call_binary_mod(lhs, rhs);
		break;
	case ast::BinaryOpType::EXP:
		result = m_emitter.call_binary_pow(lhs, rhs);
		break;
	case ast::BinaryOpType::LEFTSHIFT:
		result = m_emitter.call_binary_lshift(lhs, rhs);
		break;
	case ast::BinaryOpType::RIGHTSHIFT:
		result = m_emitter.call_binary_rshift(lhs, rhs);
		break;
	case ast::BinaryOpType::AND:
		result = m_emitter.call_binary_and(lhs, rhs);
		break;
	case ast::BinaryOpType::OR:
		result = m_emitter.call_binary_or(lhs, rhs);
		break;
	case ast::BinaryOpType::XOR:
		result = m_emitter.call_binary_xor(lhs, rhs);
		break;
	case ast::BinaryOpType::MATMUL:
		// Phase 3+
		log::codegen()->warn("matmul not implemented");
		result = m_emitter.get_none();
		break;
	}
	return make_value(result);
}

ast::Value *PylangCodegen::visit(const ast::UnaryExpr *node)
{
	auto *operand = generate(node->operand().get());
	if (!operand) { return nullptr; }

	llvm::Value *result = nullptr;
	switch (node->op_type()) {
	case ast::UnaryOpType::SUB:
		result = m_emitter.call_unary_neg(operand);
		break;
	case ast::UnaryOpType::ADD:
		result = m_emitter.call_unary_pos(operand);
		break;
	case ast::UnaryOpType::NOT:
		result = m_emitter.call_unary_not(operand);
		break;
	case ast::UnaryOpType::INVERT:
		result = m_emitter.call_unary_invert(operand);
		break;
	}
	return make_value(result);
}

ast::Value *PylangCodegen::visit(const ast::Compare *node)
{
	// Python 3.9: a < f() < c
	//   → tmp = f(); (a < tmp) and (tmp < c)
	// 中间值只求值一次！

	auto *lhs = generate(node->lhs().get());
	if (!lhs) { return nullptr; }

	const auto &ops = node->ops();
	const auto &comparators = node->comparators();

	if (ops.empty()) { return make_value(lhs); }

	// 单比较: a < b （最常见情况，无需临时变量）
	if (ops.size() == 1) {
		auto *rhs = generate(comparators[0].get());
		if (!rhs) { return nullptr; }

		llvm::Value *result = nullptr;
		switch (ops[0]) {
		case ast::Compare::OpType::Eq:
			result = m_emitter.call_compare_eq(lhs, rhs);
			break;
		case ast::Compare::OpType::NotEq:
			result = m_emitter.call_compare_ne(lhs, rhs);
			break;
		case ast::Compare::OpType::Lt:
			result = m_emitter.call_compare_lt(lhs, rhs);
			break;
		case ast::Compare::OpType::LtE:
			result = m_emitter.call_compare_le(lhs, rhs);
			break;
		case ast::Compare::OpType::Gt:
			result = m_emitter.call_compare_gt(lhs, rhs);
			break;
		case ast::Compare::OpType::GtE:
			result = m_emitter.call_compare_ge(lhs, rhs);
			break;
		case ast::Compare::OpType::Is:
			result = m_emitter.call_compare_is(lhs, rhs);
			break;
		case ast::Compare::OpType::IsNot:
			result = m_emitter.call_compare_is_not(lhs, rhs);
			break;
		case ast::Compare::OpType::In:
			result = m_emitter.call_compare_in(lhs, rhs);
			break;
		case ast::Compare::OpType::NotIn:
			result = m_emitter.call_compare_not_in(lhs, rhs);
			break;
		}
		return make_value(result);
	}

	// 链式比较: a < b < c < d
	// 编译为: (a < b) and (b < c) and (c < d)
	// 关键: b, c 只求值一次
	auto *func = m_codegen_ctx.current_function();

	// alloca 保存最终结果
	auto *result_alloca = m_codegen_ctx.create_local(".cmp_result", m_emitter.pyobject_ptr_type());
	m_builder.CreateStore(m_emitter.get_true(), result_alloca);

	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "cmp.end", func);

	llvm::Value *prev = lhs;

	for (size_t i = 0; i < ops.size(); ++i) {
		// 求值右操作数（只求值一次，保存到临时变量）
		auto *rhs = generate(comparators[i].get());
		if (!rhs) { return nullptr; }

		// 执行比较
		llvm::Value *cmp_result = nullptr;
		switch (ops[i]) {
		case ast::Compare::OpType::Eq:
			cmp_result = m_emitter.call_compare_eq(prev, rhs);
			break;
		case ast::Compare::OpType::NotEq:
			cmp_result = m_emitter.call_compare_ne(prev, rhs);
			break;
		case ast::Compare::OpType::Lt:
			cmp_result = m_emitter.call_compare_lt(prev, rhs);
			break;
		case ast::Compare::OpType::LtE:
			cmp_result = m_emitter.call_compare_le(prev, rhs);
			break;
		case ast::Compare::OpType::Gt:
			cmp_result = m_emitter.call_compare_gt(prev, rhs);
			break;
		case ast::Compare::OpType::GtE:
			cmp_result = m_emitter.call_compare_ge(prev, rhs);
			break;
		case ast::Compare::OpType::Is:
			cmp_result = m_emitter.call_compare_is(prev, rhs);
			break;
		case ast::Compare::OpType::IsNot:
			cmp_result = m_emitter.call_compare_is_not(prev, rhs);
			break;
		case ast::Compare::OpType::In:
			cmp_result = m_emitter.call_compare_in(prev, rhs);
			break;
		case ast::Compare::OpType::NotIn:
			cmp_result = m_emitter.call_compare_not_in(prev, rhs);
			break;
		}

		// 短路求值: 如果这个比较为 false，跳到结束
		if (i < ops.size() - 1) {
			auto *is_true = m_emitter.call_is_true(cmp_result);
			auto *next_bb = llvm::BasicBlock::Create(m_ctx, "cmp.next", func);

			// 如果 false，存储 false 并跳到 merge
			auto *false_bb = llvm::BasicBlock::Create(m_ctx, "cmp.false", func);
			m_builder.CreateCondBr(is_true, next_bb, false_bb);

			m_builder.SetInsertPoint(false_bb);
			m_builder.CreateStore(cmp_result, result_alloca);
			m_builder.CreateBr(merge_bb);

			m_builder.SetInsertPoint(next_bb);
		} else {
			// 最后一个比较: 直接存储结果
			m_builder.CreateStore(cmp_result, result_alloca);
			m_builder.CreateBr(merge_bb);
		}

		// 下一轮的左操作数 = 这轮的右操作数（不重新求值！）
		prev = rhs;
	}

	m_builder.SetInsertPoint(merge_bb);
	auto *final_result = m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), result_alloca);
	return make_value(final_result);
}

ast::Value *PylangCodegen::visit(const ast::BoolOp *node)
{
	auto *func = m_codegen_ctx.current_function();
	auto *result_alloca = m_codegen_ctx.create_local(".bool_result", m_emitter.pyobject_ptr_type());
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "boolop.merge", func);

	auto &values = node->values();
	for (size_t i = 0; i < values.size(); ++i) {
		auto *val = generate(values[i].get());
		m_builder.CreateStore(val, result_alloca);

		if (i < values.size() - 1) {
			auto *is_true = m_emitter.call_is_true(val);
			auto *next_bb = llvm::BasicBlock::Create(m_ctx, "boolop.next", func);

			if (node->op() == ast::BoolOp::OpType::And) {
				// and: 如果 false 则短路
				m_builder.CreateCondBr(is_true, next_bb, merge_bb);
			} else {
				// or: 如果 true 则短路
				m_builder.CreateCondBr(is_true, merge_bb, next_bb);
			}
			m_builder.SetInsertPoint(next_bb);
		} else {
			m_builder.CreateBr(merge_bb);
		}
	}

	m_builder.SetInsertPoint(merge_bb);
	auto *result =
		m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), result_alloca, "boolop.result");
	return make_value(result);
}

ast::Value *PylangCodegen::visit(const ast::Call *node)
{
	auto *callable = generate(node->function().get());
	if (!callable) { return nullptr; }

	// 编译 positional args
	std::vector<llvm::Value *> arg_values;
	for (auto &arg : node->args()) { arg_values.push_back(generate(arg.get())); }

	auto *args_tuple = m_emitter.create_tuple(arg_values);

	// 编译 keyword args
	llvm::Value *kwargs = nullptr;
	if (!node->keywords().empty()) {
		std::vector<llvm::Value *> keys;
		std::vector<llvm::Value *> vals;
		for (auto &kw : node->keywords()) {
			if (kw->arg().has_value()) {
				keys.push_back(m_emitter.create_string(*kw->arg()));
				vals.push_back(generate(kw->value().get()));
			}
			// **kwargs expansion: Phase 3+
		}
		kwargs = m_emitter.create_dict(keys, vals);
	}

	auto *result = m_emitter.call_function(callable, args_tuple, kwargs);
	return make_value(result);
}

ast::Value *PylangCodegen::visit(const ast::Attribute *node)
{
	auto *obj = generate(node->value().get());
	if (!obj) { return nullptr; }

	switch (node->context()) {
	case ast::ContextType::LOAD:
		return make_value(m_emitter.call_getattr(obj, node->attr()));
	case ast::ContextType::DELETE:
		m_emitter.call_delattr(obj, node->attr());
		return nullptr;
	case ast::ContextType::STORE:
		// 处理由 Assign 驱动
		return make_value(obj);
	default:
		return nullptr;
	}
}

ast::Value *PylangCodegen::visit(const ast::Subscript *node)
{
	auto *obj = generate(node->value().get());
	if (!obj) { return nullptr; }

	if (std::holds_alternative<ast::Subscript::Index>(node->slice())) {
		auto &idx = std::get<ast::Subscript::Index>(node->slice());
		auto *key = generate(idx.value.get());
		if (!key) { return nullptr; }

		switch (node->context()) {
		case ast::ContextType::LOAD:
			return make_value(m_emitter.call_getitem(obj, key));
		case ast::ContextType::DELETE:
			m_emitter.call_delitem(obj, key);
			return nullptr;
		default:
			return make_value(obj);
		}
	}

	if (std::holds_alternative<ast::Subscript::Slice>(node->slice())) {
		auto &sl = std::get<ast::Subscript::Slice>(node->slice());
		auto *lower = sl.lower ? generate(sl.lower.get()) : m_emitter.get_none();
		auto *upper = sl.upper ? generate(sl.upper.get()) : m_emitter.get_none();
		auto *step = sl.step ? generate(sl.step.get()) : m_emitter.get_none();
		auto *slice_obj = m_emitter.create_slice(lower, upper, step);

		switch (node->context()) {
		case ast::ContextType::LOAD:
			return make_value(m_emitter.call_getitem(obj, slice_obj));
		case ast::ContextType::DELETE:
			m_emitter.call_delitem(obj, slice_obj);
			return nullptr;
		default:
			return make_value(obj);
		}
	}

	log::codegen()->warn("ExtSlice not implemented");
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::IfExpr *node)
{
	auto *func = m_codegen_ctx.current_function();
	auto *test = generate(node->test().get());
	auto *is_true = m_emitter.call_is_true(test);

	auto *then_bb = llvm::BasicBlock::Create(m_ctx, "ifexpr.then", func);
	auto *else_bb = llvm::BasicBlock::Create(m_ctx, "ifexpr.else", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "ifexpr.merge", func);

	m_builder.CreateCondBr(is_true, then_bb, else_bb);

	m_builder.SetInsertPoint(then_bb);
	auto *then_val = generate(node->body().get());
	auto *then_exit = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(else_bb);
	auto *else_val = generate(node->orelse().get());
	auto *else_exit = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(merge_bb);
	auto *phi = m_builder.CreatePHI(m_emitter.pyobject_ptr_type(), 2, "ifexpr.val");
	phi->addIncoming(then_val, then_exit);
	phi->addIncoming(else_val, else_exit);
	return make_value(phi);
}

// =============================================================================
// visit() 实现 — 容器字面量
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::List *node)
{
	std::vector<llvm::Value *> elements;
	for (auto &elem : node->elements()) { elements.push_back(generate(elem.get())); }
	return make_value(m_emitter.create_list(elements));
}

ast::Value *PylangCodegen::visit(const ast::Tuple *node)
{
	std::vector<llvm::Value *> elements;
	for (auto &elem : node->elements()) { elements.push_back(generate(elem.get())); }
	return make_value(m_emitter.create_tuple(elements));
}

ast::Value *PylangCodegen::visit(const ast::Dict *node)
{
	std::vector<llvm::Value *> keys, vals;
	for (auto &k : node->keys()) { keys.push_back(generate(k.get())); }
	for (auto &v : node->values()) { vals.push_back(generate(v.get())); }
	return make_value(m_emitter.create_dict(keys, vals));
}

ast::Value *PylangCodegen::visit(const ast::Set *node)
{
	std::vector<llvm::Value *> elements;
	for (auto &elem : node->elements()) { elements.push_back(generate(elem.get())); }
	return make_value(m_emitter.create_set(elements));
}

// =============================================================================
// visit() 实现 — 语句
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Module *node)
{
	generate_body(node->body());
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Expression *node)
{
	// 表达式语句: 计算值但丢弃结果
	generate(node->value().get());
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Assign *node)
{
	auto *value = generate(node->value().get());
	if (!value) { return nullptr; }

	for (auto &target : node->targets()) { generate_store_target(target.get(), value); }
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::AugAssign *node)
{
    // Python 3.9 语义: x += y
    //   1. LOAD x 的当前值
    //   2. 计算 y
    //   3. inplace op (先 __iadd__, 失败退回 __add__)
    //   4. STORE 结果回 x

    //  Step 1: LOAD 目标的当前值 
    llvm::Value *target_val = nullptr;
    const ast::Name *target_name = nullptr;
    const ast::Attribute *target_attr = nullptr;

    if (auto *name_node = dynamic_cast<const ast::Name *>(node->target().get())) {
        target_name = name_node;
        // ids() 返回 vector<string>，[0] 是变量名
        target_val = load_variable(name_node->ids()[0]);
    } else if (auto *attr_node = dynamic_cast<const ast::Attribute *>(node->target().get())) {
        target_attr = attr_node;
        // obj.attr += y → 先 getattr(obj, attr)
        auto *obj = generate(attr_node->value().get());
        if (!obj) { return nullptr; }
        target_val = m_emitter.emit_runtime_call("getattr",
            { obj, m_emitter.create_string(attr_node->attr()) });
    } else {
        // Subscript 等复杂目标暂不支持
        log::codegen()->warn("AugAssign: unsupported target type (subscript etc.)");
        return nullptr;
    }

    if (!target_val) { return nullptr; }

    //  Step 2: 计算 RHS 
    auto *rhs = generate(node->value().get());
    if (!rhs) { return nullptr; }

    //  Step 3: inplace 运算 
    llvm::Value *result = nullptr;
    switch (node->op()) {
    case ast::BinaryOpType::PLUS:
        result = m_emitter.emit_runtime_call("inplace_add", { target_val, rhs });
        break;
    case ast::BinaryOpType::MINUS:
        result = m_emitter.emit_runtime_call("inplace_sub", { target_val, rhs });
        break;
    case ast::BinaryOpType::MULTIPLY:
        result = m_emitter.emit_runtime_call("inplace_mul", { target_val, rhs });
        break;
    case ast::BinaryOpType::SLASH:
        result = m_emitter.emit_runtime_call("inplace_truediv", { target_val, rhs });
        break;
    case ast::BinaryOpType::FLOORDIV:
        result = m_emitter.emit_runtime_call("inplace_floordiv", { target_val, rhs });
        break;
    case ast::BinaryOpType::MODULO:
        result = m_emitter.emit_runtime_call("inplace_mod", { target_val, rhs });
        break;
    case ast::BinaryOpType::EXP:
        result = m_emitter.emit_runtime_call("inplace_pow", { target_val, rhs });
        break;
    case ast::BinaryOpType::LEFTSHIFT:
        result = m_emitter.emit_runtime_call("inplace_lshift", { target_val, rhs });
        break;
    case ast::BinaryOpType::RIGHTSHIFT:
        result = m_emitter.emit_runtime_call("inplace_rshift", { target_val, rhs });
        break;
    case ast::BinaryOpType::AND:
        result = m_emitter.emit_runtime_call("inplace_and", { target_val, rhs });
        break;
    case ast::BinaryOpType::OR:
        result = m_emitter.emit_runtime_call("inplace_or", { target_val, rhs });
        break;
    case ast::BinaryOpType::XOR:
        result = m_emitter.emit_runtime_call("inplace_xor", { target_val, rhs });
        break;
    default:
        log::codegen()->error("AugAssign: unsupported op: {}",
            ast::stringify_binary_op(node->op()));
        return nullptr;
    }

    if (!result) { return nullptr; }

    //  Step 4: STORE 结果回目标 
    if (target_name) {
        store_variable(target_name->ids()[0], result);
    } else if (target_attr) {
        auto *obj = generate(target_attr->value().get());
        if (obj) {
            m_emitter.emit_runtime_call("setattr",
                { obj, m_emitter.create_string(target_attr->attr()), result });
        }
    }

    return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Return *node)
{
	if (node->value()) {
		auto *val = generate(node->value().get());
		m_builder.CreateRet(val);
	} else {
		m_builder.CreateRet(m_emitter.get_none());
	}
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Delete *node)
{
	for (auto &target : node->targets()) {
		if (target->node_type() == ast::ASTNodeType::Name) {
			auto *name = static_cast<const ast::Name *>(target.get());
			delete_variable(name->ids()[0]);
		} else if (target->node_type() == ast::ASTNodeType::Subscript) {
			auto *sub = static_cast<const ast::Subscript *>(target.get());
			auto *obj = generate(sub->value().get());
			if (std::holds_alternative<ast::Subscript::Index>(sub->slice())) {
				auto &idx = std::get<ast::Subscript::Index>(sub->slice());
				auto *key = generate(idx.value.get());
				m_emitter.call_delitem(obj, key);
			}
		} else if (target->node_type() == ast::ASTNodeType::Attribute) {
			auto *attr = static_cast<const ast::Attribute *>(target.get());
			auto *obj = generate(attr->value().get());
			m_emitter.call_delattr(obj, attr->attr());
		}
	}
	return nullptr;
}

// =============================================================================
// visit() 实现 — 控制流
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::If *node)
{
	auto *func = m_codegen_ctx.current_function();
	auto *test = generate(node->test().get());
	auto *is_true = m_emitter.call_is_true(test);

	auto *then_bb = llvm::BasicBlock::Create(m_ctx, "if.then", func);
	auto *else_bb =
		node->orelse().empty() ? nullptr : llvm::BasicBlock::Create(m_ctx, "if.else", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "if.merge", func);

	m_builder.CreateCondBr(is_true, then_bb, else_bb ? else_bb : merge_bb);

	// then 分支
	m_builder.SetInsertPoint(then_bb);
	generate_body(node->body());
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(merge_bb); }

	// else 分支
	if (else_bb) {
		m_builder.SetInsertPoint(else_bb);
		generate_body(node->orelse());
		if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(merge_bb); }
	}

	m_builder.SetInsertPoint(merge_bb);
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::While *node)
{
	auto *func = m_codegen_ctx.current_function();

	auto *cond_bb = llvm::BasicBlock::Create(m_ctx, "while.cond", func);
	auto *body_bb = llvm::BasicBlock::Create(m_ctx, "while.body", func);
	auto *else_bb =
		node->orelse().empty() ? nullptr : llvm::BasicBlock::Create(m_ctx, "while.else", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "while.merge", func);

	m_builder.CreateBr(cond_bb);

	// 条件检查
	m_builder.SetInsertPoint(cond_bb);
	auto *test = generate(node->test().get());
	auto *is_true = m_emitter.call_is_true(test);
	m_builder.CreateCondBr(is_true, body_bb, else_bb ? else_bb : merge_bb);

	// 循环体
	m_codegen_ctx.push_loop({ merge_bb, cond_bb });
	m_builder.SetInsertPoint(body_bb);
	generate_body(node->body());
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(cond_bb); }
	m_codegen_ctx.pop_loop();

	// else 分支（循环正常结束时执行）
	if (else_bb) {
		m_builder.SetInsertPoint(else_bb);
		generate_body(node->orelse());
		if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(merge_bb); }
	}

	m_builder.SetInsertPoint(merge_bb);
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::For *node)
{
	auto *func = m_codegen_ctx.current_function();
	auto *iter_obj = generate(node->iter().get());
	auto *iterator = m_emitter.call_get_iter(iter_obj);

	auto *cond_bb = llvm::BasicBlock::Create(m_ctx, "for.cond", func);
	auto *body_bb = llvm::BasicBlock::Create(m_ctx, "for.body", func);
	auto *else_bb =
		node->orelse().empty() ? nullptr : llvm::BasicBlock::Create(m_ctx, "for.else", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "for.merge", func);

	m_builder.CreateBr(cond_bb);

	// 条件: has_value = iter_next(iter, &flag)
	m_builder.SetInsertPoint(cond_bb);
	auto *has_value_alloca =
		m_builder.CreateAlloca(llvm::Type::getInt1Ty(m_ctx), nullptr, "has_value");
	auto *next_val = m_emitter.call_iter_next(iterator, has_value_alloca);
	auto *has_value = m_builder.CreateLoad(llvm::Type::getInt1Ty(m_ctx), has_value_alloca);
	m_builder.CreateCondBr(has_value, body_bb, else_bb ? else_bb : merge_bb);

	// 循环体
	m_codegen_ctx.push_loop({ merge_bb, cond_bb });
	m_builder.SetInsertPoint(body_bb);
	generate_store_target(node->target().get(), next_val);
	generate_body(node->body());
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(cond_bb); }
	m_codegen_ctx.pop_loop();

	// else
	if (else_bb) {
		m_builder.SetInsertPoint(else_bb);
		generate_body(node->orelse());
		if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(merge_bb); }
	}

	m_builder.SetInsertPoint(merge_bb);
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Break *node)
{
	(void)node;
	auto *loop = m_codegen_ctx.current_loop();
	if (loop) { m_builder.CreateBr(loop->break_bb); }
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Continue *node)
{
	(void)node;
	auto *loop = m_codegen_ctx.current_loop();
	if (loop) { m_builder.CreateBr(loop->continue_bb); }
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Pass *node)
{
	(void)node;
	return nullptr;
}

// =============================================================================
// visit() 实现 — 函数定义
// =============================================================================
// =============================================================================
// 共享的函数编译逻辑（FunctionDefinition 和 Lambda 复用）
// =============================================================================

llvm::Value *PylangCodegen::compile_function_body(const std::string &func_name,
	const ast::Arguments *args_node,
	const std::vector<std::shared_ptr<ast::ASTNode>> &body,
	const std::vector<std::shared_ptr<ast::ASTNode>> &decorators,
	SourceLocation source_loc)
{
	auto *obj_ptr_ty = m_emitter.pyobject_ptr_type();

	//  Step 1: 创建 LLVM 函数 
	// 签名: PyObject*(PyObject* module, PyTuple* args, PyDict* kwargs)
	//        ^^^^^^^^^^^^^^^^ 隐式首参数：所属模块
	auto *func_type =
		llvm::FunctionType::get(obj_ptr_ty, { obj_ptr_ty, obj_ptr_ty, obj_ptr_ty }, false);

	auto mangled =
		m_mangler.function_mangle(m_codegen_ctx.current_scope().name, func_name, source_loc);

	auto *llvm_func =
		llvm::Function::Create(func_type, llvm::Function::InternalLinkage, mangled, m_module.get());

	llvm_func->getArg(0)->setName("module");
	llvm_func->getArg(1)->setName("args");
	llvm_func->getArg(2)->setName("kwargs");

	//  Step 2: 保存当前状态 
	auto *saved_bb = m_builder.GetInsertBlock();

	auto *entry_bb = llvm::BasicBlock::Create(m_ctx, "entry", llvm_func);
	m_builder.SetInsertPoint(entry_bb);

	//  Step 3: 查找变量 scope 
	auto it = m_codegen_ctx.visibility_map().find(mangled);
	VariablesResolver::Scope *var_scope =
		(it != m_codegen_ctx.visibility_map().end()) ? it->second.get() : nullptr;

	//  Step 4: 进入 FUNCTION scope 
	// module_obj = arg(0)：属于本函数的 LLVM Value，安全！
	ScopeContext func_scope{};
	func_scope.type = ScopeContext::Type::FUNCTION;
	func_scope.name = func_name;
	func_scope.mangled_name = mangled;
	func_scope.llvm_func = llvm_func;
	func_scope.module_obj = llvm_func->getArg(0);// ← 关键：module 来自本函数参数
	func_scope.var_scope = var_scope;
	func_scope.class_namespace = nullptr;
	m_codegen_ctx.push_scope(std::move(func_scope));

    //  Step 5: 提取函数参数并注册为 LOCAL 
    auto *args_tuple = llvm_func->getArg(1);// arg(1) = args

    if (args_node) {
        int32_t param_idx = 0;
        auto register_param = [&](const std::string &param_name) {
            auto *val = m_emitter.emit_runtime_call(
                "tuple_getitem", { args_tuple, m_builder.getInt32(param_idx) });
            auto *alloca =
                m_codegen_ctx.create_local(param_name, m_emitter.pyobject_ptr_type());
            m_builder.CreateStore(val, alloca);
            param_idx++;
        };

        // argument_names() 返回 posonlyargs + args 的名称
        for (const auto &name : args_node->argument_names()) {
            register_param(name);
        }
        // kwonly 参数也需要注册
        for (const auto &name : args_node->kw_only_argument_names()) {
            register_param(name);
        }
    }

	//  Step 6: 编译函数体 
	generate_body(body);

	//  Step 7: 确保有返回指令 
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateRet(m_emitter.get_none()); }

	//  Step 8: 离开 FUNCTION scope 
	m_codegen_ctx.pop_scope();

	//  Step 9: 恢复插入点到外层函数 
	m_builder.SetInsertPoint(saved_bb);

    //  Step 10: 编译默认值（在外层函数中求值）
    llvm::Value *defaults = m_emitter.null_pyobject();
    llvm::Value *kwdefaults = m_emitter.null_pyobject();

    if (args_node && !args_node->defaults().empty()) {
        std::vector<llvm::Value *> default_vals;
        for (const auto &def : args_node->defaults()) {
            auto *val = generate(def.get());
            if (val) { default_vals.push_back(val); }
        }
        if (!default_vals.empty()) { defaults = m_emitter.create_tuple(default_vals); }
    }

    if (args_node && !args_node->kw_defaults().empty()) {
        std::vector<llvm::Value *> kwdef_keys;
        std::vector<llvm::Value *> kwdef_vals;
        // 用 kw_only_argument_names() 获取名称列表
        auto kw_names = args_node->kw_only_argument_names();
        for (size_t i = 0; i < kw_names.size(); ++i) {
            if (i < args_node->kw_defaults().size() && args_node->kw_defaults()[i]) {
                auto *val = generate(args_node->kw_defaults()[i].get());
                if (val) {
                    kwdef_keys.push_back(m_emitter.create_string(kw_names[i]));
                    kwdef_vals.push_back(val);
                }
            }
        }
        if (!kwdef_keys.empty()) { kwdefaults = m_emitter.create_dict(kwdef_keys, kwdef_vals); }
    }

	//  Step 11: 在外层函数中创建 Python 可调用对象 
	// module_object() 返回外层 scope 的 module，是外层函数的 Value，安全
	auto *closure = m_emitter.null_pyobject();

	return m_emitter.call_make_function(
		func_name, llvm_func, m_codegen_ctx.module_object(), defaults, kwdefaults, closure);
}

ast::Value *PylangCodegen::visit(const ast::FunctionDefinition *node)
{
    auto *func_val = compile_function_body(
        node->name(),
        node->args().get(),
        node->body(),
        node->decorator_list(),
        node->source_location());

    if (!func_val) { return nullptr; }

    // 应用装饰器（从内到外，即列表逆序）
    // @dec1
    // @dec2
    // def f(): ...
    // 等价于 f = dec1(dec2(f))
    const auto &decorators = node->decorator_list();
    for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) {
        auto *dec = generate(it->get());
        if (!dec) { continue; }

        // 调用装饰器: func_val = dec(func_val)
        auto *args = m_emitter.create_tuple({ func_val });
        func_val = m_emitter.call_function(dec, args, m_emitter.null_pyobject());
    }

    // 将（可能被装饰后的）函数存入变量
    store_variable(node->name(), func_val);
    return nullptr;
}

// =============================================================================
// visit() 实现 — import
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Import *node)
{
	for (auto &alias : node->names()) {
		auto *mod = m_emitter.call_import(alias.name);
		std::string store_name = alias.asname.empty() ? alias.name : alias.asname;
		// import a.b.c 时只存储顶层模块名
		auto dot_pos = store_name.find('.');
		if (dot_pos != std::string::npos && alias.asname.empty()) {
			store_name = store_name.substr(0, dot_pos);
		}
		store_variable(store_name, mod);
	}
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::ImportFrom *node)
{
	// from module import name1, name2
	auto *mod =
		m_emitter.call_import(node->module(), nullptr, nullptr, static_cast<int>(node->level()));

	for (auto &alias : node->names()) {
		auto *attr = m_emitter.call_getattr(mod, alias.name);
		std::string store_name = alias.asname.empty() ? alias.name : alias.asname;
		store_variable(store_name, attr);
	}
	return nullptr;
}

// =============================================================================
// visit() 实现 — 异常相关
// =============================================================================

// ast::Value *PylangCodegen::visit(const ast::Raise *node)
// {
// 	if (node->exception()) {
// 		auto *exc = generate(node->exception().get());
// 		m_emitter.call_raise(exc);
// 	}
// 	// bare raise: Phase 4+
// 	m_builder.CreateUnreachable();
// 	return nullptr;
// }
ast::Value *PylangCodegen::visit(const ast::Raise *node)
{
	if (node->exception()) {// 不是 value()
		auto *exc = generate(node->exception().get());
		if (!exc) { return nullptr; }
		m_emitter.call_raise(exc);
	} else {
		// bare raise: 重新抛出当前异常
		m_emitter.call_reraise(m_emitter.null_pyobject());
	}

	m_builder.CreateUnreachable();

	auto *dead_bb =
		llvm::BasicBlock::Create(m_ctx, "after.raise", m_codegen_ctx.current_function());
	m_builder.SetInsertPoint(dead_bb);

	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Assert *node)
{
	auto *func = m_codegen_ctx.current_function();
	auto *test = generate(node->test().get());
	auto *is_true = m_emitter.call_is_true(test);

	auto *fail_bb = llvm::BasicBlock::Create(m_ctx, "assert.fail", func);
	auto *pass_bb = llvm::BasicBlock::Create(m_ctx, "assert.pass", func);

	m_builder.CreateCondBr(is_true, pass_bb, fail_bb);

	m_builder.SetInsertPoint(fail_bb);
	auto *exc_type = m_emitter.call_load_assertion_error();
	if (node->msg()) {
		auto *msg = generate(node->msg().get());
		auto *args = m_emitter.create_tuple({ msg });
		auto *exc = m_emitter.call_function(exc_type, args);
		m_emitter.call_raise(exc);
	} else {
		auto *args = m_emitter.create_tuple({});
		auto *exc = m_emitter.call_function(exc_type, args);
		m_emitter.call_raise(exc);
	}
	m_builder.CreateUnreachable();

	m_builder.SetInsertPoint(pass_bb);
	return nullptr;
}

// =============================================================================
// visit() 实现 — Global / NonLocal（不生成 IR）
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Global *node)
{
	(void)node;
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::NonLocal *node)
{
	(void)node;
	return nullptr;
}


// =============================================================================
// 类体编译
//
// 生成函数: PyObject* ClassName.__body__(PyTuple* args, PyDict* kwargs)
// 调用时: args[0] = namespace dict
//
// 类体中的变量存取路径:
//   store_variable("x", val)
//     → visibility = NAME
//     → class_namespace != nullptr
//     → call_dict_setitem_str(ns, "x", val)
// =============================================================================

llvm::Value *PylangCodegen::compile_class_body(const std::string &class_name,
	const std::vector<std::shared_ptr<ast::ASTNode>> &body,
	SourceLocation source_loc)
{
	auto *obj_ptr_ty = m_emitter.pyobject_ptr_type();

	// 签名: PyObject*(PyObject* module, PyTuple* args, PyDict* kwargs)
	auto *func_type =
		llvm::FunctionType::get(obj_ptr_ty, { obj_ptr_ty, obj_ptr_ty, obj_ptr_ty }, false);

	auto mangled =
		m_mangler.function_mangle(m_codegen_ctx.current_scope().name, class_name, source_loc);

	auto *llvm_func = llvm::Function::Create(
		func_type, llvm::Function::InternalLinkage, mangled + ".__body__", m_module.get());

	llvm_func->getArg(0)->setName("module");
	llvm_func->getArg(1)->setName("args");
	llvm_func->getArg(2)->setName("kwargs");

	auto *saved_bb = m_builder.GetInsertBlock();

	auto *entry_bb = llvm::BasicBlock::Create(m_ctx, "entry", llvm_func);
	m_builder.SetInsertPoint(entry_bb);

	// namespace dict = args[0]
	auto *ns = m_emitter.emit_runtime_call(
		"tuple_getitem", { llvm_func->getArg(1), m_builder.getInt32(0) });

	auto it = m_codegen_ctx.visibility_map().find(mangled);
	VariablesResolver::Scope *var_scope =
		(it != m_codegen_ctx.visibility_map().end()) ? it->second.get() : nullptr;

	ScopeContext class_scope{};
	class_scope.type = ScopeContext::Type::CLASS;
	class_scope.name = class_name;
	class_scope.mangled_name = mangled;
	class_scope.llvm_func = llvm_func;
	class_scope.module_obj = llvm_func->getArg(0);// ← module 来自本函数参数
	class_scope.var_scope = var_scope;
	class_scope.class_namespace = ns;
	m_codegen_ctx.push_scope(std::move(class_scope));

	generate_body(body);

	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateRet(m_emitter.get_none()); }

	m_codegen_ctx.pop_scope();
	m_builder.SetInsertPoint(saved_bb);

	return m_emitter.call_make_function(class_name,
		llvm_func,
		m_codegen_ctx.module_object(),// ← 外层 module，安全
		m_emitter.null_pyobject(),
		m_emitter.null_pyobject(),
		m_emitter.null_pyobject());
}
// =============================================================================
// visit(ClassDefinition)
//
// Python:
//   @decorator
//   class Foo(Bar, metaclass=Meta):
//       x = 1
//       def method(self): ...
//
// IR 调用序列:
//   %bases  = rt_build_tuple(...)
//   %kwargs = rt_build_dict(...)         ; metaclass=Meta
//   %body   = rt_make_function("Foo", @Foo.__body__, ...)
//   %cls    = rt_build_class_aot(%body, "Foo", %bases, %kwargs)
//   rt_store_global(%mod, "Foo", %cls)
//
// rt_build_class_aot 内部:
//   → py::build_class_aot (ClassBuilder.cpp)
//     → calculate_metaclass       ← C3 metaclass 冲突检测
//     → __prepare__               ← 创建 namespace
//     → body_fn(PyTuple(ns))      ← 执行类体
//     → metaclass(name,bases,ns)  ← PyType::__new__ → mro_() → ready()
//     → __classcell__ 处理        ← super() 支持
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::ClassDefinition *node)
{
	//  Step 1: 编译基类表达式
	std::vector<llvm::Value *> base_values;
	for (const auto &base : node->bases()) {
		auto *val = generate(base.get());
		if (val) { base_values.push_back(val); }
	}
	auto *bases_tuple = m_emitter.create_tuple(base_values);

	//  Step 2: 编译 keyword 参数（metaclass= 等）
	llvm::Value *kwargs = m_emitter.null_pyobject();
	if (!node->keywords().empty()) {
		std::vector<llvm::Value *> kw_keys;
		std::vector<llvm::Value *> kw_vals;
		for (const auto &kw : node->keywords()) {
			// kw 是 shared_ptr<ast::Keyword>，不需要 static_cast
			// arg() 返回 std::optional<std::string>，nullopt 表示 **dict 展开
			if (!kw->arg().has_value()) {
				log::codegen()->warn("**kwargs unpacking in class bases not yet supported");
				continue;
			}
			auto *val = generate(kw->value().get());
			if (val) {
				kw_keys.push_back(m_emitter.create_string(kw->arg().value()));
				kw_vals.push_back(val);
			}
		}
		if (!kw_keys.empty()) { kwargs = m_emitter.create_dict(kw_keys, kw_vals); }
	}

	//  Step 3: 编译类体函数
	auto *body_fn = compile_class_body(node->name(), node->body(), node->source_location());
	if (!body_fn) {
		log::codegen()->error("Failed to compile class body for '{}'", node->name());
		return nullptr;
	}

	//  Step 4: 调用 rt_build_class_aot
	auto *cls = m_emitter.call_build_class_aot(body_fn, node->name(), bases_tuple, kwargs);

	//  Step 5: 应用装饰器（从内到外）
	for (auto it = node->decorator_list().rbegin(); it != node->decorator_list().rend(); ++it) {
		auto *decorator = generate(it->get());
		if (decorator) {
			auto *args = m_emitter.create_tuple({ cls });
			cls = m_emitter.call_function(decorator, args);
		}
	}

	//  Step 6: 存储到当前作用域
	store_variable(node->name(), cls);
	return nullptr;
}
// =============================================================================
// visit() 实现 — 未实现的节点 (Phase 3+)
// =============================================================================
// Phase 3+: 以下节点在 Phase 2 中不生成代码
ast::Value *PylangCodegen::visit(const ast::Try *node)
{
	(void)node;
	emit_not_implemented("try/except");
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::ExceptHandler *node)
{
	(void)node;
	emit_not_implemented("try/except");
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::With *node)
{
	(void)node;
	emit_not_implemented("with");
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::WithItem *node)
{
	(void)node;
	emit_not_implemented("with item");
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Lambda *node)
{
	// Lambda 等价于匿名函数:  lambda x: x+1  →  def <lambda>(x): return x+1

	// 构造单语句 body: [Return(node->body())]
	// 注意: Lambda 的 body 是一个表达式节点，不是 Return 节点
	auto return_node = std::make_shared<ast::Return>(node->body(), node->source_location());
	std::vector<std::shared_ptr<ast::ASTNode>> body = { return_node };

	// 空装饰器列表
	std::vector<std::shared_ptr<ast::ASTNode>> no_decorators;

	auto *func_result = compile_function_body(
		"<lambda>", node->args().get(), body, no_decorators, node->source_location());

	if (!func_result) { return make_value(m_emitter.get_none()); }
	return make_value(func_result);
}

ast::Value *PylangCodegen::visit(const ast::AsyncFunctionDefinition *node)
{
	log::codegen()->warn("AsyncFunctionDefinition not implemented");
	(void)node;
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Await *node)
{
	log::codegen()->warn("Await not implemented");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::Yield *node)
{
	log::codegen()->warn("Yield not implemented");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::YieldFrom *node)
{
	log::codegen()->warn("YieldFrom not implemented");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::ListComp *node)
{
	log::codegen()->warn("ListComp not implemented in Phase 2");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::DictComp *node)
{
	log::codegen()->warn("DictComp not implemented in Phase 2");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::SetComp *node)
{
	log::codegen()->warn("SetComp not implemented in Phase 2");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::GeneratorExp *node)
{
	log::codegen()->warn("GeneratorExp not implemented in Phase 2");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::Comprehension *node)
{
	(void)node;
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Starred *node)
{
	// *args 展开: Phase 3+
	return make_value(generate(node->value().get()));
}

ast::Value *PylangCodegen::visit(const ast::NamedExpr *node)
{
	// walrus operator: x := expr
	auto *val = generate(node->value().get());
	generate_store_target(node->target().get(), val);
	return make_value(val);
}

ast::Value *PylangCodegen::visit(const ast::JoinedStr *node)
{
	log::codegen()->warn("f-string not implemented in Phase 2");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::FormattedValue *node)
{
	log::codegen()->warn("FormattedValue not implemented in Phase 2");
	(void)node;
	return make_value(m_emitter.get_none());
}

ast::Value *PylangCodegen::visit(const ast::Keyword *node)
{
	return make_value(generate(node->value().get()));
}

ast::Value *PylangCodegen::visit(const ast::Argument *node)
{
	(void)node;
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Arguments *node)
{
	(void)node;
	return nullptr;
}

}// namespace pylang
