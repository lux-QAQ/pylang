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

	// Step 4: 创建模块初始化函数 PyInit_<module_name>
	auto *init_func = codegen.create_module_init_function();

	// Step 5: 设置 VariablesResolver scope
	auto mangled = codegen.m_mangler.function_mangle(
		std::string(module_name), "<module>", module_node->source_location());
	auto it = visibility.find(mangled);
	VariablesResolver::Scope *var_scope = nullptr;

	if (it != visibility.end()) {
		var_scope = it->second.get();
	} else {
		for (auto &[key, scope_ptr] : visibility) {
			if (scope_ptr && scope_ptr->type == VariablesResolver::Scope::Type::MODULE) {
				var_scope = scope_ptr.get();
				break;
			}
		}
	}

	// Step 6: 进入模块 scope
	ScopeContext module_scope{};
	module_scope.type = ScopeContext::Type::MODULE;
	module_scope.name = std::string(module_name);
	module_scope.mangled_name = mangled;
	module_scope.llvm_func = init_func;
	module_scope.module_obj = nullptr;
	module_scope.var_scope = var_scope;
	codegen.m_codegen_ctx.push_scope(std::move(module_scope));

	// Step 7: 生成模块初始化函数体起始指令
	auto *mod = codegen.m_emitter.call_add_module(module_name);
	codegen.m_codegen_ctx.current_scope().module_obj = mod;

	// [修复]：使用 Alloca 指令作为锚点，而不是会被常量折叠的 Add(0, 0)。
	// Alloca 保证是一个 llvm::Instruction，且在函数入口块是安全的。
	// 这确保了后续 generate_body 中发现的字符串都能通过 SetInsertPoint(anchor) 正确前插。
	auto *anchor =
		codegen.m_builder.CreateAlloca(codegen.m_builder.getPtrTy(), nullptr, "pystr_init_anchor");

	// Step 8: 遍历 AST，生成 IR
	// 此时 anchor 指令已在 entry block 中，generate_body 生成的指令将排在 anchor 之后。
	codegen.generate_body(module_node->body());

	// Step 8.5: [核心修复] 将常量池初始化代码回填到锚点之前
	// 这样无论 generate_body 里有多少全局代码（如 verify() 调用），字符串常量都已初始化完毕。
	auto body_insert_point = codegen.m_builder.saveIP();
	codegen.m_builder.SetInsertPoint(anchor);

	codegen.m_emitter.emit_interned_strings_initialization();

	codegen.m_builder.restoreIP(body_insert_point);

	// Step 9: 模块初始化函数返回 void
	if (!codegen.m_builder.GetInsertBlock()->getTerminator()) { codegen.m_builder.CreateRetVoid(); }

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

	log::codegen()->info(
		"Module '{}' compiled successfully with interned-string-cache", module_name);
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

	// 必须添加 UWTable，否则 C++ 异常无法通过该函数传播
	func->addFnAttr(llvm::Attribute::UWTable);

	auto *entry = llvm::BasicBlock::Create(m_ctx, "entry", func);
	m_builder.SetInsertPoint(entry);

	return func;
}

