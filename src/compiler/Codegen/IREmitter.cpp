#include "IREmitter.hpp"

#include "compiler/Support/Log.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>

#include <cstdint>
#include <string>

namespace pylang {

namespace {

	constexpr int64_t kTaggedIntMax = (1LL << 62) - 1;
	constexpr int64_t kTaggedIntMin = -(1LL << 62);

	bool fits_tagged_int(int64_t value) { return value >= kTaggedIntMin && value <= kTaggedIntMax; }

	uint64_t encode_tagged_int_bits(int64_t value)
	{
		return (static_cast<uint64_t>(value) << 1U) | 1U;
	}

	llvm::Value *emit_fits_tagged_int(llvm::IRBuilder<> &builder, llvm::Value *value)
	{
		auto *i64_ty = builder.getInt64Ty();
		auto *min_val = llvm::ConstantInt::get(i64_ty, kTaggedIntMin, true);
		auto *max_val = llvm::ConstantInt::get(i64_ty, kTaggedIntMax, true);
		auto *ge_min = builder.CreateICmpSGE(value, min_val, "tag.fit.min");
		auto *le_max = builder.CreateICmpSLE(value, max_val, "tag.fit.max");
		return builder.CreateAnd(ge_min, le_max, "tag.fit");
	}

	llvm::Value *emit_encode_tagged_int(llvm::IRBuilder<> &builder, llvm::Value *value)
	{
		auto *shifted =
			builder.CreateShl(value, llvm::ConstantInt::get(value->getType(), 1), "tag.shl");
		auto *bits =
			builder.CreateOr(shifted, llvm::ConstantInt::get(value->getType(), 1), "tag.bits");
		if (!bits->getType()->isIntegerTy(64)) {
			bits = builder.CreateTrunc(bits, builder.getInt64Ty(), "tag.bits.i64");
		}
		return builder.CreateIntToPtr(bits, builder.getPtrTy(), "tag.ptr");
	}

	llvm::Value *emit_signed_overflow_intrinsic(llvm::IRBuilder<> &builder,
		llvm::Module *module,
		llvm::Intrinsic::ID intrinsic,
		llvm::Value *lhs,
		llvm::Value *rhs,
		llvm::Value **overflow_out)
	{
		auto *fn = llvm::Intrinsic::getDeclaration(module, intrinsic, { builder.getInt64Ty() });
		auto *pair = builder.CreateCall(fn, { lhs, rhs }, "arith.ov");
		auto *result = builder.CreateExtractValue(pair, 0, "arith.result");
		*overflow_out = builder.CreateExtractValue(pair, 1, "arith.overflow");
		return result;
	}

}// namespace

// =============================================================================
// 核心：通用调用生成器
// =============================================================================
llvm::Value *IREmitter::emit_runtime_call(std::string_view name,
	llvm::ArrayRef<llvm::Value *> args,
	llvm::BasicBlock *unwind_dest)
{
	// 如果调用者没有显式传 unwind_dest，使用当前上下文的
	if (!unwind_dest) { unwind_dest = m_unwind_dest; }

	// Step 1: 从 RuntimeLinker 查找函数元数据
	auto result = m_linker.get_function(std::string(name));

	llvm::Function *func = nullptr;

	if (result.has_value()) {
		auto *rt_func = result.value()->llvm_func;
		auto mangled_name = rt_func->getName();
		func = m_module->getFunction(mangled_name);
		if (!func) {
			func = llvm::Function::Create(rt_func->getFunctionType(),
				llvm::Function::ExternalLinkage,
				mangled_name,
				m_module);
			func->setAttributes(rt_func->getAttributes());
		}
	} else {
		func = m_module->getFunction(std::string(name));
	}

	if (!func) {
		log::codegen()->error("Runtime function '{}' not found", name);
		return nullptr;
	}

	if (unwind_dest) {
		auto *current_func = m_builder.GetInsertBlock()->getParent();
		auto *normal_bb =
			llvm::BasicBlock::Create(m_builder.getContext(), "invoke.cont", current_func);
		auto *invoke_inst = m_builder.CreateInvoke(func, normal_bb, unwind_dest, args);
		m_builder.SetInsertPoint(normal_bb);
		return invoke_inst;
	} else {
		return m_builder.CreateCall(func, args);
	}
}

// void IREmitter::declare_eh_intrinsics()
// {
//     auto *i32_ty = m_builder.getInt32Ty();

//     if (!m_module->getFunction("__gxx_personality_v0")) {
//         auto *personality_ty = llvm::FunctionType::get(i32_ty, /*isVarArg=*/true);
//         llvm::Function::Create(
//             personality_ty, llvm::Function::ExternalLinkage, "__gxx_personality_v0", m_module);
//     }

//     // PylangException typeinfo — 精确捕获
//     // namespace py { struct PylangException; }
//     // Itanium mangling: _ZTIN2py16PylangExceptionE
//     if (!m_module->getGlobalVariable("_ZTIN2py16PylangExceptionE")) {
//         new llvm::GlobalVariable(*m_module,
//             m_builder.getPtrTy(),
//             /*isConstant=*/true,
//             llvm::GlobalValue::ExternalLinkage,
//             /*Initializer=*/nullptr,
//             "_ZTIN2py16PylangExceptionE");
//     }
// }

void IREmitter::declare_eh_intrinsics()
{
	auto *i32_ty = m_builder.getInt32Ty();

	if (!m_module->getFunction("__gxx_personality_v0")) {
		auto *personality_ty = llvm::FunctionType::get(i32_ty, /*isVarArg=*/true);
		llvm::Function::Create(
			personality_ty, llvm::Function::ExternalLinkage, "__gxx_personality_v0", m_module);
	}

	// 不再伪造定义 _ZTIN2py16PylangExceptionE
	// 真实 RTTI 会在 runtime.bc 中由编译器带入
}

// llvm::Constant *IREmitter::get_pylang_exception_typeinfo()
// {
//     declare_eh_intrinsics();
//     return m_module->getGlobalVariable("_ZTIN2py16PylangExceptionE");
// }

llvm::Constant *IREmitter::get_pylang_exception_typeinfo()
{
	declare_eh_intrinsics();
	// 返回 null 代表生成 `catch ptr null`（等同于 C++ 中 catch (...)）
	// 避免伪造 TypeInfo 导致 llvm-link 时被重命名造成 undefined symbol。
	// 在 exception 最终抛出和落地时，由 rt_catch_begin 负责具体的对象解包操作。
	return llvm::ConstantPointerNull::get(m_builder.getPtrTy());
}
// =============================================================================
// 辅助：全局字符串常量（带缓存）
// =============================================================================
llvm::Constant *IREmitter::create_global_string(std::string_view str)
{
	std::string key(str);
	auto it = m_string_cache.find(key);
	if (it != m_string_cache.end()) { return it->second; }


	auto *str_const = m_builder.CreateGlobalString(str, ".str");
	m_string_cache[key] = str_const;
	return str_const;
}

llvm::Constant *IREmitter::null_pyobject() const
{
	return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_builder.getContext()));
}

llvm::AllocaInst *IREmitter::create_entry_block_alloca(llvm::Type *type, const std::string &name)
{
	llvm::Function *func = m_builder.GetInsertBlock()->getParent();
	llvm::IRBuilder<> tmp_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
	return tmp_builder.CreateAlloca(type, nullptr, name);
}

