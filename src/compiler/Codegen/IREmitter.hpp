#pragma once

#include "compiler/RuntimeLinker/RuntimeLinker.hpp"
#include "compiler/Support/Error.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <string_view>
#include <unordered_map>

namespace pylang {

// =============================================================================
// IREmitter — Runtime 函数调用生成器
//
// 职责:
//   - 封装 RuntimeLinker，提供高层 API
//   - 生成对 runtime 函数的 LLVM IR 调用
//   - 管理常量字符串的全局变量
//
// 设计原则:
//   - 方法返回 llvm::Value*（可能为 nullptr），不使用 Result<T>
//   - 调用者（PylangCodegen）负责检查 nullptr 并转换为 Result<T>
//   - 所有错误通过 log::codegen() 记录
//
// 用法:
//   IREmitter emitter(builder, linker, module);
//   auto *result = emitter.call_binary_add(lhs, rhs);
//   if (!result) { return MAKE_ERROR(...); }
// =============================================================================
class IREmitter
{
  public:
	IREmitter(llvm::IRBuilder<> &builder, RuntimeLinker &linker, llvm::Module *module)
		: m_builder(builder), m_linker(linker), m_module(module)
	{}

	// ========== Tier 0: 单例 ==========
	llvm::Value *get_none();
	llvm::Value *get_true();
	llvm::Value *get_false();
	llvm::Value *get_ellipsis();
	llvm::Value *get_not_implemented();

	// ========== Tier 0: 对象创建 ==========
	llvm::Value *create_string(std::string_view str);
	llvm::Value *create_tuple(llvm::ArrayRef<llvm::Value *> elements);

	// ========== Tier 1: 更多对象创建 ==========
	llvm::Value *create_integer(int64_t value);
	llvm::Value *create_integer_big(std::string_view decimal_str);
	llvm::Value *create_float(double value);
	llvm::Value *create_list(llvm::ArrayRef<llvm::Value *> elements);
	llvm::Value *create_dict(llvm::ArrayRef<llvm::Value *> keys,
		llvm::ArrayRef<llvm::Value *> values);
	llvm::Value *create_set(llvm::ArrayRef<llvm::Value *> elements);
	llvm::Value *create_slice(llvm::Value *start, llvm::Value *stop, llvm::Value *step);

	// ========== Tier 1: 字节/复数字面量 ==========
	llvm::Value *create_bytes(std::string_view data);
	llvm::Value *create_complex(double real, double imag);