llvm::Function *PylangCodegen::create_main_function(llvm::Function *module_init)
{
	auto *i32_ty = llvm::Type::getInt32Ty(m_ctx);
	auto *ptr_ty = llvm::PointerType::getUnqual(m_ctx);
	auto *main_ty = llvm::FunctionType::get(i32_ty, { i32_ty, ptr_ty }, false);
	auto *main_func =
		llvm::Function::Create(main_ty, llvm::Function::ExternalLinkage, "main", m_module.get());

	auto *entry = llvm::BasicBlock::Create(m_ctx, "entry", main_func);
	m_builder.SetInsertPoint(entry);

	// call void @rt_init()
	m_emitter.emit_init();

	// 修复: 用 invoke 调用 PyInit，捕获未处理的 Python 异常
	auto *normal_bb = llvm::BasicBlock::Create(m_ctx, "normal", main_func);
	auto *unwind_bb = llvm::BasicBlock::Create(m_ctx, "unwind", main_func);

	// 设置 personality 函数
	if (!main_func->hasPersonalityFn()) {
		auto *personality = m_emitter.get_personality_function();
		main_func->setPersonalityFn(personality);
	}

	m_builder.CreateInvoke(module_init, normal_bb, unwind_bb);

	// 正常路径
	m_builder.SetInsertPoint(normal_bb);
	m_emitter.emit_shutdown();
	m_builder.CreateRet(llvm::ConstantInt::get(i32_ty, 0));

	// 异常路径: 打印未捕获异常并返回 1
	m_builder.SetInsertPoint(unwind_bb);
	auto *lp = m_builder.CreateLandingPad(llvm::StructType::get(m_ctx, { ptr_ty, i32_ty }), 0);
	lp->addClause(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_ctx)));

	auto *exc_ptr = m_builder.CreateExtractValue(lp, 0);

	auto *py_exc = m_emitter.call_catch_begin(exc_ptr);
	m_emitter.call_print_unhandled_exception(py_exc);
	m_emitter.call_catch_end();
	m_emitter.emit_shutdown();

	m_builder.CreateRet(llvm::ConstantInt::get(i32_ty, 1));

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

		// alloca 输出解包数组需要放在入口以防爆栈
		auto *ptr_ty = m_emitter.pyobject_ptr_type();
		auto *arr_ty = llvm::ArrayType::get(ptr_ty, count);
		auto *out_arr = m_emitter.create_entry_block_alloca(arr_ty, "unpack_arr");

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

	std::visit(
		overloaded{
			[&](const py::Number &num) {
				std::visit(overloaded{
							   [&](double d) { result = m_emitter.create_float(d); },
							   [&](const py::BigIntType &big) {
								   if (big.fits_slong_p()) {
									   result = m_emitter.create_integer(big.get_si());
								   } else {
									   result = m_emitter.create_integer_big(big.get_str());
								   }
							   },
						   },
					num.value);
			},
			//[&](const py::Complex &c) { result = m_emitter.create_complex(c.real, c.imag); },
			[&](const py::String &str) { result = m_emitter.create_string(str.s); },
			[&](const py::Bytes &bytes) {
				std::string_view data(
					reinterpret_cast<const char *>(bytes.b.data()), bytes.b.size());
				result = m_emitter.create_bytes(data);
			},
			[&](const py::Ellipsis &) { result = m_emitter.get_ellipsis(); },
			[&](const py::NoneType &) { result = m_emitter.get_none(); },
			[&](const py::NameConstant &nc) {
				std::visit(
					overloaded{
						[&](bool b) { result = b ? m_emitter.get_true() : m_emitter.get_false(); },
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

// ast::Value *PylangCodegen::visit(const ast::Call *node)
// {
//     // 拦截无参 super() 并重写为 super(__class__, self)
//     // 这解决了 AOT 运行时缺少解释器栈帧(PyFrame)导致无法推断参数的问题。
//     if (auto *name_node = dynamic_cast<const ast::Name *>(node->function().get())) {
//         if (name_node->ids().size() == 1 && name_node->ids()[0] == "super" &&
//         node->args().empty()
//             && node->keywords().empty()) {

//             // 检查上下文:
//             // 1. 我们必须在一个函数内部 (m_func_first_args 非空)
//             // 2. 函数必须有第一个参数 (self)
//             // 3. __class__ 必须在作用域内 (由 VariablesResolver 保证)
//             if (!m_func_first_args.empty() && !m_func_first_args.back().empty()) {
//                 const std::string &self_name = m_func_first_args.back();

//                 // 尝试加载 'super', '__class__', 和 'self'
//                 // 注意: VariablesResolver 已处理 'super' 会隐式捕获 '__class__'

//                 // 1. 加载 super 全局对象
//                 llvm::Value *super_cls = load_variable("super");

//                 // 2. 加载 __class__ (作为闭包/Cell变量)
//                 llvm::Value *class_cell = load_variable("__class__");

//                 // 3. 加载 self (作为局部变量)
//                 llvm::Value *self_obj = load_variable(self_name);

//                 if (super_cls && class_cell && self_obj) {
//                     log::codegen()->debug("Rewriting super() to super(__class__, {})",
//                     self_name);

//                     // 构造参数 tuple: (__class__, self)
//                     auto *args_tuple = m_emitter.create_tuple({ class_cell, self_obj });

//                     // 生成调用: super(__class__, self)
//                     // 使用 implicit kwargs (None)
//                     auto *res = m_emitter.call_function(super_cls, args_tuple, nullptr);
//                     return make_value(res);
//                 }
//             }
//         }
//     }

//     // === 优化：识别 Method 调用 ===
//     bool is_method = false;
//     llvm::Value *method_owner = nullptr;
//     std::string method_name;

//     if (auto *attr_node = dynamic_cast<const ast::Attribute *>(node->function().get())) {
//         method_owner = generate(attr_node->value().get());
//         if (!method_owner) { return nullptr; }
//         method_name = attr_node->attr();
//         is_method = true;
//     }

//     llvm::Value *func_val = nullptr;
//     if (!is_method) {
//         func_val = generate(node->function().get());
//         if (!func_val) { return nullptr; }
//     }

//     // --- 收集位置参数 ---
//     bool has_starred = false;
//     for (const auto &arg : node->args()) {
//         if (dynamic_cast<const ast::Starred *>(arg.get())) {
//             has_starred = true;
//             break;
//         }
//     }

//     // --- 收集关键字参数 ---
//     llvm::Value *kwargs_dict = nullptr;
//     bool has_kwargs = false;

//     for (const auto &kw : node->keywords()) {
//         auto *kw_node = dynamic_cast<const ast::Keyword *>(kw.get());
//         if (!kw_node) { continue; }

//         auto *kw_value = generate(kw_node->value().get());
//         if (!kw_value) { continue; }

//         if (!kwargs_dict) {
//             kwargs_dict = m_emitter.create_dict({}, {});
//             has_kwargs = true;
//         }

//         if (!kw_node->arg().has_value()) {
//             m_emitter.call_dict_merge(kwargs_dict, kw_value);
//         } else {
//             m_emitter.call_dict_setitem_str(kwargs_dict, *kw_node->arg(), kw_value);
//         }
//     }

//     if (!has_kwargs) { kwargs_dict = m_emitter.null_pyobject(); }

//     // === 生成超高性能调用路径 ===
//     if (!has_starred) {
//         // 无 *args: 绝对零拷贝 (Zero-allocation)
//         std::vector<llvm::Value *> pos_args;
//         for (const auto &arg : node->args()) {
//             auto *val = generate(arg.get());
//             if (val) { pos_args.push_back(val); }
//         }

//         auto *argc_val = m_builder.getInt32(pos_args.size());
//         llvm::Value *args_ptr = nullptr;
//         if (pos_args.empty()) {
//             args_ptr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_ctx));
//         } else {
//             // 必须在入口块分配，防止 while 循环爆栈！
//             auto *ptr_ty = m_emitter.pyobject_ptr_type();
//             auto *arr_ty = llvm::ArrayType::get(ptr_ty, pos_args.size());
//             auto *alloc = m_codegen_ctx.create_local("raw_args", arr_ty);
//             args_ptr = m_builder.CreateBitCast(alloc, llvm::PointerType::getUnqual(m_ctx));

//             // 在当前执行块填入数据
//             for (size_t i = 0; i < pos_args.size(); ++i) {
//                 auto *gep = m_builder.CreateConstGEP2_32(arr_ty, alloc, 0, i);
//                 m_builder.CreateStore(pos_args[i], gep);
//             }
//         }

//         llvm::Value *result = nullptr;
//         if (is_method) {
//             // 方法调用：直接使用 C 风格字符串发起查找和穿透执行
//             auto *cstr_val = m_builder.CreateGlobalStringPtr(method_name);
//             result = m_emitter.call_method_ic_ptrs(method_owner, cstr_val, args_ptr, argc_val,
//             kwargs_dict);
//         } else {
//             // 普通调用：传递纯数组
//             result = m_emitter.call_function_raw_ptrs(func_val, args_ptr, argc_val, kwargs_dict);
//         }
//         return make_value(result);
//     } else {
//         // 有 *args: 回退到 Tuple 逻辑
//         if (is_method) { func_val = m_emitter.call_getattr(method_owner, method_name); }

//         auto *tmp_list = m_emitter.create_list({});
//         for (const auto &arg : node->args()) {
//             if (auto *starred = dynamic_cast<const ast::Starred *>(arg.get())) {
//                 auto *iterable = generate(starred->value().get());
//                 if (iterable) { m_emitter.call_list_extend(tmp_list, iterable); }
//             } else {
//                 auto *val = generate(arg.get());
//                 if (val) { m_emitter.call_list_append(tmp_list, val); }
//             }
//         }
//         llvm::Value *args_tuple = m_emitter.call_list_to_tuple(tmp_list);
//         auto *result = m_emitter.call_function(func_val, args_tuple, kwargs_dict);
//         return make_value(result);
//     }
// }

ast::Value *PylangCodegen::visit(const ast::Call *node)
{
	// 拦截无参 super() 并重写为 super(__class__, self)
	if (auto *name_node = dynamic_cast<const ast::Name *>(node->function().get())) {
		if (name_node->ids().size() == 1 && name_node->ids()[0] == "super" && node->args().empty()
			&& node->keywords().empty()) {

			if (!m_func_first_args.empty() && !m_func_first_args.back().empty()) {
				const std::string &self_name = m_func_first_args.back();
				llvm::Value *super_cls = load_variable("super");
				llvm::Value *class_cell = load_variable("__class__");
				llvm::Value *self_obj = load_variable(self_name);

				if (super_cls && class_cell && self_obj) {
					auto *args_tuple = m_emitter.create_tuple({ class_cell, self_obj });
					auto *res = m_emitter.call_function(super_cls, args_tuple, nullptr);
					return make_value(res);
				}
			}
		}
	}

	// === 优化：识别 Method 调用 ===
	bool is_method = false;
	llvm::Value *method_owner = nullptr;
	std::string method_name;

	// 1. 严格求值顺序：先求值 Callable/Owner
	if (auto *attr_node = dynamic_cast<const ast::Attribute *>(node->function().get())) {
		method_owner = generate(attr_node->value().get());
		if (!method_owner) { return nullptr; }
		method_name = attr_node->attr();
		is_method = true;
	}

	llvm::Value *func_val = nullptr;
	if (!is_method) {
		func_val = generate(node->function().get());
		if (!func_val) { return nullptr; }
	}

	// 检查是否有 *args 拆包
	bool has_starred = false;
	for (const auto &arg : node->args()) {
		if (dynamic_cast<const ast::Starred *>(arg.get())) {
			has_starred = true;
			break;
		}
	}

	// === 生成超高性能调用路径 ===
	if (!has_starred) {
		// 无 *args: 绝对零拷贝 (Zero-allocation)

		// 2. 严格求值顺序：求值位置参数
		std::vector<llvm::Value *> pos_args;
		for (const auto &arg : node->args()) {
			auto *val = generate(arg.get());
			if (val) { pos_args.push_back(val); }
		}

		// 3. 严格求值顺序：最后求值关键字参数
		llvm::Value *kwargs_dict = nullptr;
		bool has_kwargs = false;
		for (const auto &kw : node->keywords()) {
			auto *kw_node = dynamic_cast<const ast::Keyword *>(kw.get());
			if (!kw_node) continue;

			auto *kw_value = generate(kw_node->value().get());
			if (!kw_value) continue;

			if (!kwargs_dict) {
				kwargs_dict = m_emitter.create_dict({}, {});
				has_kwargs = true;
			}

			if (!kw_node->arg().has_value()) {
				m_emitter.call_dict_merge(kwargs_dict, kw_value);
			} else {
				m_emitter.call_dict_setitem_str(kwargs_dict, *kw_node->arg(), kw_value);
			}
		}
		if (!has_kwargs) { kwargs_dict = m_emitter.null_pyobject(); }

		auto *argc_val = m_builder.getInt32(pos_args.size());
		llvm::Value *args_ptr = nullptr;

		if (pos_args.empty()) {
			args_ptr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_ctx));
		} else {
			// [安全修复] 必须在入口块分配，且不经过 symbol table (避免多次调用的变量名/类型冲突)
			auto *ptr_ty = m_emitter.pyobject_ptr_type();
			auto *arr_ty = llvm::ArrayType::get(ptr_ty, pos_args.size());

			auto &entry_bb = m_codegen_ctx.current_function()->getEntryBlock();
			llvm::IRBuilder<> entry_builder(&entry_bb, entry_bb.begin());
			auto *alloc = entry_builder.CreateAlloca(arr_ty, nullptr, "raw_args");

			args_ptr = m_builder.CreateBitCast(alloc, llvm::PointerType::getUnqual(m_ctx));

			// 在当前执行块填入数据
			for (size_t i = 0; i < pos_args.size(); ++i) {
				auto *gep = m_builder.CreateConstGEP2_32(arr_ty, alloc, 0, i);
				m_builder.CreateStore(pos_args[i], gep);
			}
		}

		llvm::Value *result = nullptr;
		if (is_method) {
			// 方法调用：直接使用 C 风格字符串发起查找和穿透执行
			auto *cstr_val = m_builder.CreateGlobalString(method_name);
			result = m_emitter.call_method_ic_ptrs(
				method_owner, cstr_val, args_ptr, argc_val, kwargs_dict);
		} else {
			// 普通调用：传递纯数组
			result = m_emitter.call_function_raw_ptrs(func_val, args_ptr, argc_val, kwargs_dict);
		}
		return make_value(result);
	} else {
		// === 有 *args: 回退到 Tuple 分配逻辑 ===

		if (is_method) { func_val = m_emitter.call_getattr(method_owner, method_name); }

		auto *tmp_list = m_emitter.create_list({});
		for (const auto &arg : node->args()) {
			if (auto *starred = dynamic_cast<const ast::Starred *>(arg.get())) {
				auto *iterable = generate(starred->value().get());
				if (iterable) { m_emitter.call_list_extend(tmp_list, iterable); }
			} else {
				auto *val = generate(arg.get());
				if (val) { m_emitter.call_list_append(tmp_list, val); }
			}
		}
		llvm::Value *args_tuple = m_emitter.call_list_to_tuple(tmp_list);

		// Slow Path 也需保证 kwargs 最后求值
		llvm::Value *kwargs_dict = nullptr;
		bool has_kwargs = false;
		for (const auto &kw : node->keywords()) {
			auto *kw_node = dynamic_cast<const ast::Keyword *>(kw.get());
			if (!kw_node) continue;
			auto *kw_value = generate(kw_node->value().get());
			if (!kw_value) continue;

			if (!kwargs_dict) {
				kwargs_dict = m_emitter.create_dict({}, {});
				has_kwargs = true;
			}
			if (!kw_node->arg().has_value()) {
				m_emitter.call_dict_merge(kwargs_dict, kw_value);
			} else {
				m_emitter.call_dict_setitem_str(kwargs_dict, *kw_node->arg(), kw_value);
			}
		}
		if (!has_kwargs) { kwargs_dict = m_emitter.null_pyobject(); }

		auto *result = m_emitter.call_function(func_val, args_tuple, kwargs_dict);
		return make_value(result);
	}
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
	// =================================================================
	// Python 3.9 语义: target op= value
	//
	//   1. LOAD target 的当前值（保存中间值避免重复求值）
	//   2. 计算 RHS
	//   3. 调用 inplace 运算（rt_inplace_xxx）
	//   4. STORE 结果回 target
	//
	// 支持的 target 类型:
	//   - Name:      x += 1
	//   - Attribute: obj.attr += 1
	//   - Subscript: obj[key] += 1
	// =================================================================

	auto *rhs = generate(node->value().get());
	if (!rhs) { return nullptr; }

	// --- Step 1: LOAD 当前值 + 保存副作用表达式的中间值 ---
	llvm::Value *current_val = nullptr;

	// 用于 STORE 阶段复用的中间值（避免重复求值 obj / key）
	llvm::Value *attr_obj = nullptr;
	llvm::Value *subscr_obj = nullptr;
	llvm::Value *subscr_key = nullptr;

	const auto *target = node->target().get();

	if (auto *name_node = dynamic_cast<const ast::Name *>(target)) {
		current_val = load_variable(name_node->ids()[0]);

	} else if (auto *attr_node = dynamic_cast<const ast::Attribute *>(target)) {
		attr_obj = generate(attr_node->value().get());
		if (!attr_obj) { return nullptr; }
		current_val = m_emitter.call_getattr(attr_obj, attr_node->attr());

	} else if (auto *subscr_node = dynamic_cast<const ast::Subscript *>(target)) {
		subscr_obj = generate(subscr_node->value().get());
		if (!subscr_obj) { return nullptr; }

		if (std::holds_alternative<ast::Subscript::Index>(subscr_node->slice())) {
			auto &idx = std::get<ast::Subscript::Index>(subscr_node->slice());
			subscr_key = generate(idx.value.get());
		} else if (std::holds_alternative<ast::Subscript::Slice>(subscr_node->slice())) {
			auto &sl = std::get<ast::Subscript::Slice>(subscr_node->slice());
			auto *start = sl.lower ? generate(sl.lower.get()) : m_emitter.get_none();
			auto *stop = sl.upper ? generate(sl.upper.get()) : m_emitter.get_none();
			auto *step = sl.step ? generate(sl.step.get()) : m_emitter.get_none();
			subscr_key = m_emitter.create_slice(start, stop, step);
		} else {
			log::codegen()->warn("AugAssign: ExtSlice not supported");
			return nullptr;
		}
		if (!subscr_key) { return nullptr; }
		current_val = m_emitter.call_getitem(subscr_obj, subscr_key);

	} else {
		log::codegen()->error("AugAssign: unsupported target type");
		return nullptr;
	}

	if (!current_val) { return nullptr; }

	// --- Step 2: inplace 运算 ---
	llvm::Value *result = nullptr;
	switch (node->op()) {
	case ast::BinaryOpType::PLUS:
		result = m_emitter.call_inplace_add(current_val, rhs);
		break;
	case ast::BinaryOpType::MINUS:
		result = m_emitter.call_inplace_sub(current_val, rhs);
		break;
	case ast::BinaryOpType::MULTIPLY:
		result = m_emitter.call_inplace_mul(current_val, rhs);
		break;
	case ast::BinaryOpType::SLASH:
		result = m_emitter.call_inplace_truediv(current_val, rhs);
		break;
	case ast::BinaryOpType::FLOORDIV:
		result = m_emitter.call_inplace_floordiv(current_val, rhs);
		break;
	case ast::BinaryOpType::MODULO:
		result = m_emitter.call_inplace_mod(current_val, rhs);
		break;
	case ast::BinaryOpType::EXP:
		result = m_emitter.call_inplace_pow(current_val, rhs);
		break;
	case ast::BinaryOpType::LEFTSHIFT:
		result = m_emitter.call_inplace_lshift(current_val, rhs);
		break;
	case ast::BinaryOpType::RIGHTSHIFT:
		result = m_emitter.call_inplace_rshift(current_val, rhs);
		break;
	case ast::BinaryOpType::AND:
		result = m_emitter.call_inplace_and(current_val, rhs);
		break;
	case ast::BinaryOpType::OR:
		result = m_emitter.call_inplace_or(current_val, rhs);
		break;
	case ast::BinaryOpType::XOR:
		result = m_emitter.call_inplace_xor(current_val, rhs);
		break;
	default:
		log::codegen()->error("AugAssign: unsupported operator");
		return nullptr;
	}

	if (!result) { return nullptr; }

	// --- Step 3: STORE 结果（复用 Step 1 保存的中间值）---
	if (auto *name_node = dynamic_cast<const ast::Name *>(target)) {
		store_variable(name_node->ids()[0], result);

	} else if (auto *attr_node = dynamic_cast<const ast::Attribute *>(target)) {
		// 复用 attr_obj，不重复求值
		m_emitter.call_setattr(attr_obj, attr_node->attr(), result);

	} else if (subscr_obj && subscr_key) {
		// 复用 subscr_obj 和 subscr_key，不重复求值
		m_emitter.call_setitem(subscr_obj, subscr_key, result);
	}

	return nullptr;
}

// ast::Value *PylangCodegen::visit(const ast::Return *node)
// {
// 	if (node->value()) {
// 		auto *val = generate(node->value().get());
// 		m_builder.CreateRet(val);
// 	} else {
// 		m_builder.CreateRet(m_emitter.get_none());
// 	}
// 	return nullptr;
// }
// =============================================================================
// visit(Return*) — try 块内的 return 需要走 finally
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Return *node)
{
	llvm::Value *retval = node->value() ? generate(node->value().get()) : m_emitter.get_none();

	if (auto *try_ctx = m_codegen_ctx.nearest_finally_try()) {
		m_builder.CreateStore(retval, try_ctx->retval_alloca);
		m_builder.CreateStore(m_builder.getInt32(2), try_ctx->state_alloca);
		m_builder.CreateBr(try_ctx->finally_bb);
	} else {
		m_builder.CreateRet(retval);
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

	// 在 entry block 分配判定标志，循环重复利用这 1 bytes 地址，从此杜绝爆栈
	auto *has_value_alloca =
		m_codegen_ctx.create_local("for_has_value", llvm::Type::getInt1Ty(m_ctx));

	// 条件: has_value = iter_next(iter, &flag)
	m_builder.SetInsertPoint(cond_bb);
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

// =============================================================================
// visit(Break*) / visit(Continue*) — try 块内需要走 finally
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Break *node)
{
	(void)node;
	if (auto *try_ctx = m_codegen_ctx.nearest_finally_try()) {
		m_builder.CreateStore(m_builder.getInt32(3), try_ctx->state_alloca);
		m_builder.CreateBr(try_ctx->finally_bb);
	} else {
		auto *loop = m_codegen_ctx.current_loop();
		ASSERT(loop);
		m_builder.CreateBr(loop->break_bb);
	}
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::Continue *node)
{
	(void)node;
	if (auto *try_ctx = m_codegen_ctx.nearest_finally_try()) {
		m_builder.CreateStore(m_builder.getInt32(4), try_ctx->state_alloca);
		m_builder.CreateBr(try_ctx->finally_bb);
	} else {
		auto *loop = m_codegen_ctx.current_loop();
		ASSERT(loop);
		m_builder.CreateBr(loop->continue_bb);
	}
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
	// === Step 1: Mangle 名字 ===
	auto &enclosing = m_codegen_ctx.current_scope();
	auto mangled = m_mangler.function_mangle(enclosing.mangled_name, func_name, source_loc);

	// 记录第一个参数名（用于 super() 支持）
	std::string first_arg_name;
	if (args_node) {
		if (!args_node->posonlyargs().empty()) {
			first_arg_name = args_node->posonlyargs().front()->name();
		} else if (!args_node->args().empty()) {
			first_arg_name = args_node->args().front()->name();
		}
	}
	m_func_first_args.push_back(first_arg_name);

	// Scope 查找
	VariablesResolver::Scope *var_scope = nullptr;
	auto *parent_var_scope = enclosing.var_scope;
	if (parent_var_scope) {
		for (auto &child_ref : parent_var_scope->children) {
			auto &child = child_ref.get();
			auto last_dot = child.namespace_.rfind('.');
			std::string child_plain_name = (last_dot != std::string::npos)
											   ? child.namespace_.substr(last_dot + 1)
											   : child.namespace_;
			if (child_plain_name == func_name) {
				var_scope = &child;
				break;
			}
		}
	}
	if (!var_scope) {
		auto vis_it = m_codegen_ctx.visibility_map().find(mangled);
		if (vis_it != m_codegen_ctx.visibility_map().end()) { var_scope = vis_it->second.get(); }
	}
	if (!var_scope) {
		log::codegen()->warn(
			"compile_function_body: no var_scope found for '{}' (mangled: {})", func_name, mangled);
	}

	// === Step 2: 收集 CELL 和 FREE 变量 ===
	std::vector<std::string> cell_vars;
	std::vector<std::string> free_vars;
	m_codegen_ctx.collect_cell_and_free_vars(var_scope, cell_vars, free_vars);

	// === Step 2.5: 在外围作用域中编译默认参数值 ===
	// Python 3.9 语义：默认值在函数*定义*时求值，不是调用时。
	// 所以在外围作用域生成 IR，然后通过 closure 传给函数体。
	llvm::Value *defaults_tuple = nullptr;
	size_t defaults_count = 0;
	if (args_node && !args_node->defaults().empty()) {
		std::vector<llvm::Value *> default_vals;
		default_vals.reserve(args_node->defaults().size());
		for (const auto &def : args_node->defaults()) {
			auto *val = generate(def.get());
			if (val) { default_vals.push_back(val); }
		}
		if (!default_vals.empty()) {
			defaults_tuple = m_emitter.create_tuple(default_vals);
			defaults_count = default_vals.size();
		}
	}

	// 如果有默认值或有 free vars，需要 closure
	bool has_closure = !free_vars.empty() || (defaults_tuple != nullptr);

	// === Step 3: 创建 LLVM 函数 ===
	auto *ptr_ty = m_emitter.pyobject_ptr_type();
	auto *i32_ty = m_builder.getInt32Ty();

	std::vector<llvm::Type *> param_types;
	param_types.push_back(ptr_ty);// %module

	// ✅ [关键修复]：取消 if (has_closure)，强制占位。
	// 所有 AOT 函数必须符合 AOTRawFuncPtr 签名
	param_types.push_back(ptr_ty);// %closure (即使不用也占位，保证寄存器对齐)

	// 修改签名：使用 VECTORCALL 约定
	param_types.push_back(ptr_ty);// %args_array (Value*)
	param_types.push_back(i32_ty);// %argc
	param_types.push_back(ptr_ty);// %kwargs (PyDict*)

	auto *func_ty = llvm::FunctionType::get(ptr_ty, param_types, false);
	auto *func =
		llvm::Function::Create(func_ty, llvm::Function::InternalLinkage, mangled, m_module.get());

	ensure_personality(func);

	auto arg_it = func->arg_begin();
	arg_it->setName("module");
	auto *module_arg = &*arg_it++;

	// ✅ [关键修复]：取消 if (has_closure)
	arg_it->setName("closure");
	auto *closure_arg = &*arg_it++;

	arg_it->setName("args_array");
	auto *args_array_arg = &*arg_it++;
	arg_it->setName("argc");
	auto *argc_arg = &*arg_it++;
	arg_it->setName("kwargs");
	auto *kwargs_arg = &*arg_it++;

	// === Step 4: 保存当前插入点 ===
	auto saved_ip = m_builder.saveIP();

	auto *entry_bb = llvm::BasicBlock::Create(m_ctx, "entry", func);
	m_builder.SetInsertPoint(entry_bb);

	// === Step 5: 进入新作用域 ===
	ScopeContext func_scope{};
	func_scope.type = ScopeContext::Type::FUNCTION;
	func_scope.name = func_name;
	func_scope.mangled_name = mangled;
	func_scope.llvm_func = func;
	func_scope.module_obj = module_arg;
	func_scope.var_scope = var_scope;
	m_codegen_ctx.push_scope(std::move(func_scope));

	// === Step 6: 为 CELL 变量创建 cell 对象 ===
	for (const auto &cv : cell_vars) {
		auto *cell = m_emitter.call_create_cell(nullptr);
		auto *cell_alloca = m_codegen_ctx.create_local(cv + ".cell", ptr_ty);
		m_builder.CreateStore(cell, cell_alloca);
		m_codegen_ctx.register_cell(cv, cell_alloca);
	}

	// === Step 7: 从 closure tuple 提取 FREE 变量的 cell ===
	for (size_t i = 0; i < free_vars.size(); ++i) {
		auto *cell =
			m_emitter.call_tuple_getitem(closure_arg, m_builder.getInt32(static_cast<int32_t>(i)));
		auto *free_alloca = m_codegen_ctx.create_local(free_vars[i] + ".free", ptr_ty);
		m_builder.CreateStore(cell, free_alloca);
		m_codegen_ctx.register_free_var(free_vars[i], free_alloca);
	}

	// === Step 7.5: 从 closure 提取 defaults tuple ===
	// defaults 放在 free vars 之后的位置
	llvm::Value *defaults_in_body = nullptr;
	if (defaults_tuple) {
		int32_t defaults_idx = static_cast<int32_t>(free_vars.size());
		defaults_in_body =
			m_emitter.call_tuple_getitem(closure_arg, m_builder.getInt32(defaults_idx));
	}

	// === Step 8: 提取函数参数 — Python 3.9 参数绑定语义 ===
	//
	// Python 3.9 参数解析优先级:
	//   1. 位置参数 args[index]
	//   2. 关键字参数 kwargs[name]  (暂不支持，Phase 3+)
	//   3. 默认值 defaults[index - defaults_start]
	//   4. TypeError: missing required argument
	//
	// 默认值对齐规则 (Python 3.9):
	//   def f(a, b, c=1, d=2):  # 4 params, 2 defaults
	//   defaults 对齐到最后 N 个参数:
	//     a → 无默认值 (index 0)
	//     b → 无默认值 (index 1)
	//     c → defaults[0] (index 2, defaults_start=2)
	//     d → defaults[1] (index 3, defaults_start=2)
	if (args_node) {
		int32_t param_idx = 0;
		int32_t total_params = static_cast<int32_t>(args_node->args().size());
		int32_t n_defaults = static_cast<int32_t>(defaults_count);
		int32_t defaults_start = total_params - n_defaults;

		for (const auto &arg : args_node->args()) {
			const auto &arg_name = arg->name();
			llvm::Value *val = nullptr;

			// Python 3.9 语义：动态检查当前参数是否由 kwargs 提供，或者是否超出了 argc
			auto *idx_val = m_builder.getInt32(param_idx);

			// has_pos_arg = (param_idx < argc)
			auto *has_pos_arg =
				m_builder.CreateICmpSLT(idx_val, argc_arg, arg_name + ".has_pos_arg");

			if (param_idx >= defaults_start && defaults_in_body) {
				// === 有默认值的参数 ===
				auto *from_args_bb = llvm::BasicBlock::Create(m_ctx, arg_name + ".from_args", func);
				auto *from_default_bb =
					llvm::BasicBlock::Create(m_ctx, arg_name + ".from_default", func);
				auto *bind_done_bb = llvm::BasicBlock::Create(m_ctx, arg_name + ".done", func);

				m_builder.CreateCondBr(has_pos_arg, from_args_bb, from_default_bb);

				// --- 从 C 数组 取值 ---
				m_builder.SetInsertPoint(from_args_bb);
				// 使用新增的 call_value_array_get
				auto *val_from_args = m_emitter.call_value_array_get(args_array_arg, idx_val);
				m_builder.CreateBr(bind_done_bb);
				auto *from_args_exit = m_builder.GetInsertBlock();

				// --- 使用默认值 ---
				m_builder.SetInsertPoint(from_default_bb);
				// 注意：如果有 kwargs，这里应该先查 kwargs，为了简化我们假设 fast_path 不带 kwargs
				int32_t def_idx = param_idx - defaults_start;
				auto *val_from_default =
					m_emitter.call_tuple_getitem(defaults_in_body, m_builder.getInt32(def_idx));
				m_builder.CreateBr(bind_done_bb);
				auto *from_default_exit = m_builder.GetInsertBlock();

				// --- PHI 合并 ---
				m_builder.SetInsertPoint(bind_done_bb);
				auto *phi = m_builder.CreatePHI(ptr_ty, 2, arg_name + ".val");
				phi->addIncoming(val_from_args, from_args_exit);
				phi->addIncoming(val_from_default, from_default_exit);
				val = phi;
			} else {
				// === 无默认值的必填参数 ===
				// 如果 has_pos_arg 为假（且没在 kwargs 里），必须抛出 TypeError (缺失参数)
				// TODO: 完整的 IR 这里应该分支抛出
				// TypeError，目前为了性能假设上层调用合法，或依赖越界读取引发段错误被捕获
				val = m_emitter.call_value_array_get(args_array_arg, idx_val);
			}

			// 存储到局部变量或 cell
			auto vis = m_codegen_ctx.get_visibility(arg_name);
			if (vis && *vis == Visibility::CELL) {
				auto *cell_alloca = m_codegen_ctx.find_cell(arg_name);
				if (cell_alloca) {
					auto *cell = m_builder.CreateLoad(ptr_ty, cell_alloca);
					m_emitter.call_cell_set(cell, val);
				}
			} else {
				auto *alloca = m_codegen_ctx.create_local(arg_name, ptr_ty);
				m_builder.CreateStore(val, alloca);
			}

			param_idx++;
		}
	}

	// === Step 9: 生成函数体 ===
	generate_body(body);

	// === Step 10: 确保有终结器 ===
	auto *current_bb = m_builder.GetInsertBlock();
	if (!current_bb->getTerminator()) { m_builder.CreateRet(m_emitter.get_none()); }

	// === Step 11: 离开作用域 ===
	m_codegen_ctx.pop_scope();

	// === Step 12: 恢复插入点 ===
	m_builder.restoreIP(saved_ip);
	m_func_first_args.pop_back();

	// === Step 13: 构建 closure tuple ===
	// 布局: [ cell_for_free_var_0, ..., cell_for_free_var_N-1, defaults_tuple? ]
	// defaults_tuple 紧跟在所有 free var cell 之后
	llvm::Value *closure_val = m_emitter.null_pyobject();
	if (has_closure) {
		std::vector<llvm::Value *> closure_elems;

		// 13a. free vars — 从外围作用域取 cell
		for (const auto &fv : free_vars) {
			llvm::Value *cell = nullptr;
			if (auto *cell_alloca = m_codegen_ctx.find_cell(fv)) {
				cell = m_builder.CreateLoad(ptr_ty, cell_alloca);
			} else if (auto *free_alloca = m_codegen_ctx.find_free_var(fv)) {
				cell = m_builder.CreateLoad(ptr_ty, free_alloca);
			} else {
				log::codegen()->error("Cannot find cell/free for '{}'", fv);
				cell = m_emitter.get_none();
			}
			closure_elems.push_back(cell);
		}

		// 13b. defaults tuple（如果有）
		if (defaults_tuple) { closure_elems.push_back(defaults_tuple); }

		closure_val = m_emitter.create_tuple(closure_elems);
	}

	// === Step 14: 编译 kwdefaults ===
	// Python 3.9: keyword-only 参数的默认值存储为 dict
	// def f(*, key=val):  →  kwdefaults = {"key": val}
	llvm::Value *kwdefaults_val = nullptr;
	if (args_node && !args_node->kw_defaults().empty()) {
		std::vector<llvm::Value *> keys;
		std::vector<llvm::Value *> vals;
		for (size_t i = 0; i < args_node->kw_defaults().size(); ++i) {
			auto &kw_def = args_node->kw_defaults()[i];
			if (kw_def) {
				auto *val = generate(kw_def.get());
				if (val) {
					auto &kw_arg = args_node->kwonlyargs()[i];
					keys.push_back(m_emitter.create_string(kw_arg->name()));
					vals.push_back(val);
				}
			}
		}
		if (!keys.empty()) { kwdefaults_val = m_emitter.create_dict(keys, vals); }
	}

	// === Step 15: rt_make_function ===
	// defaults_tuple 同时传给 make_function 用于内省（inspect.signature 等）
	auto *mod = m_codegen_ctx.module_object();
	auto *func_val = m_emitter.call_make_function(func_name,
		func,
		mod,
		defaults_tuple ? defaults_tuple : m_emitter.null_pyobject(),
		kwdefaults_val ? kwdefaults_val : m_emitter.null_pyobject(),
		closure_val);

	// === Step 16: 装饰器 ===
	// Python 3.9: 装饰器从内到外（逆序）应用
	//   @d1 @d2 def f: ...  →  f = d1(d2(f))
	llvm::Value *decorated = func_val;
	for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) {
		auto *dec = generate(it->get());
		if (!dec) { continue; }
		auto *dec_args = m_emitter.create_tuple({ decorated });
		decorated = m_emitter.call_function(dec, dec_args, nullptr);
	}

	return decorated;
}

ast::Value *PylangCodegen::visit(const ast::FunctionDefinition *node)
{
	auto *func_val = compile_function_body(node->name(),
		node->args().get(),
		node->body(),
		node->decorator_list(),// ← Step 15 已经应用了装饰器！
		node->source_location());

	if (!func_val) { return nullptr; }

	// 修复: 删除重复的装饰器应用！compile_function_body Step 15 已经处理了
	// const auto &decorators = node->decorator_list();
	// for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) { ... }

	// 直接存储
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
	if (node->exception()) {
		auto *exc = generate(node->exception().get());
		m_emitter.call_raise(exc);
	} else {
		if (auto *try_ctx = m_codegen_ctx.current_try()) {
			auto *exc = m_builder.CreateLoad(
				m_emitter.pyobject_ptr_type(), try_ctx->exc_alloca, "reraise.exc");
			m_emitter.call_reraise(exc);
		} else {
			// 使用 call_reraise(null)
			m_emitter.call_reraise(nullptr);
		}
	}

	// call_raise / call_reraise 调用的是 noreturn 函数。
	// - 非 try 上下文: emit_runtime_call 生成 call，LLVM 将其视为 terminator
	//   → 不能再加 unreachable
	// - try 上下文: emit_runtime_call 生成 invoke（terminator），
	//   然后 SetInsertPoint 到 invoke.cont BB（空的，无 terminator）
	//   → 需要加 unreachable
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateUnreachable(); }

	// 创建一个新的 BB 供后续死代码使用（防止后续 visit 往已终止的 BB 写入）
	auto *dead_bb = llvm::BasicBlock::Create(m_ctx, "raise.dead", m_codegen_ctx.current_function());
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

	// - 非 try 上下文: call_raise 生成 noreturn call，已是 terminator，不能再加
	// - try 上下文: call_raise 生成 invoke，SetInsertPoint 到 invoke.cont（空 BB），需要加
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateUnreachable(); }

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
	auto *i32_ty = m_builder.getInt32Ty();
	auto *val_ptr_ty = m_builder.getPtrTy();// 对应 py::Value*

	// 修改签名符合 AOTRawFuncPtr:
	// PyObject* (PyObject* module, PyObject* closure, const Value* args, int32_t argc, PyDict*
	// kwargs)
	auto *func_type = llvm::FunctionType::get(
		obj_ptr_ty, { obj_ptr_ty, obj_ptr_ty, val_ptr_ty, i32_ty, obj_ptr_ty }, false);

	auto mangled = m_mangler.function_mangle(
		m_codegen_ctx.current_scope().mangled_name, class_name, source_loc);

	auto *llvm_func = llvm::Function::Create(
		func_type, llvm::Function::InternalLinkage, mangled + ".__body__", m_module.get());

	ensure_personality(llvm_func);

	auto arg_it = llvm_func->arg_begin();
	auto *module_arg = &*arg_it++;
	module_arg->setName("module");
	auto *closure_arg = &*arg_it++;
	closure_arg->setName("closure");
	auto *args_ptr_arg = &*arg_it++;
	args_ptr_arg->setName("args_ptr");
	auto *argc_arg = &*arg_it++;
	argc_arg->setName("argc");
	auto *kwargs_arg = &*arg_it++;
	kwargs_arg->setName("kwargs");

	auto *saved_bb = m_builder.GetInsertBlock();
	auto *entry_bb = llvm::BasicBlock::Create(m_ctx, "entry", llvm_func);
	m_builder.SetInsertPoint(entry_bb);

	// 从 Value* 数组提取 namespace: ns = args_ptr[0]
	// 使用新的 emitter 接口（需在 IREmitter 中实现读取 Value 的逻辑）
	auto *ns = m_emitter.call_value_array_get(args_ptr_arg, m_builder.getInt32(0));
	ns->setName("ns");

	// =================================================================
	// 查找 VariablesResolver 中的 CLASS scope
	// =================================================================
	VariablesResolver::Scope *var_scope = nullptr;
	auto *parent_var_scope = m_codegen_ctx.current_scope().var_scope;

	if (parent_var_scope) {
		// 方法 1: 构造预期的 namespace_ 并在 visibility map 中匹配
		std::string expected_ns = parent_var_scope->namespace_ + "." + class_name;
		for (auto &[key, scope_ptr] : m_codegen_ctx.visibility_map()) {
			if (scope_ptr && scope_ptr->type == VariablesResolver::Scope::Type::CLASS
				&& scope_ptr->namespace_ == expected_ns) {
				var_scope = scope_ptr.get();
				break;
			}
		}
	}

	if (!var_scope) {
		// 方法 2: 退回到 mangled name 精确匹配
		auto it = m_codegen_ctx.visibility_map().find(mangled);
		if (it != m_codegen_ctx.visibility_map().end()) { var_scope = it->second.get(); }
	}

	if (!var_scope) {
		// 方法 3: 暴力搜索——namespace_ 以 "." + class_name 结尾的 CLASS scope
		std::string suffix = "." + class_name;
		for (auto &[key, scope_ptr] : m_codegen_ctx.visibility_map()) {
			if (scope_ptr && scope_ptr->type == VariablesResolver::Scope::Type::CLASS
				&& scope_ptr->namespace_.size() >= suffix.size()
				&& scope_ptr->namespace_.compare(
					   scope_ptr->namespace_.size() - suffix.size(), suffix.size(), suffix)
					   == 0) {
				var_scope = scope_ptr.get();
				break;
			}
		}
	}

	if (!var_scope) {
		log::codegen()->warn("compile_class_body: no var_scope found for class '{}'", class_name);
	}

	ScopeContext class_scope{};
	class_scope.type = ScopeContext::Type::CLASS;
	class_scope.name = class_name;
	class_scope.mangled_name = mangled;
	class_scope.llvm_func = llvm_func;
	class_scope.module_obj = llvm_func->getArg(0);
	class_scope.var_scope = var_scope;
	class_scope.class_namespace = ns;
	m_codegen_ctx.push_scope(std::move(class_scope));

	// =================================================================
	// 初始化 Cell 变量 (特别是 __class__)
	// 如果类使用了 super()，VariablesResolver 会将 __class__ 标记为 CELL
	// =================================================================
	if (var_scope) {
		std::vector<std::string> cell_vars;
		std::vector<std::string> free_vars;
		m_codegen_ctx.collect_cell_and_free_vars(var_scope, cell_vars, free_vars);

		for (const auto &cv : cell_vars) {
			// 1. 调用 runtime 创建空 cell
			auto *cell = m_emitter.call_create_cell(nullptr);
			// 2. 在栈上分配空间存储 cell 指针
			auto *cell_alloca = m_codegen_ctx.create_local(cv + ".cell", obj_ptr_ty);
			m_builder.CreateStore(cell, cell_alloca);
			// 3. 注册到 Context，供方法闭包捕获
			m_codegen_ctx.register_cell(cv, cell_alloca);

			log::codegen()->debug("Allocated cell for class variable: {}", cv);
		}
	}

	// 生成类体代码
	generate_body(body);

	// =================================================================
	// 将 __class__ cell 注入到 namespace 中
	// 这允许 metaclass (type.__new__) 获取该 cell 并绑定类对象
	// =================================================================
	if (auto *cell_alloca = m_codegen_ctx.find_cell("__class__")) {
		auto *cell_obj = m_builder.CreateLoad(obj_ptr_ty, cell_alloca, "__class__.cell.load");

		// 使用 call_dict_setitem_str
		// 之前手动 emit_runtime_call 传入了 PyString 对象(PyObject*)，
		// 而 dict_setitem_str 运行时函数期望的是 C-Style 字符串(char*)。
		// 这导致 Key 变成乱码，rt_build_class 无法找到 __classcell__，导致 cell 未初始化。
		m_emitter.call_dict_setitem_str(ns, "__classcell__", cell_obj);

		log::codegen()->debug("Injected __classcell__ into namespace for '{}'", class_name);
	}

	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateRet(m_emitter.get_none()); }

	m_codegen_ctx.pop_scope();
	m_builder.SetInsertPoint(saved_bb);

	// 对于简单的类定义（非嵌套），通常不需要 closure。
	// 如果将来支持嵌套类捕获外部变量，这里需要传入 closure tuple。
	return m_emitter.call_make_function(class_name,
		llvm_func,
		m_codegen_ctx.module_object(),
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
	// --- Step 1: 编译类体函数 ---
	auto *body_fn = compile_class_body(node->name(), node->body(), node->source_location());
	if (!body_fn) { return nullptr; }

	// --- Step 2: 收集基类 ---
	std::vector<llvm::Value *> base_vals;
	for (const auto &base : node->bases()) {
		auto *val = generate(base.get());
		if (val) { base_vals.push_back(val); }
	}
	auto *bases_tuple = m_emitter.create_tuple(base_vals);

	// --- Step 3: 收集 keywords（metaclass= 等）---
	llvm::Value *kwargs_dict = nullptr;
	bool has_kwargs = false;

	for (const auto &kw : node->keywords()) {
		auto *kw_node = dynamic_cast<const ast::Keyword *>(kw.get());
		if (!kw_node) { continue; }

		auto *kw_value = generate(kw_node->value().get());
		if (!kw_value) { continue; }

		if (!kwargs_dict) {
			kwargs_dict = m_emitter.create_dict({}, {});
			has_kwargs = true;
		}

		if (!kw_node->arg().has_value()) {
			// **extra → 展开到 kwargs dict
			// 使用封装函数
			m_emitter.call_dict_merge(kwargs_dict, kw_value);
		} else {
			// metaclass=Meta 等命名关键字
			// [CRITICAL FIX] 修复 ABI 不匹配!
			// 之前: m_emitter.emit_runtime_call("dict_setitem_str", { kwargs, create_string(...),
			// val }) create_string 返回 PyObject*，而 dict_setitem_str 期望 const char*
			m_emitter.call_dict_setitem_str(kwargs_dict, *kw_node->arg(), kw_value);
		}
	}

	if (!has_kwargs) { kwargs_dict = m_emitter.null_pyobject(); }

	// --- Step 4: 调用 rt_build_class_aot ---
	// 使用封装函数，自动处理 name 字符串转换
	auto *cls = m_emitter.call_build_class_aot(body_fn, node->name(), bases_tuple, kwargs_dict);

	// --- Step 5: 应用装饰器 ---
	const auto &decorators = node->decorator_list();
	llvm::Value *decorated = cls;
	for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) {
		auto *dec = generate(it->get());
		if (!dec) { continue; }
		auto *dec_args = m_emitter.create_tuple({ decorated });
		// 使用 call_function
		decorated = m_emitter.call_function(dec, dec_args, nullptr);
	}

	// --- Step 6: 存储到变量 ---
	store_variable(node->name(), decorated);
	return nullptr;
}
// =============================================================================
// visit() 实现 — 未实现的节点 (Phase 3+)
// =============================================================================