// =============================================================================
// Tier 0: 单例
// =============================================================================
llvm::Value *IREmitter::get_none() { return emit_runtime_call("none", {}); }

llvm::Value *IREmitter::get_true() { return emit_runtime_call("true", {}); }

llvm::Value *IREmitter::get_false() { return emit_runtime_call("false", {}); }

llvm::Value *IREmitter::get_ellipsis() { return emit_runtime_call("ellipsis", {}); }

llvm::Value *IREmitter::get_not_implemented() { return emit_runtime_call("not_implemented", {}); }

// =============================================================================
// Tier 0: 对象创建
// =============================================================================
llvm::Value *IREmitter::create_string(std::string_view str)
{
	auto *str_ptr = create_global_string(str);
	auto *len = m_builder.getInt64(str.size());
	return emit_runtime_call("string_from_cstr", { str_ptr, len });
}

// llvm::Value *IREmitter::create_tuple(llvm::ArrayRef<llvm::Value *> elements)
// {
// 	if (elements.empty()) {
// 		// 传递 ptr 类型的 null（对应 PyObject**）
// 		auto *null_ptr =
// 			llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_builder.getContext()));
// 		return emit_runtime_call("build_tuple", { m_builder.getInt32(0), null_ptr });
// 	}

// 	// 创建临时数组存放元素指针
// 	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), elements.size());
// 	auto *arr = m_builder.CreateAlloca(arr_type, nullptr, "tuple_elems");

// 	for (size_t i = 0; i < elements.size(); ++i) {
// 		auto *gep = m_builder.CreateConstGEP2_32(arr_type, arr, 0, static_cast<unsigned>(i));
// 		m_builder.CreateStore(elements[i], gep);
// 	}

// 	auto *count = m_builder.getInt32(static_cast<uint32_t>(elements.size()));
// 	auto *arr_ptr = m_builder.CreateBitCast(arr, llvm::PointerType::getUnqual(pyobject_ptr_type()));

// 	return emit_runtime_call("build_tuple", { count, arr_ptr });
// }

llvm::Function *IREmitter::get_personality_function()
{
	declare_eh_intrinsics();
	return m_module->getFunction("__gxx_personality_v0");
}

llvm::Value *IREmitter::create_tuple(llvm::ArrayRef<llvm::Value *> elements)
{
	if (elements.empty()) {
		return emit_runtime_call("build_tuple",
			{ m_builder.getInt32(0), llvm::ConstantPointerNull::get(m_builder.getPtrTy()) });
	}

	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), elements.size());
	auto *arr = create_entry_block_alloca(arr_type, "tuple_elems");

	for (size_t i = 0; i < elements.size(); ++i) {
		auto *gep = m_builder.CreateConstGEP2_32(arr_type, arr, 0, static_cast<unsigned>(i));
		m_builder.CreateStore(elements[i], gep);
	}

	auto *count = m_builder.getInt32(static_cast<uint32_t>(elements.size()));
	// opaque pointer: arr 已经是 ptr，直接传递
	auto *arr_ptr = m_builder.CreateConstGEP2_32(arr_type, arr, 0, 0, "arr_ptr");

	return emit_runtime_call("build_tuple", { count, arr_ptr });
}

// =============================================================================
// Tier 1: 更多对象创建
// =============================================================================
llvm::Value *IREmitter::create_tagged_int_constant(int64_t value)
{
	auto *bits = llvm::ConstantInt::get(m_builder.getInt64Ty(), encode_tagged_int_bits(value));
	return llvm::ConstantExpr::getIntToPtr(bits, m_builder.getPtrTy());
}

llvm::Value *IREmitter::create_cached_big_int_literal(std::string_view decimal_str)
{
	std::string key(decimal_str);
	auto it = m_cached_big_int_objects.find(key);
	if (it == m_cached_big_int_objects.end()) {
		auto *gvar = new llvm::GlobalVariable(*m_module,
			m_builder.getPtrTy(),
			false,
			llvm::GlobalValue::InternalLinkage,
			llvm::ConstantPointerNull::get(m_builder.getPtrTy()),
			".pyint_obj." + std::to_string(m_cached_big_int_objects.size()));
		it = m_cached_big_int_objects.emplace(std::move(key), gvar).first;
	}
	return m_builder.CreateLoad(m_builder.getPtrTy(), it->second, "pyint.literal");
}

llvm::Value *IREmitter::create_integer_from_i64_value(llvm::Value *value)
{
	return emit_runtime_call("integer_from_i64", { value });
}

llvm::Value *IREmitter::create_integer(int64_t value)
{
	if (fits_tagged_int(value)) { return create_tagged_int_constant(value); }
	return create_cached_big_int_literal(std::to_string(value));
}

llvm::Value *IREmitter::create_integer_big(std::string_view decimal_str)
{
	return create_cached_big_int_literal(decimal_str);
}

llvm::Value *IREmitter::create_float(double value)
{
	auto *val = llvm::ConstantFP::get(m_builder.getDoubleTy(), value);
	return emit_runtime_call("float_from_f64", { val });
}

llvm::Value *IREmitter::create_list(llvm::ArrayRef<llvm::Value *> elements)
{
	if (elements.empty()) {
		auto *null_ptr =
			llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_builder.getContext()));
		return emit_runtime_call("build_list", { m_builder.getInt32(0), null_ptr });
	}

	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), elements.size());
	auto *arr = create_entry_block_alloca(arr_type, "list_elems");

	for (size_t i = 0; i < elements.size(); ++i) {
		auto *gep = m_builder.CreateConstGEP2_32(arr_type, arr, 0, static_cast<unsigned>(i));
		m_builder.CreateStore(elements[i], gep);
	}

	auto *count = m_builder.getInt32(static_cast<uint32_t>(elements.size()));
	auto *arr_ptr = m_builder.CreateBitCast(arr, llvm::PointerType::getUnqual(pyobject_ptr_type()));

	return emit_runtime_call("build_list", { count, arr_ptr });
}