	// ========== Tier 1: 二元运算 ==========
	llvm::Value *call_binary_add(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_sub(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_mul(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_truediv(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_floordiv(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_mod(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_pow(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_lshift(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_rshift(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_and(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_or(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_binary_xor(llvm::Value *lhs, llvm::Value *rhs);
	// ========== Tier 1: 增量赋值（inplace）运算 ==========
	llvm::Value *call_inplace_add(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_sub(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_mul(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_truediv(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_floordiv(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_mod(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_pow(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_lshift(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_rshift(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_and(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_or(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_inplace_xor(llvm::Value *lhs, llvm::Value *rhs);


	// ========== Tier 1: 一元运算 ==========
	llvm::Value *call_unary_neg(llvm::Value *obj);
	llvm::Value *call_unary_pos(llvm::Value *obj);
	llvm::Value *call_unary_invert(llvm::Value *obj);
	llvm::Value *call_unary_not(llvm::Value *obj);

	// ========== Tier 1: 类型转换 ==========
	llvm::Value *call_is_true(llvm::Value *obj);

	// ========== Tier 2: 比较操作 ==========
	llvm::Value *call_compare_eq(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_ne(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_lt(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_le(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_gt(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_ge(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_is(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_is_not(llvm::Value *lhs, llvm::Value *rhs);
	llvm::Value *call_compare_in(llvm::Value *value, llvm::Value *container);
	llvm::Value *call_compare_not_in(llvm::Value *value, llvm::Value *container);

	// ========== Tier 2: 迭代器 ==========
	llvm::Value *call_get_iter(llvm::Value *obj);

	// ========== Tier 2: 解包 ==========
	void call_unpack_sequence(llvm::Value *iterable, int32_t count, llvm::Value *out_array);
	void call_unpack_ex(llvm::Value *iterable,
		int32_t before,
		int32_t after,
		llvm::Value *out_array);

	/// iter_next 返回下一个元素，has_value 是输出参数（i1*）
	/// 如果迭代结束，has_value 设为 false，返回值为 nullptr
	llvm::Value *call_iter_next(llvm::Value *iter, llvm::Value *has_value_out);

	// ========== Tier 3: 下标访问 ==========
	llvm::Value *call_getitem(llvm::Value *obj, llvm::Value *key);
	void call_setitem(llvm::Value *obj, llvm::Value *key, llvm::Value *value);
	void call_delitem(llvm::Value *obj, llvm::Value *key);

	// ========== Tier 3: 容器方法 ==========
	/// 获取 tuple 的元素数量，返回 i32
	llvm::Value *call_tuple_len(llvm::Value *tuple);
	void call_list_append(llvm::Value *list, llvm::Value *value);
	llvm::Value *call_list_to_tuple(llvm::Value *list);
	void call_set_add(llvm::Value *set, llvm::Value *value);
	void call_list_extend(llvm::Value *list, llvm::Value *iterable);
	void call_dict_merge(llvm::Value *dict, llvm::Value *other);
	void call_dict_update(llvm::Value *dict, llvm::Value *other);
	void call_set_update(llvm::Value *set, llvm::Value *iterable);
	// ========== Tier 0: 属性访问 ==========
	llvm::Value *call_getattr(llvm::Value *obj, std::string_view name);
	llvm::Value *call_load_global(llvm::Value *module, std::string_view name);
	void call_store_global(llvm::Value *module, std::string_view name, llvm::Value *value);
	void call_setattr(llvm::Value *obj, std::string_view name, llvm::Value *value);
	void call_delattr(llvm::Value *obj, std::string_view name);


	// ========== Tier 0: 函数调用 ==========
	llvm::Value *
		call_function(llvm::Value *callable, llvm::Value *args, llvm::Value *kwargs = nullptr);

	// 纯指针数组调用
	llvm::Value *call_function_raw_ptrs(llvm::Value *callable,
		llvm::Value *args_ptr,
		llvm::Value *argc,
		llvm::Value *kwargs);
	llvm::Value *call_method_raw_ptrs(llvm::Value *owner,
		llvm::Value *method_name_cstr,
		llvm::Value *args_ptr,
		llvm::Value *argc,
		llvm::Value *kwargs);

	// ========== Tier 4: 方法调用 ==========
	llvm::Value *call_load_method(llvm::Value *obj, std::string_view method_name);
	llvm::Value *call_method_fast(llvm::Value *obj,
		std::string_view name,
		llvm::ArrayRef<llvm::Value *> args);

	// ========== Phase 4+: 快速调用 ==========
	/// 直接传 C 数组调用，避免 PyTuple 堆分配
	llvm::Value *call_function_fast(llvm::Value *callable, llvm::ArrayRef<llvm::Value *> args);


	// ========== 模块导入 ==========
	llvm::Value *call_import(std::string_view name,
		llvm::Value *globals = nullptr,
		llvm::Value *fromlist = nullptr,
		int level = 0);

	llvm::Value *call_add_module(std::string_view name);

	// ========== Tier 0: 异常处理 ==========
	void call_raise(llvm::Value *exception);
	llvm::Value *call_load_assertion_error();

	// 底层异常处理 (ABI)
	llvm::Value *call_catch_begin(llvm::Value *exc_ptr);
	void call_catch_end();
	void call_print_unhandled_exception(llvm::Value *exc);

	// [Add] 异常信息提取
	llvm::Value *call_type_of(llvm::Value *obj);
	llvm::Value *call_get_traceback(llvm::Value *exc);

	// ========== Tier 4: 闭包操作 (Phase 3.2) ==========
	llvm::Value *call_create_cell(llvm::Value *value);
	llvm::Value *call_cell_get(llvm::Value *cell);
	void call_cell_set(llvm::Value *cell, llvm::Value *value);

	// ========== Tier 4: tuple 特化访问 ==========
	// 快速元组访问 (用于闭包、参数解包)
	// [Fix] 参数类型改为 llvm::Value* 以匹配 m_builder.getInt32(...)
	llvm::Value *call_tuple_getitem(llvm::Value *tuple, llvm::Value *index);

	// ✅ 新增：从 Value 数组提取 PyObject*
	llvm::Value *call_value_array_get(llvm::Value *array_ptr, llvm::Value *index);

	// ========== Tier 4: 函数创建 (Phase 3.2) ==========
	/// 从 AOT 编译后的原生函数指针创建 Python 可调用对象
	///
	/// @param name       函数名
	/// @param code_ptr   编译后的 LLVM Function*（作为 ptr 传入）
	/// @param module     所属模块
	/// @param defaults   默认值 PyTuple*，或 null
	/// @param kwdefaults 关键字默认值 PyDict*，或 null
	/// @param closure    闭包 cell 元组 PyTuple*，或 null
	llvm::Value *call_make_function(std::string_view name,
		llvm::Value *code_ptr,
		llvm::Value *module,
		llvm::Value *defaults = nullptr,
		llvm::Value *kwdefaults = nullptr,
		llvm::Value *closure = nullptr);

	/// 获取函数的闭包 cell 元组
	llvm::Value *call_get_closure(llvm::Value *func);

	// ========== Tier 5: 类创建 (Phase 3.3) ==========
	/// 加载 builtins.__build_class__
	llvm::Value *call_load_build_class();

	/// AOT 类创建: rt_build_class_aot(body_fn, name, bases, kwargs)
	llvm::Value *call_build_class_aot(llvm::Value *body_fn,
		std::string_view class_name,
		llvm::Value *bases_tuple,
		llvm::Value *kwargs);

	/// 向 dict 写入字符串键条目（类体 namespace）
	void call_dict_setitem_str(llvm::Value *dict, std::string_view key, llvm::Value *value);

	/// 从 dict 读取字符串键条目（类体 namespace）
	llvm::Value *call_dict_getitem_str(llvm::Value *dict, std::string_view key);


	// ========== Tier 6: 异常匹配 (Phase 3.3) ==========
	llvm::Value *call_check_exception_match(llvm::Value *exc, llvm::Value *exc_type);
	void call_reraise(llvm::Value *exc);

	// ========== 生命周期 ==========
	void emit_init();
	void emit_shutdown();
	// ========== 辅助 ==========
	llvm::Type *pyobject_ptr_type() const { return m_linker.pyobject_ptr_type(); }
	llvm::LLVMContext &context() const { return m_builder.getContext(); }
	/// 创建 null PyObject* 常量
	llvm::Constant *null_pyobject() const;

	/// 在当前函数的 entry block 创建 AllocaInst (解决循环爆栈)
	llvm::AllocaInst *create_entry_block_alloca(llvm::Type *type, const std::string &name);

	/// C++ EH 辅助
	void declare_eh_intrinsics();
	llvm::Function *get_personality_function();
	llvm::Value *emit_landingpad(llvm::BasicBlock *lp_bb);

	/// 获取 PylangException 的 typeinfo（用于 landingpad catch 子句）
	llvm::Constant *get_pylang_exception_typeinfo();

	/// 设置/清除当前 unwind 目标（try 块入口设置，出口恢复）
	void set_unwind_dest(llvm::BasicBlock *bb) { m_unwind_dest = bb; }
	llvm::BasicBlock *unwind_dest() const { return m_unwind_dest; }

	// 获取属性名的全局 PyObject* 指针
	llvm::Value *get_interned_string_obj(std::string_view name);
	// 在模块初始化时生成所有缓存对象的初始化代码
	void emit_interned_strings_initialization();

  private:
	/// 创建全局字符串常量（带缓存）
	llvm::Constant *create_global_string(std::string_view str);

	// [Move] 移至 private，禁止外部直接调用
	/// 核心调用发射 — 统一接口
	/// 自动处理函数查找、invoke/call 选择
	llvm::Value *emit_runtime_call(std::string_view func_name,
		llvm::ArrayRef<llvm::Value *> args,
		llvm::BasicBlock *unwind_dest = nullptr);

	llvm::IRBuilder<> &m_builder;
	RuntimeLinker &m_linker;
	llvm::Module *m_module;

	// 字符串常量缓存
	std::unordered_map<std::string, llvm::Constant *> m_string_cache;

	// 当前 unwind 目标（try 块内非 null）
	llvm::BasicBlock *m_unwind_dest = nullptr;

	// 缓存：属性名 -> 全局变量 (ptr @.pystr.xxxx)
	std::unordered_map<std::string, llvm::GlobalVariable *> m_interned_string_objects;
};

}// namespace pylang