void PylangCodegen::ensure_personality(llvm::Function *func)
{
	if (!func->hasPersonalityFn()) { func->setPersonalityFn(m_emitter.get_personality_function()); }
	// 确保有 uwtable —— 否则 .eh_frame 不生成，landingpad 无效
	if (!func->hasFnAttribute(llvm::Attribute::UWTable)) {
		func->addFnAttr(llvm::Attribute::UWTable);
	}
}

// void PylangCodegen::emit_finally_dispatch(const TryContext &try_ctx,
// 	llvm::BasicBlock *merge_bb,
// 	llvm::BasicBlock *reraise_bb)
// {
// 	auto *state = m_builder.CreateLoad(m_builder.getInt32Ty(), try_ctx.state_alloca, "state");

// 	auto *func = m_codegen_ctx.current_function();
// 	auto *do_return_bb = llvm::BasicBlock::Create(m_ctx, "finally.return", func);

// 	// switch on state: 0=normal→merge, 1=exception→reraise, 2=return→do_return
// 	auto *sw = m_builder.CreateSwitch(state, merge_bb, 3);
// 	sw->addCase(m_builder.getInt32(0), merge_bb);
// 	sw->addCase(m_builder.getInt32(1), reraise_bb);
// 	sw->addCase(m_builder.getInt32(2), do_return_bb);