// =============================================================================
// Tier 1: 二元运算（使用宏减少重复）
// =============================================================================
#define DEFINE_BINARY_OP(name, func_name)                                          \
	llvm::Value *IREmitter::call_binary_##name(llvm::Value *lhs, llvm::Value *rhs) \
	{                                                                              \
		return emit_runtime_call(#func_name, { lhs, rhs });                        \
	}

DEFINE_BINARY_OP(add, binary_add)
DEFINE_BINARY_OP(sub, binary_sub)
DEFINE_BINARY_OP(mul, binary_mul)
DEFINE_BINARY_OP(truediv, binary_truediv)
DEFINE_BINARY_OP(floordiv, binary_floordiv)
DEFINE_BINARY_OP(mod, binary_mod)
DEFINE_BINARY_OP(pow, binary_pow)
DEFINE_BINARY_OP(lshift, binary_lshift)
DEFINE_BINARY_OP(rshift, binary_rshift)
DEFINE_BINARY_OP(and, binary_and)
DEFINE_BINARY_OP(or, binary_or)
DEFINE_BINARY_OP(xor, binary_xor)

#undef DEFINE_BINARY_OP

llvm::Value *IREmitter::emit_tagged_binary_op(TaggedBinaryOp op,
	llvm::Value *lhs,
	llvm::Value *rhs,
	std::string_view slow_symbol)
{
	auto *func = m_builder.GetInsertBlock()->getParent();
	auto *i64_ty = m_builder.getInt64Ty();
	auto *i128_ty = llvm::IntegerType::get(m_builder.getContext(), 128);
	auto *ptr_ty = m_builder.getPtrTy();

	auto *lhs_bits = m_builder.CreatePtrToInt(lhs, i64_ty, "lhs.bits");
	auto *rhs_bits = m_builder.CreatePtrToInt(rhs, i64_ty, "rhs.bits");
	auto *both_tagged_bits =
		m_builder.CreateAnd(m_builder.CreateAnd(lhs_bits, rhs_bits), m_builder.getInt64(1));
	auto *both_tagged =
		m_builder.CreateICmpEQ(both_tagged_bits, m_builder.getInt64(1), "both.tagged");

	auto *fast_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.fast", func);
	auto *slow_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.slow", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.merge", func);

	m_builder.CreateCondBr(both_tagged, fast_bb, slow_bb);

	std::vector<std::pair<llvm::Value *, llvm::BasicBlock *>> fast_incoming;
	auto emit_fast_result = [&](llvm::Value *result) {
		auto *encoded = emit_encode_tagged_int(m_builder, result);
		auto *from_bb = m_builder.GetInsertBlock();
		m_builder.CreateBr(merge_bb);
		fast_incoming.emplace_back(encoded, from_bb);
	};

	auto branch_if_valid_or_slow = [&](llvm::Value *valid, const llvm::Twine &name) {
		auto *body_bb = llvm::BasicBlock::Create(m_builder.getContext(), name, func);
		m_builder.CreateCondBr(valid, body_bb, slow_bb);
		m_builder.SetInsertPoint(body_bb);
	};

	m_builder.SetInsertPoint(fast_bb);
	auto *l = m_builder.CreateAShr(lhs_bits, m_builder.getInt64(1), "lhs.i64");
	auto *r = m_builder.CreateAShr(rhs_bits, m_builder.getInt64(1), "rhs.i64");

	switch (op) {
	case TaggedBinaryOp::Add: {
		llvm::Value *overflow = nullptr;
		auto *result = emit_signed_overflow_intrinsic(
			m_builder, m_module, llvm::Intrinsic::sadd_with_overflow, l, r, &overflow);
		auto *valid = m_builder.CreateAnd(
			m_builder.CreateNot(overflow), emit_fits_tagged_int(m_builder, result));
		branch_if_valid_or_slow(valid, "tag.add.ok");
		emit_fast_result(result);
		break;
	}
	case TaggedBinaryOp::Sub: {
		llvm::Value *overflow = nullptr;
		auto *result = emit_signed_overflow_intrinsic(
			m_builder, m_module, llvm::Intrinsic::ssub_with_overflow, l, r, &overflow);
		auto *valid = m_builder.CreateAnd(
			m_builder.CreateNot(overflow), emit_fits_tagged_int(m_builder, result));
		branch_if_valid_or_slow(valid, "tag.sub.ok");
		emit_fast_result(result);
		break;
	}
	case TaggedBinaryOp::Mul: {
		llvm::Value *overflow = nullptr;
		auto *result = emit_signed_overflow_intrinsic(
			m_builder, m_module, llvm::Intrinsic::smul_with_overflow, l, r, &overflow);
		auto *valid = m_builder.CreateAnd(
			m_builder.CreateNot(overflow), emit_fits_tagged_int(m_builder, result));
		branch_if_valid_or_slow(valid, "tag.mul.ok");
		emit_fast_result(result);
		break;
	}
	case TaggedBinaryOp::FloorDiv: {
		auto *nonzero = m_builder.CreateICmpNE(r, m_builder.getInt64(0), "tag.div.nonzero");
		branch_if_valid_or_slow(nonzero, "tag.floordiv.ok");
		auto *q = m_builder.CreateSDiv(l, r, "tag.div.q");
		auto *rem = m_builder.CreateSRem(l, r, "tag.div.rem");
		auto *rem_nonzero = m_builder.CreateICmpNE(rem, m_builder.getInt64(0));
		auto *sign_diff =
			m_builder.CreateICmpSLT(m_builder.CreateXor(rem, r), m_builder.getInt64(0));
		auto *adjust = m_builder.CreateAnd(rem_nonzero, sign_diff);
		auto *adjusted =
			m_builder.CreateSub(q, m_builder.CreateZExt(adjust, i64_ty), "tag.div.floor");
		auto *fits = emit_fits_tagged_int(m_builder, adjusted);
		branch_if_valid_or_slow(fits, "tag.floordiv.fit");
		emit_fast_result(adjusted);
		break;
	}
	case TaggedBinaryOp::Mod: {
		auto *nonzero = m_builder.CreateICmpNE(r, m_builder.getInt64(0), "tag.mod.nonzero");
		branch_if_valid_or_slow(nonzero, "tag.mod.ok");
		auto *rem = m_builder.CreateSRem(l, r, "tag.mod.rem");
		auto *rem_nonzero = m_builder.CreateICmpNE(rem, m_builder.getInt64(0));
		auto *sign_diff =
			m_builder.CreateICmpSLT(m_builder.CreateXor(rem, r), m_builder.getInt64(0));
		auto *adjust = m_builder.CreateAnd(rem_nonzero, sign_diff);
		auto *adjusted =
			m_builder.CreateSelect(adjust, m_builder.CreateAdd(rem, r), rem, "tag.mod.py");
		emit_fast_result(adjusted);
		break;
	}
	case TaggedBinaryOp::LShift: {
		auto *nonnegative = m_builder.CreateICmpSGE(r, m_builder.getInt64(0), "tag.shl.nonneg");
		auto *small = m_builder.CreateICmpSLT(r, m_builder.getInt64(63), "tag.shl.small");
		auto *valid_shift = m_builder.CreateAnd(nonnegative, small, "tag.shl.valid");
		branch_if_valid_or_slow(valid_shift, "tag.shl.ok");
		auto *l128 = m_builder.CreateSExt(l, i128_ty);
		auto *r128 = m_builder.CreateZExt(r, i128_ty);
		auto *result128 = m_builder.CreateShl(l128, r128, "tag.shl.i128");
		auto *min128 = llvm::ConstantInt::get(i128_ty, kTaggedIntMin, true);
		auto *max128 = llvm::ConstantInt::get(i128_ty, kTaggedIntMax, true);
		auto *fits = m_builder.CreateAnd(m_builder.CreateICmpSGE(result128, min128),
			m_builder.CreateICmpSLE(result128, max128),
			"tag.shl.fit");
		branch_if_valid_or_slow(fits, "tag.shl.fit.ok");
		emit_fast_result(m_builder.CreateTrunc(result128, i64_ty, "tag.shl.result"));
		break;
	}
	case TaggedBinaryOp::RShift: {
		auto *nonnegative = m_builder.CreateICmpSGE(r, m_builder.getInt64(0), "tag.shr.nonneg");
		branch_if_valid_or_slow(nonnegative, "tag.shr.ok");
		auto *large = m_builder.CreateICmpSGE(r, m_builder.getInt64(64), "tag.shr.large");
		auto *large_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.shr.large.ok", func);
		auto *small_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.shr.small.ok", func);
		m_builder.CreateCondBr(large, large_bb, small_bb);

		m_builder.SetInsertPoint(large_bb);
		auto *negative_l = m_builder.CreateICmpSLT(l, m_builder.getInt64(0), "tag.shr.l.neg");
		auto *large_result =
			m_builder.CreateSelect(negative_l, m_builder.getInt64(-1), m_builder.getInt64(0));
		emit_fast_result(large_result);

		m_builder.SetInsertPoint(small_bb);
		auto *shifted = m_builder.CreateAShr(l, r, "tag.shr.result");
		emit_fast_result(shifted);
		break;
	}
	case TaggedBinaryOp::BitAnd:
		emit_fast_result(m_builder.CreateAnd(l, r, "tag.and"));
		break;
	case TaggedBinaryOp::BitOr:
		emit_fast_result(m_builder.CreateOr(l, r, "tag.or"));
		break;
	case TaggedBinaryOp::BitXor:
		emit_fast_result(m_builder.CreateXor(l, r, "tag.xor"));
		break;
	}

	m_builder.SetInsertPoint(slow_bb);
	auto *slow_result = emit_runtime_call(slow_symbol, { lhs, rhs });
	auto *slow_end = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(merge_bb);
	auto *phi =
		m_builder.CreatePHI(ptr_ty, static_cast<unsigned>(fast_incoming.size() + 1), "tag.result");
	for (auto &[value, block] : fast_incoming) { phi->addIncoming(value, block); }
	phi->addIncoming(slow_result, slow_end);
	return phi;
}

