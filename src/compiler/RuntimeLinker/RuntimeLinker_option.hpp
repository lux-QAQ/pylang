#pragma once

namespace pylang {

struct RuntimeLinkerOptions
{
	/// 是否开启 declare_in 的函数缓存
	///
	/// 开启后，RuntimeLinker 会维护 std::unordered_map<Module*, map<name, Function*>>
	/// 以避免重复查找开销 (map lookup vs string mangling + symbol table lookup)。
	///
	/// 注意：
	/// RuntimeLinker 的生命周期通常长于单个 Module (如在编译器 Pipeline 中)。
	/// 如果 Module 被销毁，其地址即失效。如果后续新 Module
	/// 分配在相同地址（常见于单元测试栈内存复用）， 缓存将返回悬垂指针导致 Crash。
	///
	/// 解决方案：
	/// 1. 在 Module 销毁时调用 linker.forget_module(mod)
	/// 2. 或在测试 TearDown 时调用 linker.clear_cache()
	bool enable_function_cache = true;
};

}// namespace pylang