// 	// break/continue: 如果在循环中
// 	if (auto *loop = m_codegen_ctx.current_loop()) {
// 		auto *do_break_bb = llvm::BasicBlock::Create(m_ctx, "finally.break", func);
// 		auto *do_cont_bb = llvm::BasicBlock::Create(m_ctx, "finally.continue", func);
// 		sw->addCase(m_builder.getInt32(3), do_break_bb);
// 		sw->addCase(m_builder.getInt32(4), do_cont_bb);

// 		m_builder.SetInsertPoint(do_break_bb);
// 		m_builder.CreateBr(loop->break_bb);

// 		m_builder.SetInsertPoint(do_cont_bb);
// 		m_builder.CreateBr(loop->continue_bb);
// 	}

// 	// return path
// 	m_builder.SetInsertPoint(do_return_bb);
// 	auto *retval =
// 		m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), try_ctx.retval_alloca, "retval");
// 	m_builder.CreateRet(retval);
// }

void PylangCodegen::emit_finally_dispatch(const TryContext &try_ctx,
	llvm::BasicBlock *merge_bb,
	llvm::BasicBlock *reraise_bb)
{
	auto *state = m_builder.CreateLoad(m_builder.getInt32Ty(), try_ctx.state_alloca, "state");

	auto *func = m_codegen_ctx.current_function();
	auto *ret_ty = func->getReturnType();

	// state==2 (return) 路径只在非 void 函数中才有意义
	// 模块顶层函数返回 void，不可能有 return <value>
	bool has_return_path = !ret_ty->isVoidTy();

	// 计算 switch case 数量
	int num_cases = 2;// 0=normal, 1=reraise
	if (has_return_path) { num_cases++; }
	if (m_codegen_ctx.current_loop()) { num_cases += 2; }// 3=break, 4=continue

	auto *sw = m_builder.CreateSwitch(state, merge_bb, num_cases);
	sw->addCase(m_builder.getInt32(0), merge_bb);
	sw->addCase(m_builder.getInt32(1), reraise_bb);

	if (has_return_path) {
		auto *do_return_bb = llvm::BasicBlock::Create(m_ctx, "finally.return", func);
		sw->addCase(m_builder.getInt32(2), do_return_bb);

		m_builder.SetInsertPoint(do_return_bb);
		auto *retval =
			m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), try_ctx.retval_alloca, "retval");
		m_builder.CreateRet(retval);
	}

	if (auto *loop = m_codegen_ctx.current_loop()) {
		auto *do_break_bb = llvm::BasicBlock::Create(m_ctx, "finally.break", func);
		auto *do_cont_bb = llvm::BasicBlock::Create(m_ctx, "finally.continue", func);
		sw->addCase(m_builder.getInt32(3), do_break_bb);
		sw->addCase(m_builder.getInt32(4), do_cont_bb);

		m_builder.SetInsertPoint(do_break_bb);
		m_builder.CreateBr(loop->break_bb);

		m_builder.SetInsertPoint(do_cont_bb);
		m_builder.CreateBr(loop->continue_bb);
	}
}