// =============================================================================
// Tier 1: 增量赋值（inplace）运算
// =============================================================================
#define DEFINE_INPLACE_OP(name, func_name)                                          \
	llvm::Value *IREmitter::call_inplace_##name(llvm::Value *lhs, llvm::Value *rhs) \
	{                                                                               \
		return emit_runtime_call(#func_name, { lhs, rhs });                         \
	}

DEFINE_INPLACE_OP(add, inplace_add)
DEFINE_INPLACE_OP(sub, inplace_sub)
DEFINE_INPLACE_OP(mul, inplace_mul)
DEFINE_INPLACE_OP(truediv, inplace_truediv)
DEFINE_INPLACE_OP(floordiv, inplace_floordiv)
DEFINE_INPLACE_OP(mod, inplace_mod)
DEFINE_INPLACE_OP(pow, inplace_pow)
DEFINE_INPLACE_OP(lshift, inplace_lshift)
DEFINE_INPLACE_OP(rshift, inplace_rshift)
DEFINE_INPLACE_OP(and, inplace_and)
DEFINE_INPLACE_OP(or, inplace_or)
DEFINE_INPLACE_OP(xor, inplace_xor)

#undef DEFINE_INPLACE_OP

// =============================================================================
// Tier 1: 一元运算
// =============================================================================
#define DEFINE_UNARY_OP(name, func_name)                        \
	llvm::Value *IREmitter::call_unary_##name(llvm::Value *obj) \
	{                                                           \
		return emit_runtime_call(#func_name, { obj });          \
	}

DEFINE_UNARY_OP(neg, unary_neg)
DEFINE_UNARY_OP(pos, unary_pos)
DEFINE_UNARY_OP(invert, unary_invert)
DEFINE_UNARY_OP(not, unary_not)

#undef DEFINE_UNARY_OP

// =============================================================================
// Tier 1: 类型转换
// =============================================================================
llvm::Value *IREmitter::call_is_true(llvm::Value *obj)
{
	return emit_runtime_call("is_true", { obj });
}

// =============================================================================
// Tier 2: 比较操作
// =============================================================================
#define DEFINE_COMPARE_OP(name, func_name)                                          \
	llvm::Value *IREmitter::call_compare_##name(llvm::Value *lhs, llvm::Value *rhs) \
	{                                                                               \
		return emit_runtime_call(#func_name, { lhs, rhs });                         \
	}

DEFINE_COMPARE_OP(eq, compare_eq)
DEFINE_COMPARE_OP(ne, compare_ne)
DEFINE_COMPARE_OP(lt, compare_lt)
DEFINE_COMPARE_OP(le, compare_le)
DEFINE_COMPARE_OP(gt, compare_gt)
DEFINE_COMPARE_OP(ge, compare_ge)
DEFINE_COMPARE_OP(is, compare_is)
DEFINE_COMPARE_OP(is_not, compare_is_not)

#undef DEFINE_COMPARE_OP

llvm::Value *IREmitter::call_compare_in(llvm::Value *value, llvm::Value *container)
{
	return emit_runtime_call("compare_in", { value, container });
}

llvm::Value *IREmitter::call_compare_not_in(llvm::Value *value, llvm::Value *container)
{
	return emit_runtime_call("compare_not_in", { value, container });
}

llvm::Value *IREmitter::emit_tagged_compare_bool(TaggedCompareOp op,
	llvm::Value *lhs,
	llvm::Value *rhs,
	std::string_view slow_symbol)
{
	auto *func = m_builder.GetInsertBlock()->getParent();
	auto *i64_ty = m_builder.getInt64Ty();

	auto *lhs_bits = m_builder.CreatePtrToInt(lhs, i64_ty, "lhs.bits");
	auto *rhs_bits = m_builder.CreatePtrToInt(rhs, i64_ty, "rhs.bits");
	auto *both_tagged_bits =
		m_builder.CreateAnd(m_builder.CreateAnd(lhs_bits, rhs_bits), m_builder.getInt64(1));
	auto *both_tagged =
		m_builder.CreateICmpEQ(both_tagged_bits, m_builder.getInt64(1), "both.tagged");

	auto *fast_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.cmp.fast", func);
	auto *slow_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.cmp.slow", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.cmp.merge", func);
	m_builder.CreateCondBr(both_tagged, fast_bb, slow_bb);

	m_builder.SetInsertPoint(fast_bb);
	auto *l = m_builder.CreateAShr(lhs_bits, m_builder.getInt64(1), "lhs.i64");
	auto *r = m_builder.CreateAShr(rhs_bits, m_builder.getInt64(1), "rhs.i64");
	llvm::Value *fast_result = nullptr;
	switch (op) {
	case TaggedCompareOp::Eq:
		fast_result = m_builder.CreateICmpEQ(l, r, "tag.eq");
		break;
	case TaggedCompareOp::Ne:
		fast_result = m_builder.CreateICmpNE(l, r, "tag.ne");
		break;
	case TaggedCompareOp::Lt:
		fast_result = m_builder.CreateICmpSLT(l, r, "tag.lt");
		break;
	case TaggedCompareOp::Le:
		fast_result = m_builder.CreateICmpSLE(l, r, "tag.le");
		break;
	case TaggedCompareOp::Gt:
		fast_result = m_builder.CreateICmpSGT(l, r, "tag.gt");
		break;
	case TaggedCompareOp::Ge:
		fast_result = m_builder.CreateICmpSGE(l, r, "tag.ge");
		break;
	}
	auto *fast_end = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(slow_bb);
	auto *slow_result = emit_runtime_call(slow_symbol, { lhs, rhs });
	auto *slow_end = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(merge_bb);
	auto *phi = m_builder.CreatePHI(m_builder.getInt1Ty(), 2, "tag.cmp.result");
	phi->addIncoming(fast_result, fast_end);
	phi->addIncoming(slow_result, slow_end);
	return phi;
}

llvm::Value *IREmitter::emit_tagged_compare_object(TaggedCompareOp op,
	llvm::Value *lhs,
	llvm::Value *rhs,
	std::string_view slow_symbol)
{
	auto *func = m_builder.GetInsertBlock()->getParent();
	auto *i64_ty = m_builder.getInt64Ty();
	auto *ptr_ty = m_builder.getPtrTy();

	auto *lhs_bits = m_builder.CreatePtrToInt(lhs, i64_ty, "lhs.bits");
	auto *rhs_bits = m_builder.CreatePtrToInt(rhs, i64_ty, "rhs.bits");
	auto *both_tagged_bits =
		m_builder.CreateAnd(m_builder.CreateAnd(lhs_bits, rhs_bits), m_builder.getInt64(1));
	auto *both_tagged =
		m_builder.CreateICmpEQ(both_tagged_bits, m_builder.getInt64(1), "both.tagged");

	auto *fast_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.cmp.obj.fast", func);
	auto *slow_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.cmp.obj.slow", func);
	auto *merge_bb = llvm::BasicBlock::Create(m_builder.getContext(), "tag.cmp.obj.merge", func);
	m_builder.CreateCondBr(both_tagged, fast_bb, slow_bb);

	m_builder.SetInsertPoint(fast_bb);
	auto *l = m_builder.CreateAShr(lhs_bits, m_builder.getInt64(1), "lhs.i64");
	auto *r = m_builder.CreateAShr(rhs_bits, m_builder.getInt64(1), "rhs.i64");
	llvm::Value *fast_bool = nullptr;
	switch (op) {
	case TaggedCompareOp::Eq:
		fast_bool = m_builder.CreateICmpEQ(l, r, "tag.eq");
		break;
	case TaggedCompareOp::Ne:
		fast_bool = m_builder.CreateICmpNE(l, r, "tag.ne");
		break;
	case TaggedCompareOp::Lt:
		fast_bool = m_builder.CreateICmpSLT(l, r, "tag.lt");
		break;
	case TaggedCompareOp::Le:
		fast_bool = m_builder.CreateICmpSLE(l, r, "tag.le");
		break;
	case TaggedCompareOp::Gt:
		fast_bool = m_builder.CreateICmpSGT(l, r, "tag.gt");
		break;
	case TaggedCompareOp::Ge:
		fast_bool = m_builder.CreateICmpSGE(l, r, "tag.ge");
		break;
	}
	auto *fast_result = bool_to_object(fast_bool);
	auto *fast_end = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(slow_bb);
	auto *slow_result = emit_runtime_call(slow_symbol, { lhs, rhs });
	auto *slow_end = m_builder.GetInsertBlock();
	m_builder.CreateBr(merge_bb);

	m_builder.SetInsertPoint(merge_bb);
	auto *phi = m_builder.CreatePHI(ptr_ty, 2, "tag.cmp.obj.result");
	phi->addIncoming(fast_result, fast_end);
	phi->addIncoming(slow_result, slow_end);
	return phi;
}

llvm::Value *IREmitter::bool_to_object(llvm::Value *value)
{
	auto *true_obj = get_true();
	auto *false_obj = get_false();
	return m_builder.CreateSelect(value, true_obj, false_obj, "bool.obj");
}

// =============================================================================
// Tier 2: 迭代器
// =============================================================================
llvm::Value *IREmitter::call_get_iter(llvm::Value *obj)
{
	return emit_runtime_call("get_iter", { obj });
}

llvm::Value *IREmitter::call_iter_next(llvm::Value *iter, llvm::Value *has_value_out)
{
	return emit_runtime_call("iter_next", { iter, has_value_out });
}

llvm::Value *
	IREmitter::call_iter_next_unpack2(llvm::Value *iter, llvm::Value *out_a, llvm::Value *out_b)
{
	return emit_runtime_call("iter_next_unpack2", { iter, out_a, out_b });
}

// =============================================================================
// 融合运算 (Fused Operations)
// =============================================================================

llvm::Value *IREmitter::call_compare_lt_bool(llvm::Value *lhs, llvm::Value *rhs)
{
	return emit_runtime_call("compare_lt_bool", { lhs, rhs });
}

llvm::Value *IREmitter::call_compare_le_bool(llvm::Value *lhs, llvm::Value *rhs)
{
	return emit_runtime_call("compare_le_bool", { lhs, rhs });
}

llvm::Value *IREmitter::call_compare_gt_bool(llvm::Value *lhs, llvm::Value *rhs)
{
	return emit_runtime_call("compare_gt_bool", { lhs, rhs });
}

llvm::Value *IREmitter::call_compare_eq_bool(llvm::Value *lhs, llvm::Value *rhs)
{
	return emit_runtime_call("compare_eq_bool", { lhs, rhs });
}

llvm::Value *IREmitter::call_compare_ne_bool(llvm::Value *lhs, llvm::Value *rhs)
{
	return emit_runtime_call("compare_ne_bool", { lhs, rhs });
}

llvm::Value *IREmitter::call_compare_ge_bool(llvm::Value *lhs, llvm::Value *rhs)
{
	return emit_runtime_call("compare_ge_bool", { lhs, rhs });
}

llvm::Value *IREmitter::call_compare_not_in_bool(llvm::Value *key, llvm::Value *container)
{
	return emit_runtime_call("compare_not_in_bool", { key, container });
}

llvm::Value *IREmitter::call_compare_in_bool(llvm::Value *key, llvm::Value *container)
{
	return emit_runtime_call("compare_in_bool", { key, container });
}

llvm::Value *IREmitter::call_is_true_fast(llvm::Value *obj)
{
	return emit_runtime_call("is_true_fast", { obj });
}

llvm::Value *IREmitter::call_list_getitem_i64(llvm::Value *list, llvm::Value *index)
{
	return emit_runtime_call("list_getitem_i64", { list, index });
}

llvm::Value *IREmitter::call_list_getitem_i64_truthy(llvm::Value *list, llvm::Value *index)
{
	return emit_runtime_call("list_getitem_i64_truthy", { list, index });
}

llvm::Value *IREmitter::call_list_getitem_i64_not(llvm::Value *list, llvm::Value *index)
{
	return emit_runtime_call("list_getitem_i64_not", { list, index });
}

void IREmitter::call_list_setitem_i64(llvm::Value *list, llvm::Value *index, llvm::Value *value)
{
	emit_runtime_call("list_setitem_i64", { list, index, value });
}

llvm::Value *IREmitter::call_dict_getitem(llvm::Value *dict, llvm::Value *key)
{
	return emit_runtime_call("dict_getitem", { dict, key });
}

void IREmitter::call_dict_setitem(llvm::Value *dict, llvm::Value *key, llvm::Value *value)
{
	emit_runtime_call("dict_setitem", { dict, key, value });
}

llvm::Value *IREmitter::call_dict_contains_bool(llvm::Value *key, llvm::Value *container)
{
	return emit_runtime_call("dict_contains_bool", { key, container });
}

void IREmitter::call_list_insert_0_tuple2(llvm::Value *list, llvm::Value *a, llvm::Value *b)
{
	emit_runtime_call("list_insert_0_tuple2", { list, a, b });
}

llvm::Value *IREmitter::call_dict_get_or_null(llvm::Value *dict, llvm::Value *key)
{
	return emit_runtime_call("dict_get_or_null", { dict, key });
}

llvm::Value *
	IREmitter::call_list_pop_unpack2(llvm::Value *list, llvm::Value *out_a, llvm::Value *out_b)
{
	return emit_runtime_call("list_pop_unpack2", { list, out_a, out_b });
}

llvm::Value *IREmitter::call_dict_items_iter_for_loop(llvm::Value *owner)
{
	return emit_runtime_call("dict_items_iter_for_loop", { owner });
}

void IREmitter::call_setitem_fast(llvm::Value *obj, llvm::Value *key, llvm::Value *value)
{
	emit_runtime_call("setitem_fast", { obj, key, value });
}

// =============================================================================
// Tier 3: 下标访问
// =============================================================================
llvm::Value *IREmitter::call_getitem(llvm::Value *obj, llvm::Value *key)
{
	return emit_runtime_call("getitem", { obj, key });
}

void IREmitter::call_setitem(llvm::Value *obj, llvm::Value *key, llvm::Value *value)
{
	emit_runtime_call("setitem", { obj, key, value });
}

void IREmitter::call_delitem(llvm::Value *obj, llvm::Value *key)
{
	emit_runtime_call("delitem", { obj, key });
}

// =============================================================================
// Tier 3: 容器方法
// =============================================================================
void IREmitter::call_list_append(llvm::Value *list, llvm::Value *value)
{
	emit_runtime_call("list_append", { list, value });
}

void IREmitter::call_set_add(llvm::Value *set, llvm::Value *value)
{
	emit_runtime_call("set_add", { set, value });
}

// =============================================================================
// Tier 0: 属性访问
// =============================================================================

llvm::Value *IREmitter::get_interned_string_obj(std::string_view name)
{
	std::string key(name);
	if (m_interned_string_objects.count(key)) return m_interned_string_objects[key];

	// 创建一个全局 ptr 变量，初始值为 null
	auto *gvar = new llvm::GlobalVariable(*m_module,
		m_builder.getPtrTy(),
		false,
		llvm::GlobalValue::InternalLinkage,
		llvm::ConstantPointerNull::get(m_builder.getPtrTy()),
		".pystr_obj." + key);

	m_interned_string_objects[key] = gvar;
	return gvar;
}

void IREmitter::emit_interned_strings_initialization()
{
	// 遍历当前模块记录的所有属性名常量
	for (auto &[name, gvar] : m_interned_string_objects) {
		auto *str_ptr = create_global_string(name);
		auto *len = m_builder.getInt64(name.size());

		// 调用 rt_string_from_cstr。内部 PyString::create 会调用 intern，
		// 确保跨模块的同名字符串指向同一个真正的 PyString 实例。
		auto *py_str = emit_runtime_call("string_from_cstr", { str_ptr, len });

		// 将获取到的 PyObject* 存入该模块私有的全局变量槽位
		m_builder.CreateStore(py_str, gvar);
	}
}

void IREmitter::emit_cached_literals_initialization()
{
	for (auto &[decimal, gvar] : m_cached_big_int_objects) {
		auto *str_ptr = create_global_string(decimal);
		auto *py_int = emit_runtime_call("integer_from_string", { str_ptr });
		m_builder.CreateStore(py_int, gvar);
	}
}

llvm::Value *IREmitter::call_getattr(llvm::Value *obj, std::string_view name)
{
	// 从缓存加载 PyObject* (PyString)
	auto *gvar = get_interned_string_obj(name);
	auto *name_obj = m_builder.CreateLoad(m_builder.getPtrTy(), gvar);

	llvm::Type *cache_struct_ty = nullptr;
	if (auto *cache_template = m_module->getNamedGlobal("pylang_attr_cache_template")) {
		cache_struct_ty = cache_template->getValueType();
	} else {
		cache_struct_ty = llvm::ArrayType::get(m_builder.getInt64Ty(), 8);
	}

	auto *cache_gvar = new llvm::GlobalVariable(*m_module,
		cache_struct_ty,
		false,
		llvm::GlobalValue::InternalLinkage,
		llvm::Constant::getNullValue(cache_struct_ty),
		".attr_cache");

	return emit_runtime_call("getattr_ic", { cache_gvar, obj, name_obj });
}

llvm::Value *IREmitter::call_load_global(llvm::Value *module, std::string_view name)
{
	auto *name_str = create_global_string(name);
	return emit_runtime_call("load_global", { module, name_str });
}

void IREmitter::call_store_global(llvm::Value *module, std::string_view name, llvm::Value *value)
{
	auto *name_str = create_global_string(name);
	emit_runtime_call("store_global", { module, name_str, value });
}

// void IREmitter::call_setattr(llvm::Value *obj, std::string_view name, llvm::Value *value)
// {
// 	auto *name_str = create_global_string(name);
// 	emit_runtime_call("setattr", { obj, name_str, value });
// }

void IREmitter::call_setattr(llvm::Value *obj, std::string_view name, llvm::Value *value)
{
	// 同理，setattr 也应使用 fast 版本以避免每一步都 intern
	auto *gvar = get_interned_string_obj(name);
	auto *name_obj = m_builder.CreateLoad(m_builder.getPtrTy(), gvar);

	llvm::Type *cache_struct_ty = nullptr;
	if (auto *cache_template = m_module->getNamedGlobal("pylang_attr_cache_template")) {
		cache_struct_ty = cache_template->getValueType();
	} else {
		cache_struct_ty = llvm::ArrayType::get(m_builder.getInt64Ty(), 8);
	}

	auto *cache_gvar = new llvm::GlobalVariable(*m_module,
		cache_struct_ty,
		false,
		llvm::GlobalValue::InternalLinkage,
		llvm::Constant::getNullValue(cache_struct_ty),
		".attr_store_cache");

	emit_runtime_call("setattr_ic", { cache_gvar, obj, name_obj, value });
}

void IREmitter::call_delattr(llvm::Value *obj, std::string_view name)
{
	auto *name_str = create_global_string(name);
	emit_runtime_call("delattr", { obj, name_str });
}

// =============================================================================
// Tier 0: 函数调用
// =============================================================================
llvm::Value *IREmitter::call_function(llvm::Value *callable, llvm::Value *args, llvm::Value *kwargs)
{
	if (!kwargs) { kwargs = null_pyobject(); }
	return emit_runtime_call("call", { callable, args, kwargs });
}

llvm::Value *IREmitter::call_function_raw_ptrs(llvm::Value *callable,
	llvm::Value *args_ptr,
	llvm::Value *argc,
	llvm::Value *kwargs)
{
	return emit_runtime_call("call_raw_ptrs", { callable, args_ptr, argc, kwargs });
}

llvm::Value *IREmitter::call_method_ic_ptrs(llvm::Value *owner,
	llvm::Value *method_name_cstr,
	llvm::Value *args_ptr,
	llvm::Value *argc,
	llvm::Value *kwargs)
{
	// 动态申请全局 CallSite Cache (PIC) 变量，传入 runtime
	// 我们从 runtime.bc 里面读取 `pylang_method_cache_template` 获取其实际编译出的结构体类型
	// 从而彻底避免硬编码诸如 32 个 i64 等容易随系统/宏不同而崩溃的幻数。
	llvm::Type *cache_struct_ty = nullptr;
	if (auto *gv = m_module->getNamedGlobal("pylang_method_cache_template")) {
		cache_struct_ty = gv->getValueType();
	} else {
		// 理论上不可能走入这里，因为已经跑了 RuntimeLinker，如果万一未找到就 Fallback
		cache_struct_ty = llvm::ArrayType::get(m_builder.getInt64Ty(), 32);
	}

	auto *cache_gvar = new llvm::GlobalVariable(*m_module,
		cache_struct_ty,
		false,
		llvm::GlobalValue::InternalLinkage,
		llvm::Constant::getNullValue(cache_struct_ty),
		".method_cache");

	return emit_runtime_call(
		"call_method_ic_ptrs", { cache_gvar, owner, method_name_cstr, args_ptr, argc, kwargs });
}

llvm::Value *IREmitter::call_function_fast(llvm::Value *callable,
	llvm::ArrayRef<llvm::Value *> args)
{
	if (args.empty()) {
		auto *null_ptr =
			llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_builder.getContext()));
		return emit_runtime_call("call_fast", { callable, m_builder.getInt32(0), null_ptr });
	}

	// 栈上分配参数数组（不需要堆分配 PyTuple）
	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), args.size());
	auto *arr = create_entry_block_alloca(arr_type, "call_args");

	for (size_t i = 0; i < args.size(); ++i) {
		auto *gep = m_builder.CreateConstGEP2_32(arr_type, arr, 0, static_cast<unsigned>(i));
		m_builder.CreateStore(args[i], gep);
	}

	auto *argc = m_builder.getInt32(static_cast<uint32_t>(args.size()));
	auto *arr_ptr = m_builder.CreateBitCast(arr, llvm::PointerType::getUnqual(pyobject_ptr_type()));

	return emit_runtime_call("call_fast", { callable, argc, arr_ptr });
}

