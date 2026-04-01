#include "IREmitter.hpp"

#include "compiler/Support/Log.hpp"

#include <llvm/IR/Constants.h>

namespace pylang {

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
llvm::Value *IREmitter::create_integer(int64_t value)
{
	auto *val = m_builder.getInt64(value);
	return emit_runtime_call("integer_from_i64", { val });
}

llvm::Value *IREmitter::create_integer_big(std::string_view decimal_str)
{
	// decimal_str 是编译期常量，嵌入 .rodata，运行时只调用一次
	auto *str_ptr = create_global_string(decimal_str);
	return emit_runtime_call("integer_from_string", { str_ptr });
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

llvm::Value *IREmitter::call_getattr(llvm::Value *obj, std::string_view name)
{
	// 从缓存加载 PyObject* (PyString)
	auto *gvar = get_interned_string_obj(name);
	auto *name_obj = m_builder.CreateLoad(m_builder.getPtrTy(), gvar);
	return emit_runtime_call("getattr_fast", { obj, name_obj });
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
	emit_runtime_call("setattr_fast", { obj, name_obj, value });
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