// =============================================================================
// visit(Try*) — 完整 Python 3.9 语义
//
// IR 结构:
//
//   ; allocas for state tracking
//   %state = alloca i32        ; 0=normal, 1=exc, 2=return, 3=break, 4=continue
//   %retval = alloca ptr
//   %exc_obj = alloca ptr
//   %lp_token = alloca { ptr, i32 }
//   store i32 0, ptr %state
//
//   ; try body — all calls use invoke → unwind to %landingpad
//   try.body:
//     invoke ... to %cont unwind %landingpad
//     ...
//     store i32 0, ptr %state       ; normal exit
//     br label %try.else (or %try.finally)
//
//   ; landingpad — catch PylangException
//   landingpad:
//     %lp = landingpad { ptr, i32 } catch ptr null
//     store { ptr, i32 } %lp, ptr %lp_token
//     %exc_ptr = extractvalue { ptr, i32 } %lp, 0
//     %exc = call ptr @rt_catch_begin(ptr %exc_ptr)
//     store ptr %exc, ptr %exc_obj
//     store i32 1, ptr %state
//     br label %except.dispatch
//
//   ; except dispatch — match handlers
//   except.dispatch:
//     %e = load ptr, ptr %exc_obj
//     ; for each handler:
//     %match = call i1 @rt_check_exception_match(%e, %type)
//     br i1 %match, label %handler_N, label %next_check
//
//   handler_N:
//     ; bind "as" variable
//     ; execute handler body
//     call void @rt_catch_end()      ; end catch (matched)
//     store i32 0, ptr %state        ; handled → normal
//     br label %try.finally (or %merge)
//
//   except.nomatch:
//     call void @rt_catch_end()      ; end catch (unmatched)
//     ; state stays 1 → finally will reraise
//     br label %try.finally
//
//   try.else:
//     ; else body
//     br label %try.finally
//
//   try.finally:
//     ; finally body (always runs)
//     ; then dispatch on state
//
//   finally.reraise:
//     %lp_saved = load { ptr, i32 }, ptr %lp_token
//     resume { ptr, i32 } %lp_saved
//
//   try.merge:
//     ; continue execution
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::Try *node)
{
	auto *func = m_codegen_ctx.current_function();
	ensure_personality(func);

	auto *obj_ptr_ty = m_emitter.pyobject_ptr_type();
	auto *i32_ty = m_builder.getInt32Ty();

	// 构造 landingpad 返回类型: { ptr, i32 }
	auto *lp_ty = llvm::StructType::get(m_ctx, { m_builder.getPtrTy(), i32_ty });

	// --- Allocas (在函数入口) ---
	auto *state_alloca = m_codegen_ctx.create_local("__try_state", i32_ty);
	auto *retval_alloca = m_codegen_ctx.create_local("__try_retval", obj_ptr_ty);
	auto *exc_alloca = m_codegen_ctx.create_local("__try_exc", obj_ptr_ty);

	// lp_token 需要手动创建 alloca (非 PyObject* 类型)
	auto &entry_bb = func->getEntryBlock();
	llvm::IRBuilder<> entry_builder(&entry_bb, entry_bb.begin());
	auto *lp_alloca = entry_builder.CreateAlloca(lp_ty, nullptr, "__try_lp");

	m_builder.CreateStore(m_builder.getInt32(0), state_alloca);
	m_builder.CreateStore(llvm::ConstantPointerNull::get(m_builder.getPtrTy()), exc_alloca);

	// --- 创建 BBs ---
	auto *try_body_bb = llvm::BasicBlock::Create(m_ctx, "try.body", func);
	auto *landingpad_bb = llvm::BasicBlock::Create(m_ctx, "try.landingpad", func);
	auto *except_dispatch_bb = node->handlers().empty()
								   ? nullptr
								   : llvm::BasicBlock::Create(m_ctx, "except.dispatch", func);
	auto *else_bb =
		node->orelse().empty() ? nullptr : llvm::BasicBlock::Create(m_ctx, "try.else", func);
	auto *finally_bb =
		node->finalbody().empty() ? nullptr : llvm::BasicBlock::Create(m_ctx, "try.finally", func);
	auto *reraise_bb = llvm::BasicBlock::Create(m_ctx, "try.reraise", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "try.merge", func);

	bool has_finally = (finally_bb != nullptr);

	auto *after_try_normal = else_bb ? else_bb : (finally_bb ? finally_bb : merge_bb);
	auto *after_handler = finally_bb ? finally_bb : merge_bb;
	auto *after_else = finally_bb ? finally_bb : merge_bb;

	// --- 进入 try body ---
	m_builder.CreateBr(try_body_bb);
	m_builder.SetInsertPoint(try_body_bb);

	// Push TryContext — 所有后续 emit_call 自动使用 invoke
	TryContext try_ctx{};
	try_ctx.landingpad_bb = landingpad_bb;
	try_ctx.finally_bb = finally_bb;
	try_ctx.state_alloca = state_alloca;
	try_ctx.retval_alloca = retval_alloca;
	try_ctx.lp_alloca = lp_alloca;
	try_ctx.exc_alloca = exc_alloca;
	m_codegen_ctx.push_try(try_ctx);
	// 关键：让 IREmitter 所有调用（包括 call_binary_add 等）自动 invoke
	auto *prev_unwind = m_emitter.unwind_dest();
	m_emitter.set_unwind_dest(landingpad_bb);

	generate_body(node->body());

	m_emitter.set_unwind_dest(prev_unwind);// 恢复（支持嵌套 try）
	m_codegen_ctx.pop_try();

	// try body 正常结束
	if (!m_builder.GetInsertBlock()->getTerminator()) {
		m_builder.CreateStore(m_builder.getInt32(0), state_alloca);
		m_builder.CreateBr(after_try_normal);
	}

	// --- Landingpad ---
	m_builder.SetInsertPoint(landingpad_bb);
	auto *lp = m_builder.CreateLandingPad(lp_ty, 1, "lp");
	lp->addClause(m_emitter.get_pylang_exception_typeinfo());

	// 保存 LP token (用于 resume)
	m_builder.CreateStore(lp, lp_alloca);

	// 提取异常指针并调用 rt_catch_begin
	auto *exc_ptr = m_builder.CreateExtractValue(lp, 0, "exc.ptr");
	auto *py_exc = m_emitter.call_catch_begin(exc_ptr);
	m_builder.CreateStore(py_exc, exc_alloca);
	m_builder.CreateStore(m_builder.getInt32(1), state_alloca);

	if (except_dispatch_bb) {
		m_builder.CreateBr(except_dispatch_bb);
	} else {
		// 无 except handler: catch_end 然后去 finally 或 reraise
		// m_emitter.call_catch_end();
		m_emitter.call_catch_end();
		if (has_finally) {
			// state=1, finally dispatch 会走 reraise
			m_builder.CreateBr(finally_bb);
		} else {
			// 无 finally 也无 handler: 直接 resume
			auto *saved_lp = m_builder.CreateLoad(lp_ty, lp_alloca, "nohandler.saved.lp");
			m_builder.CreateResume(saved_lp);
		}
	}

	// --- Except dispatch ---
	if (except_dispatch_bb) {
		m_builder.SetInsertPoint(except_dispatch_bb);
		auto *exc = m_builder.CreateLoad(obj_ptr_ty, exc_alloca, "caught.exc");

		llvm::BasicBlock *current_check_bb = except_dispatch_bb;

		for (size_t i = 0; i < node->handlers().size(); ++i) {
			auto *handler_node =
				dynamic_cast<const ast::ExceptHandler *>(node->handlers()[i].get());
			if (!handler_node) { continue; }

			auto *handler_body_bb =
				llvm::BasicBlock::Create(m_ctx, fmt::format("except.{}", i), func);

			if (handler_node->type()) {
				// except SpecificType as name:
				auto *next_bb = (i + 1 < node->handlers().size())
									? llvm::BasicBlock::Create(
										  m_ctx, fmt::format("except.check.{}", i + 1), func)
									: llvm::BasicBlock::Create(m_ctx, "except.nomatch", func);

				m_builder.SetInsertPoint(current_check_bb);
				auto *exc_type = generate(handler_node->type().get());
				auto *match = m_emitter.call_check_exception_match(exc, exc_type);
				m_builder.CreateCondBr(match, handler_body_bb, next_bb);

				current_check_bb = next_bb;
			} else {
				// bare except: (匹配所有)
				m_builder.SetInsertPoint(current_check_bb);
				m_builder.CreateBr(handler_body_bb);
				current_check_bb = nullptr;
			}

			// Handler body
			m_builder.SetInsertPoint(handler_body_bb);
			if (!handler_node->name().empty()) { store_variable(handler_node->name(), exc); }
			generate_body(handler_node->body());

			if (!m_builder.GetInsertBlock()->getTerminator()) {
				// 异常已处理: catch_end + state=0
				m_emitter.call_catch_end();
				m_builder.CreateStore(m_builder.getInt32(0), state_alloca);
				m_builder.CreateBr(after_handler);
			}
		}

		// =============================================================
		// 修复: except.nomatch — 没有 handler 匹配时必须重新抛出！
		//
		// 修复前: m_builder.CreateBr(after_handler)
		//   → 无 finally 时 after_handler = merge_bb → 异常被吞掉！
		//
		// 修复后:
		//   - 有 finally: 跳 finally_bb, state=1 → finally dispatch → reraise
		//   - 无 finally: 直接 resume landingpad token
		// =============================================================
		if (current_check_bb) {
			m_builder.SetInsertPoint(current_check_bb);
			m_emitter.call_catch_end();
			// state 已经是 1（在 landingpad 中设置）

			if (has_finally) {
				// 有 finally: state 保持 1, finally dispatch 走 reraise 路径
				m_builder.CreateBr(finally_bb);
			} else {
				// 无 finally: 直接 resume 原始 landingpad token 重新抛出
				auto *saved_lp = m_builder.CreateLoad(lp_ty, lp_alloca, "nomatch.saved.lp");
				m_builder.CreateResume(saved_lp);
			}
		}
	}

	// --- Else block ---
	if (else_bb) {
		m_builder.SetInsertPoint(else_bb);
		generate_body(node->orelse());
		if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(after_else); }
	}

	// --- Finally block ---
	if (finally_bb) {
		m_builder.SetInsertPoint(finally_bb);
		generate_body(node->finalbody());

		if (!m_builder.GetInsertBlock()->getTerminator()) {
			emit_finally_dispatch(try_ctx, merge_bb, reraise_bb);
		}
	}

	// --- Reraise ---
	m_builder.SetInsertPoint(reraise_bb);
	auto *saved_lp = m_builder.CreateLoad(lp_ty, lp_alloca, "saved.lp");
	m_builder.CreateResume(saved_lp);

	// --- Merge ---
	m_builder.SetInsertPoint(merge_bb);
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::ExceptHandler *node)
{
	// 确认: 这是预期的。
	// ExceptHandler 节点总是由 visit(Try*) 在内部显式处理（遍历 handlers 并生成 body），
	// 不会通过标准的 generate() 递归分发机制被独立访问。
	// 如果此函数被调用，说明 AST 结构异常或逻辑错误。
	// (void)node; // 防止未使用警告
	// return nullptr;

	(void)node;
	log::codegen()->warn("ExceptHandler visited directly (should be handled by Try)");
	return nullptr;
}