// llvm::Value *IREmitter::call_method_fast(llvm::Value *obj,
// 	std::string_view name,
// 	llvm::ArrayRef<llvm::Value *> args)
// {
// 	// 1. 在当前函数的 EntryBlock 分配内存（避免在循环中爆栈）
// 	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), args.size());
// 	auto *arr = create_entry_block_alloca(arr_type, "method_args");

// 	// 2. 填充参数
// 	for (size_t i = 0; i < args.size(); ++i) {
// 		auto *gep = m_builder.CreateConstGEP2_32(arr_type, arr, 0, i);
// 		m_builder.SetInsertPoint(m_builder.GetInsertBlock());// 确保插入点正确
// 		m_builder.CreateStore(args[i], gep);
// 	}

// 	// 3. 发射对 rt_call_method_raw 的调用
// 	auto *name_ptr = create_global_string(name);
// 	auto *argc = m_builder.getInt32(args.size());
// 	auto *argv = m_builder.CreateConstGEP2_32(arr_type, arr, 0, 0);

// 	return emit_runtime_call("rt_call_method_raw", { obj, name_ptr, argc, argv });
// }

// =============================================================================
// Tier 4: 方法调用
// =============================================================================
llvm::Value *IREmitter::call_load_method(llvm::Value *obj, std::string_view method_name)
{
	auto *name_str = create_global_string(method_name);
	return emit_runtime_call("load_method", { obj, name_str });
}

// =============================================================================
// Tier 0: 模块导入
// =============================================================================
// llvm::Value *IREmitter::call_import(std::string_view module_name)
// {
//     auto *name_str = create_global_string(module_name);
//     auto *empty_str = create_global_string("");
//     auto *level = m_builder.getInt32(0);
//     return emit_runtime_call("import", {name_str, empty_str, level});
// }

llvm::Value *IREmitter::call_import(std::string_view name,
	llvm::Value *globals,
	llvm::Value *fromlist,
	int level)
{
	auto *name_str = create_global_string(name);

	// globals 默认传 null（AOT 场景下由运行时从当前模块取）
	if (!globals) { globals = null_pyobject(); }
	// fromlist 默认 null（等价于 import foo，不是 from foo import bar）
	if (!fromlist) { fromlist = null_pyobject(); }
	// locals 在 CPython 语义里和 globals 相同，传 null 即可
	llvm::Value *locals = null_pyobject();

	auto *level_val = m_builder.getInt32(level);

	return emit_runtime_call("import", { name_str, globals, fromlist, locals, level_val });
}

llvm::Value *IREmitter::call_add_module(std::string_view name)
{
	auto *name_str = create_global_string(name);
	return emit_runtime_call("add_module", { name_str });
}

// =============================================================================
// Tier 0: 异常处理
// =============================================================================
void IREmitter::call_raise(llvm::Value *exception) { emit_runtime_call("raise", { exception }); }