// =============================================================================
// 辅助：序列解包（处理 Starred）
// =============================================================================
void PylangCodegen::store_to_sequence_target(
	const std::vector<std::shared_ptr<ast::ASTNode>> &elements,
	llvm::Value *value)
{
	// 检测是否有 Starred 元素
	int32_t starred_idx = -1;
	for (int32_t i = 0; i < static_cast<int32_t>(elements.size()); ++i) {
		if (dynamic_cast<const ast::Starred *>(elements[i].get())) {
			if (starred_idx != -1) {
				log::codegen()->error("multiple starred expressions in assignment");
				return;
			}
			starred_idx = i;
		}
	}

	if (starred_idx == -1) {
		// 无星号：普通 unpack_sequence
		const int32_t count = static_cast<int32_t>(elements.size());
		auto *arr_alloca = m_builder.CreateAlloca(
			m_emitter.pyobject_ptr_type(), m_builder.getInt32(count), "unpack_buf");

		// 使用 call_unpack_sequence
		m_emitter.call_unpack_sequence(value, count, arr_alloca);

		for (int32_t i = 0; i < count; ++i) {
			auto *gep = m_builder.CreateConstGEP1_32(m_emitter.pyobject_ptr_type(), arr_alloca, i);
			auto *elem = m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), gep);
			store_to_target(elements[i].get(), elem);
		}

	} else {
		// 有星号：extended unpack
		// a, *b, c = iterable
		// before_count = starred_idx
		// after_count  = elements.size() - starred_idx - 1
		// out 数组大小 = before_count + 1(list) + after_count = elements.size()
		const int32_t before_count = starred_idx;
		const int32_t after_count = static_cast<int32_t>(elements.size()) - starred_idx - 1;
		const int32_t out_count = static_cast<int32_t>(elements.size());

		auto *arr_alloca = m_builder.CreateAlloca(
			m_emitter.pyobject_ptr_type(), m_builder.getInt32(out_count), "unpack_ex_buf");

		// m_emitter.emit_runtime_call("unpack_ex",
		// 	{ value,
		// 		m_builder.getInt32(before_count),
		// 		m_builder.getInt32(after_count),
		// 		arr_alloca });
		m_emitter.call_unpack_ex(value, before_count, after_count, arr_alloca);

		for (int32_t i = 0; i < out_count; ++i) {
			auto *gep = m_builder.CreateConstGEP1_32(m_emitter.pyobject_ptr_type(), arr_alloca, i);
			auto *elem = m_builder.CreateLoad(m_emitter.pyobject_ptr_type(), gep);

			const ast::ASTNode *actual_target = elements[i].get();
			if (auto *starred = dynamic_cast<const ast::Starred *>(actual_target)) {
				// *b → 存储到 starred 内部的 value（就是 list）
				store_to_target(starred->value().get(), elem);
			} else {
				store_to_target(actual_target, elem);
			}
		}
	}
}
void PylangCodegen::store_to_target(const ast::ASTNode *target, llvm::Value *value)
{
	if (auto *name = dynamic_cast<const ast::Name *>(target)) {
		// x = value
		store_variable(name->ids()[0], value);

	} else if (auto *attr = dynamic_cast<const ast::Attribute *>(target)) {
		// obj.attr = value
		auto *obj = generate(attr->value().get());
		if (obj) {
			// 旧代码: m_emitter.emit_runtime_call("setattr", { obj, create_string(...), value })
			// 错误: create_string 返回 PyObject*，rt_setattr 需要 const char*
			// 结果: Segfault 或 Key 乱码
			m_emitter.call_setattr(obj, attr->attr(), value);
		}

	} else if (auto *subscr = dynamic_cast<const ast::Subscript *>(target)) {
		auto *obj = generate(subscr->value().get());
		if (!obj) { return; }

		llvm::Value *key = nullptr;

		// 与 visit(Subscript*) 完全一致的访问方式
		if (std::holds_alternative<ast::Subscript::Index>(subscr->slice())) {
			auto &idx = std::get<ast::Subscript::Index>(subscr->slice());
			key = generate(idx.value.get());
		} else if (std::holds_alternative<ast::Subscript::Slice>(subscr->slice())) {
			auto &sl = std::get<ast::Subscript::Slice>(subscr->slice());
			auto *start = sl.lower ? generate(sl.lower.get()) : m_emitter.get_none();
			auto *stop = sl.upper ? generate(sl.upper.get()) : m_emitter.get_none();
			auto *step = sl.step ? generate(sl.step.get()) : m_emitter.get_none();
			// 使用 create_slice 封装
			key = m_emitter.create_slice(start, stop, step);
		} else {
			// ExtSlice: Phase 2 不支持
			log::codegen()->warn("store_to_target: ExtSlice not supported in Phase 2");
			return;
		}

		if (key) { m_emitter.call_setitem(obj, key, value); }

	} else if (auto *tuple = dynamic_cast<const ast::Tuple *>(target)) {
		// (a, b, *c, d) = value
		store_to_sequence_target(tuple->elements(), value);

	} else if (auto *list = dynamic_cast<const ast::List *>(target)) {
		// [a, b, *c, d] = value
		store_to_sequence_target(list->elements(), value);

	} else {
		log::codegen()->warn("store_to_target: unsupported target node type: {}",
			ast::node_type_to_string(target->node_type()));
	}
}

// =============================================================================
// visit(With*) — 异常感知的上下文管理器
//
// with expr as var:
//     body
//
// 等价于:
//   mgr = expr
//   exit = mgr.__exit__
//   value = mgr.__enter__()
//   var = value
//   exc_happened = False
//   try:
//       body
//   except:
//       exc_happened = True
//       if not exit(exc_type, exc_val, exc_tb):
//           raise
//   finally:
//       if not exc_happened:
//           exit(None, None, None)
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::With *node)
{
	auto *func = m_codegen_ctx.current_function();
	ensure_personality(func);

	auto *obj_ptr_ty = m_emitter.pyobject_ptr_type();
	auto *i32_ty = m_builder.getInt32Ty();
	auto *lp_ty = llvm::StructType::get(m_ctx, { m_builder.getPtrTy(), i32_ty });

	struct WithCleanup
	{
		llvm::AllocaInst *exit_alloca;
		llvm::AllocaInst *exc_happened_alloca;
	};

	std::vector<WithCleanup> cleanups;

	// --- 进入阶段 ---
	for (const auto &item : node->items()) {
		auto *with_item = static_cast<const ast::WithItem *>(item.get());

		auto *mgr = generate(with_item->context_expr().get());
		if (!mgr) { return nullptr; }

		// 修复: 使用 call_getattr 代替 emit_call("getattr", ...)，确保传入 const char*
		// exit = getattr(mgr, "__exit__")
		auto *exit_fn = m_emitter.call_getattr(mgr, "__exit__");

		auto *exit_alloca = m_codegen_ctx.create_local("__with_exit", obj_ptr_ty);
		m_builder.CreateStore(exit_fn, exit_alloca);

		// exc_happened = false
		auto *exc_happened = m_codegen_ctx.create_local("__with_exc", i32_ty);
		m_builder.CreateStore(m_builder.getInt32(0), exc_happened);

		cleanups.push_back({ exit_alloca, exc_happened });

		// 修复: 使用 call_getattr
		// enter = getattr(mgr, "__enter__"); value = call(enter)
		auto *enter_fn = m_emitter.call_getattr(mgr, "__enter__");

		auto *empty_args = m_emitter.create_tuple({});
		// 这里的 emit_call 是安全的，因为 call 函数期望的就是 PyObject* 参数
		auto *value = m_emitter.call_function(enter_fn, empty_args, nullptr);

		if (with_item->optional_vars()) {
			store_to_target(with_item->optional_vars().get(), value);
		}
	}

	// --- Try body (with内置异常处理) ---
	auto *body_bb = llvm::BasicBlock::Create(m_ctx, "with.body", func);
	auto *landingpad_bb = llvm::BasicBlock::Create(m_ctx, "with.landingpad", func);
	auto *normal_exit_bb = llvm::BasicBlock::Create(m_ctx, "with.normal_exit", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_ctx, "with.merge", func);

	auto *state_alloca = m_codegen_ctx.create_local("__with_state", i32_ty);
	auto *retval_alloca = m_codegen_ctx.create_local("__with_retval", obj_ptr_ty);

	// 在 entry block 创建 landingpad token alloca (防止在循环/分支中多次创建导致栈溢出)
	auto *lp_alloca_raw = [&]() {
		auto &entry = func->getEntryBlock();
		llvm::IRBuilder<> eb(&entry, entry.begin());
		return eb.CreateAlloca(lp_ty, nullptr, "__with_lp");
	}();

	auto *exc_alloca = m_codegen_ctx.create_local("__with_exc_obj", obj_ptr_ty);

	m_builder.CreateStore(m_builder.getInt32(0), state_alloca);
	m_builder.CreateBr(body_bb);
	m_builder.SetInsertPoint(body_bb);

	TryContext try_ctx{};
	try_ctx.landingpad_bb = landingpad_bb;
	try_ctx.finally_bb = nullptr;// with 不用 finally dispatch
	try_ctx.state_alloca = state_alloca;
	try_ctx.retval_alloca = retval_alloca;
	try_ctx.lp_alloca = lp_alloca_raw;
	try_ctx.exc_alloca = exc_alloca;
	m_codegen_ctx.push_try(try_ctx);

	// 关键：让 IREmitter 的所有 runtime call 在 body 内自动生成 invoke
	auto *prev_unwind = m_emitter.unwind_dest();
	m_emitter.set_unwind_dest(landingpad_bb);

	generate_body(node->body());

	m_emitter.set_unwind_dest(prev_unwind);// 恢复（支持嵌套）
	m_codegen_ctx.pop_try();

	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(normal_exit_bb); }

	// --- Landingpad: 异常发生 ---
	m_builder.SetInsertPoint(landingpad_bb);
	auto *lp = m_builder.CreateLandingPad(lp_ty, 1, "with.lp");
	lp->addClause(m_emitter.get_pylang_exception_typeinfo());// 使用正确的 typeinfo
	m_builder.CreateStore(lp, lp_alloca_raw);

	auto *exc_ptr = m_builder.CreateExtractValue(lp, 0, "with.exc.ptr");
	auto *py_exc = m_emitter.call_catch_begin(exc_ptr);
	m_builder.CreateStore(py_exc, exc_alloca);

	// 逆序调用 __exit__(exc_type, exc_val, exc_tb)
	for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
		m_builder.CreateStore(m_builder.getInt32(1), it->exc_happened_alloca);
		auto *exit_fn = m_builder.CreateLoad(obj_ptr_ty, it->exit_alloca, "exit_fn");

		//  __exit__(type(exc), exc, traceback)
		auto *exc_type = m_emitter.call_type_of(py_exc);
		auto *exc_tb = m_emitter.call_get_traceback(py_exc);
		auto *exit_args = m_emitter.create_tuple({ exc_type, py_exc, exc_tb });
		auto *suppress = m_emitter.call_function(exit_fn, exit_args, nullptr);
		// 如果 __exit__ 返回 truthy，抑制异常
		auto *is_suppressed = m_emitter.call_is_true(suppress);

		auto *reraise_bb = llvm::BasicBlock::Create(m_ctx, "with.reraise", func);
		auto *suppressed_bb = llvm::BasicBlock::Create(m_ctx, "with.suppressed", func);

		m_builder.CreateCondBr(is_suppressed, suppressed_bb, reraise_bb);

		// 抑制: catch_end + 继续
		m_builder.SetInsertPoint(suppressed_bb);
		m_emitter.call_catch_end();
		m_builder.CreateBr(merge_bb);

		// 不抑制: catch_end + resume
		m_builder.SetInsertPoint(reraise_bb);

		m_emitter.call_catch_end();
		auto *saved_lp = m_builder.CreateLoad(lp_ty, lp_alloca_raw, "with.saved_lp");
		m_builder.CreateResume(saved_lp);
	}

	// --- Normal exit: __exit__(None, None, None) ---
	m_builder.SetInsertPoint(normal_exit_bb);
	for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
		auto *exc_happened = m_builder.CreateLoad(i32_ty, it->exc_happened_alloca, "exc_happened");
		auto *was_exc = m_builder.CreateICmpNE(exc_happened, m_builder.getInt32(0));

		auto *call_exit_bb = llvm::BasicBlock::Create(m_ctx, "with.call_exit", func);
		auto *skip_exit_bb = llvm::BasicBlock::Create(m_ctx, "with.skip_exit", func);
		m_builder.CreateCondBr(was_exc, skip_exit_bb, call_exit_bb);

		m_builder.SetInsertPoint(call_exit_bb);
		auto *exit_fn = m_builder.CreateLoad(obj_ptr_ty, it->exit_alloca, "exit_fn");
		// 每个 BB 中独立调用 get_none()，不跨 BB 复用
		auto *none1 = m_emitter.get_none();
		auto *none2 = m_emitter.get_none();
		auto *none3 = m_emitter.get_none();
		auto *exit_args = m_emitter.create_tuple({ none1, none2, none3 });
		m_emitter.call_function(exit_fn, exit_args, nullptr);
		m_builder.CreateBr(skip_exit_bb);

		m_builder.SetInsertPoint(skip_exit_bb);
	}
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(merge_bb);
	return nullptr;
}