llvm::Value *IREmitter::call_load_assertion_error()
{
	return emit_runtime_call("load_assertion_error", {});
}

llvm::Value *IREmitter::call_type_of(llvm::Value *obj)
{
	return emit_runtime_call("type_of", { obj });
}

llvm::Value *IREmitter::call_get_traceback(llvm::Value *exc)
{
	return emit_runtime_call("get_traceback", { exc });
}

// =============================================================================
// Tier 3: 更多容器创建
// =============================================================================

llvm::Value *IREmitter::create_dict(llvm::ArrayRef<llvm::Value *> keys,
	llvm::ArrayRef<llvm::Value *> values)
{
	PYLANG_ASSERT(keys.size() == values.size(), "keys and values size mismatch");

	if (keys.empty()) {
		auto *null_ptr =
			llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_builder.getContext()));
		return emit_runtime_call("build_dict", { m_builder.getInt32(0), null_ptr, null_ptr });
	}

	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), keys.size());

	// 创建 keys 数组
	auto *keys_arr = create_entry_block_alloca(arr_type, "dict_keys");
	for (size_t i = 0; i < keys.size(); ++i) {
		auto *gep = m_builder.CreateConstGEP2_32(arr_type, keys_arr, 0, static_cast<unsigned>(i));
		m_builder.CreateStore(keys[i], gep);
	}

	// 创建 values 数组
	auto *values_arr = create_entry_block_alloca(arr_type, "dict_values");
	for (size_t i = 0; i < values.size(); ++i) {
		auto *gep = m_builder.CreateConstGEP2_32(arr_type, values_arr, 0, static_cast<unsigned>(i));
		m_builder.CreateStore(values[i], gep);
	}

	auto *count = m_builder.getInt32(static_cast<uint32_t>(keys.size()));
	auto *keys_ptr =
		m_builder.CreateBitCast(keys_arr, llvm::PointerType::getUnqual(pyobject_ptr_type()));
	auto *values_ptr =
		m_builder.CreateBitCast(values_arr, llvm::PointerType::getUnqual(pyobject_ptr_type()));

	return emit_runtime_call("build_dict", { count, keys_ptr, values_ptr });
}

llvm::Value *IREmitter::create_set(llvm::ArrayRef<llvm::Value *> elements)
{
	if (elements.empty()) {
		auto *null_ptr =
			llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_builder.getContext()));
		return emit_runtime_call("build_set", { m_builder.getInt32(0), null_ptr });
	}

	auto *arr_type = llvm::ArrayType::get(pyobject_ptr_type(), elements.size());
	auto *arr = create_entry_block_alloca(arr_type, "set_elems");

	for (size_t i = 0; i < elements.size(); ++i) {
		auto *gep = m_builder.CreateConstGEP2_32(arr_type, arr, 0, static_cast<unsigned>(i));
		m_builder.CreateStore(elements[i], gep);
	}

	auto *count = m_builder.getInt32(static_cast<uint32_t>(elements.size()));
	auto *arr_ptr = m_builder.CreateBitCast(arr, llvm::PointerType::getUnqual(pyobject_ptr_type()));

	return emit_runtime_call("build_set", { count, arr_ptr });
}