ast::Value *PylangCodegen::visit(const ast::WithItem *node)
{
	// WithItem 由 visit(With*) 内联处理，不应直接被调用
	(void)node;
	log::codegen()->warn("WithItem::codegen called directly (should be handled by With)");
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


// =============================================================================
// 推导式的通用编译框架
//
// 所有推导式 (ListComp, DictComp, SetComp, GeneratorExp) 共享相同结构:
//   1. 编译推导式体为匿名函数
//   2. 函数接收迭代器参数，内部创建容器并填充
//   3. 调用该函数并返回结果
//
// 等价 Python:
//   [expr for x in iter if cond]
//   →
//   def <listcomp>(.0):
//       result = []
//       for x in .0:
//           if cond:
//               result.append(expr)
//       return result
//   <listcomp>(iter(iterable))
// =============================================================================

ast::Value *PylangCodegen::visit(const ast::ListComp *node)
{
	// ... 计算 iterator ...
	auto *iterable = generate(node->generators()[0]->iter().get());
	auto *iterator = m_emitter.call_get_iter(iterable);

	auto *res = compile_comprehension_impl(
		"<listcomp>",
		node,
		iterator,
		[&]() { return m_emitter.create_list({}); },
		[&](llvm::Value *container) {
			auto *elt = generate(node->elt().get());
			if (elt) m_emitter.call_list_append(container, elt);
		});
	return make_value(res);
}

// =============================================================================
// 通用推导式循环生成器
//
// 递归处理嵌套的 for/if 子句:
//   for x in iter1:
//       for y in iter2:
//           if cond:
//               <body_emitter>()
// =============================================================================
void PylangCodegen::emit_comprehension_loops(
	const std::vector<std::shared_ptr<ast::Comprehension>> &generators,
	size_t gen_idx,
	llvm::Value *iterator,
	std::function<void()> body_emitter)
{
	auto *func = m_codegen_ctx.current_function();
	auto *gen = generators[gen_idx].get();

	// for 循环: while (has_value) { x = next(iter); ... }
	auto *loop_header = llvm::BasicBlock::Create(m_ctx, "comp.header", func);
	auto *loop_body = llvm::BasicBlock::Create(m_ctx, "comp.body", func);
	auto *loop_end = llvm::BasicBlock::Create(m_ctx, "comp.end", func);

	// 同理提升到入口安全区
	auto *has_value_alloca = m_codegen_ctx.create_local("comp_has_value", m_builder.getInt1Ty());

	m_builder.CreateBr(loop_header);
	m_builder.SetInsertPoint(loop_header);

	// next_val = iter_next(iterator, &has_value)
	auto *next_val = m_emitter.call_iter_next(iterator, has_value_alloca);
	auto *has_value = m_builder.CreateLoad(m_builder.getInt1Ty(), has_value_alloca);
	m_builder.CreateCondBr(has_value, loop_body, loop_end);

	m_builder.SetInsertPoint(loop_body);

	// 赋值给循环变量: x = next_val
	store_to_target(gen->target().get(), next_val);

	// 处理 if 过滤条件
	llvm::BasicBlock *innermost_bb = loop_body;
	for (const auto &if_clause : gen->ifs()) {
		auto *cond = generate(if_clause.get());
		auto *is_true = m_emitter.call_is_true(cond);

		auto *if_true_bb = llvm::BasicBlock::Create(m_ctx, "comp.if_true", func);
		auto *if_skip_bb = llvm::BasicBlock::Create(m_ctx, "comp.if_skip", func);

		m_builder.CreateCondBr(is_true, if_true_bb, if_skip_bb);

		// skip → 回到循环头
		m_builder.SetInsertPoint(if_skip_bb);
		m_builder.CreateBr(loop_header);

		m_builder.SetInsertPoint(if_true_bb);
		innermost_bb = if_true_bb;
	}

	// 嵌套 generator 或执行 body
	if (gen_idx + 1 < generators.size()) {
		// 嵌套: for y in iter2
		auto *inner_iterable = generate(generators[gen_idx + 1]->iter().get());
		auto *inner_iter = m_emitter.call_get_iter(inner_iterable);
		emit_comprehension_loops(generators, gen_idx + 1, inner_iter, body_emitter);
	} else {
		// 最内层: 执行 body（append/add/setitem）
		body_emitter();
	}

	// 回到循环头
	if (!m_builder.GetInsertBlock()->getTerminator()) { m_builder.CreateBr(loop_header); }

	m_builder.SetInsertPoint(loop_end);
}

llvm::Value *PylangCodegen::compile_comprehension_impl(const std::string &name,
	const ast::ASTNode *node,
	llvm::Value *iterator,
	std::function<llvm::Value *()> container_factory,// 创建容器
	std::function<void(llvm::Value *)> loop_body_gen// 填充逻辑 (参数是容器)
)
{
	auto *obj_ptr_ty = m_emitter.pyobject_ptr_type();

	// === Step 1: Mangle 名字 & 分析作用域 ===
	auto mangled = m_mangler.function_mangle(
		m_codegen_ctx.current_scope().mangled_name, name, node->source_location());

	// 查找 VariablesResolver 分析出的 Scope
	VariablesResolver::Scope *var_scope = nullptr;
	auto vis_it = m_codegen_ctx.visibility_map().find(mangled);
	if (vis_it != m_codegen_ctx.visibility_map().end()) { var_scope = vis_it->second.get(); }

	// 收集需要捕获的变量（Cell 和 Free）
	std::vector<std::string> cell_vars;
	std::vector<std::string> free_vars;
	m_codegen_ctx.collect_cell_and_free_vars(var_scope, cell_vars, free_vars);
	bool has_closure = !free_vars.empty();

	// === Step 2: 创建 LLVM 函数 ===
	// 签名: 统一使用 AOT raw ABI
	// PyObject* (PyObject* module, PyObject* closure, const Value* args_array, int32_t argc,
	// PyDict* kwargs) [修复]：强制占位 closure 并使用 value array + argc
	auto *val_ptr_ty = m_builder.getPtrTy();// 对应 const Value*
	auto *i32_ty = m_builder.getInt32Ty();// [修复]

	std::vector<llvm::Type *> param_types;
	param_types.push_back(obj_ptr_ty);// module
	param_types.push_back(obj_ptr_ty);// closure (占位)  // [修复]
	param_types.push_back(val_ptr_ty);// args_array (const Value*) // [修复]
	param_types.push_back(i32_ty);// argc // [修复]
	param_types.push_back(obj_ptr_ty);// kwargs

	auto *func_ty = llvm::FunctionType::get(obj_ptr_ty, param_types, false);
	auto *comp_func =
		llvm::Function::Create(func_ty, llvm::Function::InternalLinkage, mangled, m_module.get());

	ensure_personality(comp_func);

	// 设置参数名
	auto arg_it = comp_func->arg_begin();
	arg_it->setName("module");
	auto *module_arg = &*arg_it++;

	arg_it->setName("closure");// [修复]
	auto *closure_arg = &*arg_it++;

	arg_it->setName("args_array");// [修复]
	auto *args_array_arg = &*arg_it++;

	arg_it->setName("argc");// [修复]
	auto *argc_arg = &*arg_it++;

	arg_it->setName("kwargs");
	auto *kwargs_arg = &*arg_it++;
	(void)kwargs_arg;// 推导式暂不使用 kwargs

	// === Step 3: 进入函数体 ===
	auto *saved_bb = m_builder.GetInsertBlock();
	auto *saved_unwind = m_emitter.unwind_dest();

	auto *entry_bb = llvm::BasicBlock::Create(m_ctx, "entry", comp_func);
	m_builder.SetInsertPoint(entry_bb);
	m_emitter.set_unwind_dest(nullptr);// 重置 unwind 目标（新函数还没有 try 块）

	ScopeContext comp_scope{};
	comp_scope.type = ScopeContext::Type::FUNCTION;
	comp_scope.name = name;
	comp_scope.mangled_name = mangled;
	comp_scope.llvm_func = comp_func;
	comp_scope.module_obj = module_arg;// 使用当前函数的 module 参数
	comp_scope.var_scope = var_scope;
	m_codegen_ctx.push_scope(std::move(comp_scope));

	// === Step 4: 初始化闭包变量 ===

	// A. 为自己的 Cell 变量创建存储
	for (const auto &cv : cell_vars) {
		auto *cell = m_emitter.call_create_cell(nullptr);
		auto *alloca = m_codegen_ctx.create_local(cv + ".cell", obj_ptr_ty);
		m_builder.CreateStore(cell, alloca);
		m_codegen_ctx.register_cell(cv, alloca);
	}

	// B. 从 closure tuple 恢复 Free 变量
	for (size_t i = 0; i < free_vars.size(); ++i) {
		// closure[i]
		auto *cell =
			m_emitter.call_tuple_getitem(closure_arg, m_builder.getInt32(static_cast<int32_t>(i)));
		auto *alloca = m_codegen_ctx.create_local(free_vars[i] + ".free", obj_ptr_ty);
		m_builder.CreateStore(cell, alloca);
		m_codegen_ctx.register_free_var(free_vars[i], alloca);
	}

	// === Step 5: 生成推导式逻辑 ===

	// 从 args_array[0] 获取迭代器 (AOT raw ABI)
	// 之前: auto *iter_val = m_emitter.call_tuple_getitem(args_arg, m_builder.getInt32(0));
	// [修复]：改为从 Value 数组提取
	auto *iter_val = m_emitter.call_value_array_get(args_array_arg, m_builder.getInt32(0));// [修复]

	// 创建容器 (List/Set/Dict)
	auto *container = container_factory();

	// 生成多层循环结构
	const std::vector<std::shared_ptr<ast::Comprehension>> *generators = nullptr;
	if (auto *l = dynamic_cast<const ast::ListComp *>(node))
		generators = &l->generators();
	else if (auto *s = dynamic_cast<const ast::SetComp *>(node))
		generators = &s->generators();
	else if (auto *d = dynamic_cast<const ast::DictComp *>(node))
		generators = &d->generators();

	// 这里的闭包捕获了 container，供最内层循环使用
	emit_comprehension_loops(*generators, 0, iter_val, [&]() { loop_body_gen(container); });

	// 返回容器
	m_builder.CreateRet(container);

	// === Step 6: 恢复编译环境 ===
	m_codegen_ctx.pop_scope();
	m_builder.SetInsertPoint(saved_bb);
	m_emitter.set_unwind_dest(saved_unwind);

	// === Step 7: (在父作用域) 构建调用代码 ===

	// 构建 closure tuple (捕获父作用域的 cell)
	llvm::Value *closure_tuple = m_emitter.null_pyobject();
	if (has_closure) {
		std::vector<llvm::Value *> closure_elems;
		for (const auto &fv : free_vars) {
			llvm::Value *cell = nullptr;
			// 在父作用域查找对应的 cell/free
			if (auto *ca = m_codegen_ctx.find_cell(fv))
				cell = m_builder.CreateLoad(obj_ptr_ty, ca);
			else if (auto *fa = m_codegen_ctx.find_free_var(fv))
				cell = m_builder.CreateLoad(obj_ptr_ty, fa);
			else {
				// Bugfix: 编译器内部错误防御
				log::codegen()->error("Closure capture failed: '{}' not found in parent scope", fv);
				cell = m_emitter.get_none();
			}
			closure_elems.push_back(cell);
		}
		closure_tuple = m_emitter.create_tuple(closure_elems);
	}

	// rt_make_function
	auto *py_func = m_emitter.call_make_function(name,
		comp_func,
		m_codegen_ctx.module_object(),
		m_emitter.null_pyobject(),// defaults
		m_emitter.null_pyobject(),// kwdefaults
		closure_tuple// closure
	);

	// 调用推导式函数: func((iterator,))
	auto *call_args = m_emitter.create_tuple({ iterator });
	return m_emitter.call_function(py_func, call_args, nullptr);
}

// =============================================================================
// DictComp: {k: v for ...}
// =============================================================================
ast::Value *PylangCodegen::visit(const ast::DictComp *node)
{
	auto *iterable = generate(node->generators()[0]->iter().get());
	auto *iterator = m_emitter.call_get_iter(iterable);

	auto *res = compile_comprehension_impl(
		"<dictcomp>",
		node,
		iterator,
		[&]() { return m_emitter.create_dict({}, {}); },
		[&](llvm::Value *container) {
			auto *key = generate(node->key().get());
			auto *val = generate(node->value().get());
			if (key && val) m_emitter.call_setitem(container, key, val);
		});
	return make_value(res);
}

// =============================================================================
// SetComp: {x for ...}
// =============================================================================
ast::Value *PylangCodegen::visit(const ast::SetComp *node)
{
	auto *iterable = generate(node->generators()[0]->iter().get());
	auto *iterator = m_emitter.call_get_iter(iterable);

	auto *res = compile_comprehension_impl(
		"<setcomp>",
		node,
		iterator,
		[&]() { return m_emitter.create_set({}); },
		[&](llvm::Value *container) {
			auto *elt = generate(node->elt().get());
			if (elt) m_emitter.call_set_add(container, elt);
		});
	return make_value(res);
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