llvm::Value *IREmitter::create_slice(llvm::Value *start, llvm::Value *stop, llvm::Value *step)
{
	return emit_runtime_call("build_slice", { start, stop, step });
}

// =============================================================================
// Tier 3: 更多容器方法
// =============================================================================

void IREmitter::call_list_extend(llvm::Value *list, llvm::Value *iterable)
{
	emit_runtime_call("list_extend", { list, iterable });
}

void IREmitter::call_dict_merge(llvm::Value *dict, llvm::Value *other)
{
	emit_runtime_call("dict_merge", { dict, other });
}

void IREmitter::call_dict_update(llvm::Value *dict, llvm::Value *other)
{
	emit_runtime_call("dict_update", { dict, other });
}

void IREmitter::call_set_update(llvm::Value *set, llvm::Value *iterable)
{
	emit_runtime_call("set_update", { set, iterable });
}

// =============================================================================
// Tier 1: 字节/复数字面量
// =============================================================================

llvm::Value *IREmitter::create_bytes(std::string_view data)
{
	// 注意：bytes 数据可能含 \0，不能用 create_global_string
	// 需要直接创建 ConstantDataArray
	auto *arr_type = llvm::ArrayType::get(m_builder.getInt8Ty(), data.size() + 1);
	std::vector<uint8_t> bytes(data.begin(), data.end());
	bytes.push_back(0);// null terminator
	auto *data_const = llvm::ConstantDataArray::get(m_builder.getContext(), bytes);
	auto *global = new llvm::GlobalVariable(
		*m_module, arr_type, true, llvm::GlobalValue::PrivateLinkage, data_const, ".bytes");
	global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

	auto *data_ptr = m_builder.CreateConstGEP2_32(arr_type, global, 0, 0);
	auto *len = m_builder.getInt64(static_cast<uint64_t>(data.size()));
	return emit_runtime_call("bytes_from_buffer", { data_ptr, len });
}

llvm::Value *IREmitter::create_complex(double real, double imag)
{
	auto *real_val = llvm::ConstantFP::get(m_builder.getDoubleTy(), real);
	auto *imag_val = llvm::ConstantFP::get(m_builder.getDoubleTy(), imag);
	return emit_runtime_call("complex_from_doubles", { real_val, imag_val });
}

// =============================================================================
// Tier 2: 解包操作
// =============================================================================

void IREmitter::call_unpack_sequence(llvm::Value *iterable, int32_t count, llvm::Value *out_array)
{
	auto *count_val = m_builder.getInt32(count);
	emit_runtime_call("unpack_sequence", { iterable, count_val, out_array });
}

// =============================================================================
// Tier 4: 闭包操作 (Phase 3.2)
// =============================================================================

llvm::Value *IREmitter::call_create_cell(llvm::Value *value)
{
	if (!value) { value = null_pyobject(); }
	return emit_runtime_call("create_cell", { value });
}

llvm::Value *IREmitter::call_cell_get(llvm::Value *cell)
{
	return emit_runtime_call("cell_get", { cell });
}

void IREmitter::call_cell_set(llvm::Value *cell, llvm::Value *value)
{
	emit_runtime_call("cell_set", { cell, value });
}

// =============================================================================
// Tier 6: 异常匹配 (Phase 3.3)
// =============================================================================

llvm::Value *IREmitter::call_check_exception_match(llvm::Value *exc, llvm::Value *exc_type)
{
	return emit_runtime_call("check_exception_match", { exc, exc_type });
}

void IREmitter::call_reraise(llvm::Value *exc)
{
	if (!exc) { exc = null_pyobject(); }
	emit_runtime_call("reraise", { exc });
}

// =============================================================================
// Tier 4: 函数创建 (Phase 3.2)
// =============================================================================

llvm::Value *IREmitter::call_make_function(std::string_view name,
	llvm::Value *code_ptr,
	llvm::Value *module,
	llvm::Value *defaults,
	llvm::Value *kwdefaults,
	llvm::Value *closure)
{
	auto *name_str = create_global_string(name);
	if (!module) { module = null_pyobject(); }
	if (!defaults) { defaults = null_pyobject(); }
	if (!kwdefaults) { kwdefaults = null_pyobject(); }
	if (!closure) { closure = null_pyobject(); }

	return emit_runtime_call(
		"make_function", { name_str, code_ptr, module, defaults, kwdefaults, closure });
}

llvm::Value *IREmitter::call_get_closure(llvm::Value *func)
{
	return emit_runtime_call("get_closure", { func });
}

// =============================================================================
// Tier 5: 类创建 (Phase 3.3)
// =============================================================================

llvm::Value *IREmitter::call_load_build_class()
{
	return emit_runtime_call("load_build_class", {});
}

llvm::Value *IREmitter::call_build_class_aot(llvm::Value *body_fn,
	std::string_view class_name,
	llvm::Value *bases_tuple,
	llvm::Value *kwargs)
{
	auto *name_str = create_global_string(class_name);
	return emit_runtime_call("build_class_aot", { body_fn, name_str, bases_tuple, kwargs });
}

void IREmitter::call_dict_setitem_str(llvm::Value *dict, std::string_view key, llvm::Value *value)
{
	auto *key_str = create_global_string(key);
	emit_runtime_call("dict_setitem_str", { dict, key_str, value });
}

llvm::Value *IREmitter::call_dict_getitem_str(llvm::Value *dict, std::string_view key)
{
	auto *key_str = create_global_string(key);
	return emit_runtime_call("dict_getitem_str", { dict, key_str });
}

void IREmitter::call_unpack_ex(llvm::Value *iterable,
	int32_t before,
	int32_t after,
	llvm::Value *out_array)
{
	auto *before_val = m_builder.getInt32(before);
	auto *after_val = m_builder.getInt32(after);
	emit_runtime_call("unpack_ex", { iterable, before_val, after_val, out_array });
}

llvm::Value *IREmitter::call_tuple_getitem(llvm::Value *tuple, llvm::Value *index)
{
	// index 必须是 i32
	return emit_runtime_call("tuple_getitem", { tuple, index });
}

// 发射对 rt_value_array_get 的调用
llvm::Value *IREmitter::call_value_array_get(llvm::Value *array_ptr, llvm::Value *index)
{
	return emit_runtime_call("value_array_get", { array_ptr, index });
}

llvm::Value *IREmitter::call_tuple_len(llvm::Value *tuple)
{
	return emit_runtime_call("tuple_size", { tuple });
}


llvm::Value *IREmitter::call_catch_begin(llvm::Value *exc_ptr)
{
	return emit_runtime_call("catch_begin", { exc_ptr });
}

void IREmitter::call_catch_end() { emit_runtime_call("catch_end", {}); }

void IREmitter::call_print_unhandled_exception(llvm::Value *exc)
{
	emit_runtime_call("print_unhandled_exception", { exc });
}

// [Add] Tier 3: 容器转换
llvm::Value *IREmitter::call_list_to_tuple(llvm::Value *list)
{
	return emit_runtime_call("list_to_tuple", { list });
}

// =============================================================================
// 生命周期
// =============================================================================

void IREmitter::emit_init() { emit_runtime_call("init", {}); }

void IREmitter::emit_shutdown() { emit_runtime_call("shutdown", {}); }

}// namespace pylang
