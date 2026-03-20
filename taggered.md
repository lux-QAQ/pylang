你的思路很好。核心问题是：如何在保持 `PyObject*` ABI 的前提下，让"轻量值"和"重量对象"无缝互操作，同时利用 C++ 类型系统减少手动判断代码。

让我分析你的方案并给出一个完整设计。

## 问题分析

你提到的几个关键矛盾：

1. LLVM IR 层面只有 `ptr`（`PyObject*`），无法传递 C++ 类
2. export 函数签名是 `PyObject*`，不能改成模板或重载
3. 需要在某处自动完成 thin → fat 的转换，不想在每个 export 函数里手动写
4. thin 对象的内存管理：如果不在 GC 堆上，谁来释放？

第 4 点是最关键的。如果 thin 对象是栈上的 C++ 值，它无法通过 `PyObject*` 指针传递（指针指向栈帧，函数返回后悬空）。如果在堆上分配，又回到了 GC 的问题。

这就是为什么所有高性能动态语言运行时最终都选择了 tagged pointer——它是唯一能在"零分配"和"指针 ABI 兼容"之间取得平衡的方案。

但你说得对，裸 tagged pointer 太简陋。我们可以用 C++ 包装它。

## 方案：`RtValue` — Tagged Pointer 的 C++ 封装

核心思想：用一个 C++ 类 `RtValue` 封装 tagged pointer 的所有细节，提供类型安全的 API 和自动装箱。export 层只需要在入口/出口处做一次 `RtValue` 转换。

````cpp
#pragma once

#include <cstdint>
#include <type_traits>

namespace py {

class PyObject;
class PyInteger;
class PyFloat;
class PyBool;

// =============================================================================
// RtValue — 统一值表示
//
// 内存布局 (64-bit pointer):
//   bit 0     = 0 → 真实 PyObject* (堆对象)
//   bit 0     = 1 → 立即数
//     bit 1-2 = 00 → tagged int (高 61 位存有符号整数)
//     bit 1-2 = 01 → 保留 (未来可用于 tagged float/bool/None)
//     bit 1-2 = 10 → 保留
//     bit 1-2 = 11 → 保留
//
// 设计原则:
//   - 对外仍然是 PyObject* (ABI 兼容)
//   - 内部用 C++ 类型系统保证安全
//   - 自动装箱: 需要真实 PyObject* 时调用 box()
//   - 零开销: 所有方法都是 inline，编译后就是位操作
// =============================================================================
class RtValue
{
	uintptr_t m_bits;

	// 内部构造
	explicit constexpr RtValue(uintptr_t bits) : m_bits(bits) {}

  public:
	// --- Tag 常量 ---
	static constexpr uintptr_t kTagMask     = 0b111;  // 低 3 位
	static constexpr uintptr_t kHeapTag     = 0b000;  // 真实指针 (8 字节对齐, 低 3 位为 0)
	static constexpr uintptr_t kIntTag      = 0b001;  // 立即整数
	// 未来扩展:
	// static constexpr uintptr_t kFloatTag = 0b011;
	// static constexpr uintptr_t kSpecialTag = 0b101; // None, True, False

	static constexpr int kIntShift = 3;
	static constexpr int64_t kIntMax = (1LL << 60) - 1;   // 约 ±1.15e18
	static constexpr int64_t kIntMin = -(1LL << 60);

	// --- 构造 ---

	/// 从 PyObject* 构造 (零开销)
	static RtValue from_ptr(PyObject *obj)
	{
		return RtValue{ reinterpret_cast<uintptr_t>(obj) };
	}

	/// 从立即整数构造 (零分配)
	static RtValue from_int(int64_t value)
	{
		return RtValue{ (static_cast<uintptr_t>(value) << kIntShift) | kIntTag };
	}

	/// 智能构造: 如果 value 在范围内返回 tagged int，否则创建 PyInteger
	static RtValue from_int_or_box(int64_t value);

	/// 从 PyObject* 原样包装 (不检查 tag)
	static RtValue from_raw(uintptr_t bits) { return RtValue{ bits }; }

	// --- 类型判断 ---

	bool is_heap_object() const { return (m_bits & 0b1) == kHeapTag && m_bits != 0; }
	bool is_tagged_int() const { return (m_bits & kTagMask) == kIntTag; }
	bool is_null() const { return m_bits == 0; }

	// --- 值提取 ---

	/// 获取立即整数值 (调用前必须 is_tagged_int())
	int64_t as_int() const
	{
		return static_cast<int64_t>(m_bits) >> kIntShift;  // 算术右移保留符号
	}

	/// 获取堆对象指针 (调用前必须 is_heap_object())
	PyObject *as_ptr() const
	{
		return reinterpret_cast<PyObject *>(m_bits);
	}

	// --- 转换为 PyObject* (ABI 兼容) ---

	/// 零开销转换: 直接 reinterpret 为 PyObject*
	/// 调用者负责在需要解引用时先 box()
	PyObject *as_pyobject_raw() const
	{
		return reinterpret_cast<PyObject *>(m_bits);
	}

	/// 装箱: 如果是 tagged int，创建 PyInteger 并返回
	/// 如果已经是堆对象，直接返回
	/// 这是 thin → fat 的唯一转换点
	PyObject *box() const;

	// --- 算术快速路径 ---

	/// 两个 RtValue 相加，尽量保持 tagged
	static RtValue add(RtValue lhs, RtValue rhs);
	static RtValue sub(RtValue lhs, RtValue rhs);
	static RtValue mul(RtValue lhs, RtValue rhs);
	static RtValue floordiv(RtValue lhs, RtValue rhs);
	static RtValue mod(RtValue lhs, RtValue rhs);

	/// 比较 (返回 RtValue 包装的 PyBool*)
	static RtValue compare_lt(RtValue lhs, RtValue rhs);
	static RtValue compare_le(RtValue lhs, RtValue rhs);
	static RtValue compare_eq(RtValue lhs, RtValue rhs);
	static RtValue compare_ne(RtValue lhs, RtValue rhs);
	static RtValue compare_gt(RtValue lhs, RtValue rhs);
	static RtValue compare_ge(RtValue lhs, RtValue rhs);

	/// 真值判断
	bool is_truthy() const;

	// --- 范围检查 ---
	static bool fits_tagged_int(int64_t v)
	{
		return v >= kIntMin && v <= kIntMax;
	}

	uintptr_t raw_bits() const { return m_bits; }
};

// =============================================================================
// ENSURE_BOX — export 函数入口处的自动装箱宏
//
// 用法:
//   void rt_list_append(PyObject *list, PyObject *value)
//   {
//       ENSURE_BOX(value);  // 如果 value 是 tagged int，就地替换为 PyInteger*
//       ...
//   }
// =============================================================================
#define ENSURE_BOX(var)                                                \
	do {                                                               \
		if (RtValue::from_ptr(var).is_tagged_int()) {                  \
			var = RtValue::from_ptr(var).box();                        \
		}                                                              \
	} while (0)

} // namespace py
````

````cpp
#include "RtValue.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyInteger.hpp"

namespace py {

PyObject *RtValue::box() const
{
	if (is_heap_object() || is_null()) { return as_ptr(); }
	if (is_tagged_int()) {
		// 装箱: 创建堆上的 PyInteger
		auto result = PyInteger::create(as_int());
		return result.unwrap();
	}
	// 未来扩展其他 tag 类型
	__builtin_unreachable();
}

RtValue RtValue::from_int_or_box(int64_t value)
{
	if (fits_tagged_int(value)) { return from_int(value); }
	auto *obj = PyInteger::create(value).unwrap();
	return from_ptr(obj);
}

// --- 算术实现 ---

RtValue RtValue::add(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		int64_t a = lhs.as_int();
		int64_t b = rhs.as_int();
		int64_t result;
		if (!__builtin_add_overflow(a, b, &result) && fits_tagged_int(result)) {
			return from_int(result);
		}
		// 溢出: 走 BigInt
		auto *obj = PyInteger::create(BigIntType{a} + BigIntType{b}).unwrap();
		return from_ptr(obj);
	}
	// 慢速路径: 装箱后走 runtime
	auto *result = lhs.box()->add(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::sub(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		int64_t a = lhs.as_int();
		int64_t b = rhs.as_int();
		int64_t result;
		if (!__builtin_sub_overflow(a, b, &result) && fits_tagged_int(result)) {
			return from_int(result);
		}
		auto *obj = PyInteger::create(BigIntType{a} - BigIntType{b}).unwrap();
		return from_ptr(obj);
	}
	auto *result = lhs.box()->subtract(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::mul(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		int64_t a = lhs.as_int();
		int64_t b = rhs.as_int();
		int64_t result;
		if (!__builtin_mul_overflow(a, b, &result) && fits_tagged_int(result)) {
			return from_int(result);
		}
		auto *obj = PyInteger::create(BigIntType{a} * BigIntType{b}).unwrap();
		return from_ptr(obj);
	}
	auto *result = lhs.box()->multiply(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::floordiv(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		int64_t a = lhs.as_int();
		int64_t b = rhs.as_int();
		if (b != 0) {
			// Python floordiv: 向负无穷取整
			int64_t q = a / b;
			int64_t r = a % b;
			if ((r != 0) && ((r ^ b) < 0)) { q--; }
			if (fits_tagged_int(q)) { return from_int(q); }
		}
		// b == 0 走 runtime 抛 ZeroDivisionError
	}
	auto *result = lhs.box()->floordiv(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::mod(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		int64_t a = lhs.as_int();
		int64_t b = rhs.as_int();
		if (b != 0) {
			int64_t r = a % b;
			if ((r != 0) && ((r ^ b) < 0)) { r += b; }
			return from_int(r);
		}
	}
	auto *result = lhs.box()->modulo(rhs.box()).unwrap();
	return from_ptr(result);
}

// --- 比较实现 ---

RtValue RtValue::compare_lt(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		return from_ptr(lhs.as_int() < rhs.as_int()
			? static_cast<PyObject *>(py_true())
			: static_cast<PyObject *>(py_false()));
	}
	auto *result = lhs.box()->lt(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::compare_le(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		return from_ptr(lhs.as_int() <= rhs.as_int()
			? static_cast<PyObject *>(py_true())
			: static_cast<PyObject *>(py_false()));
	}
	auto *result = lhs.box()->le(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::compare_eq(RtValue lhs, RtValue rhs)
{
	// 快速路径: bits 完全相同
	if (lhs.m_bits == rhs.m_bits) {
		return from_ptr(static_cast<PyObject *>(py_true()));
	}
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		return from_ptr(static_cast<PyObject *>(py_false()));  // bits 不同且都是 int
	}
	auto *result = lhs.box()->eq(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::compare_ne(RtValue lhs, RtValue rhs)
{
	if (lhs.m_bits == rhs.m_bits) {
		return from_ptr(static_cast<PyObject *>(py_false()));
	}
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		return from_ptr(static_cast<PyObject *>(py_true()));
	}
	auto *result = lhs.box()->ne(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::compare_gt(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		return from_ptr(lhs.as_int() > rhs.as_int()
			? static_cast<PyObject *>(py_true())
			: static_cast<PyObject *>(py_false()));
	}
	auto *result = lhs.box()->gt(rhs.box()).unwrap();
	return from_ptr(result);
}

RtValue RtValue::compare_ge(RtValue lhs, RtValue rhs)
{
	if (lhs.is_tagged_int() && rhs.is_tagged_int()) {
		return from_ptr(lhs.as_int() >= rhs.as_int()
			? static_cast<PyObject *>(py_true())
			: static_cast<PyObject *>(py_false()));
	}
	auto *result = lhs.box()->ge(rhs.box()).unwrap();
	return from_ptr(result);
}

bool RtValue::is_truthy() const
{
	if (is_null()) { return false; }
	if (is_tagged_int()) { return as_int() != 0; }
	return box()->true_().unwrap();
}

} // namespace py
````

## export 层改造

现在 export 层变得非常干净：

````cpp
#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/NotImplemented.hpp"
#include "runtime/RtValue.hpp"

// =============================================================================
// Tier 1: 二元运算 — 通过 RtValue 统一快速路径
// =============================================================================

PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
py::PyObject *rt_binary_add(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::add(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_sub", "obj", "obj,obj")
py::PyObject *rt_binary_sub(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::sub(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_mul", "obj", "obj,obj")
py::PyObject *rt_binary_mul(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::mul(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_truediv", "obj", "obj,obj")
py::PyObject *rt_binary_truediv(py::PyObject *lhs, py::PyObject *rhs)
{
	// truediv 总是返回 float，没有 tagged 快速路径
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->truediv(rhs));
}

PYLANG_EXPORT_OP("binary_floordiv", "obj", "obj,obj")
py::PyObject *rt_binary_floordiv(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::floordiv(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_mod", "obj", "obj,obj")
py::PyObject *rt_binary_mod(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::mod(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_pow", "obj", "obj,obj")
py::PyObject *rt_binary_pow(py::PyObject *lhs, py::PyObject *rhs)
{
	// pow 溢出概率高，直接走 runtime
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->pow(rhs, py::py_none()));
}

PYLANG_EXPORT_OP("binary_lshift", "obj", "obj,obj")
py::PyObject *rt_binary_lshift(py::PyObject *lhs, py::PyObject *rhs)
{
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->lshift(rhs));
}

PYLANG_EXPORT_OP("binary_rshift", "obj", "obj,obj")
py::PyObject *rt_binary_rshift(py::PyObject *lhs, py::PyObject *rhs)
{
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->rshift(rhs));
}

PYLANG_EXPORT_OP("binary_and", "obj", "obj,obj")
py::PyObject *rt_binary_and(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() & r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->and_(r.box()));
}

PYLANG_EXPORT_OP("binary_or", "obj", "obj,obj")
py::PyObject *rt_binary_or(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() | r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->or_(r.box()));
}

PYLANG_EXPORT_OP("binary_xor", "obj", "obj,obj")
py::PyObject *rt_binary_xor(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() ^ r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->xor_(r.box()));
}

// =============================================================================
// Tier 1: 一元运算
// =============================================================================

PYLANG_EXPORT_OP("unary_neg", "obj", "obj")
py::PyObject *rt_unary_neg(py::PyObject *obj)
{
	auto v = py::RtValue::from_ptr(obj);
	if (v.is_tagged_int()) {
		int64_t val = v.as_int();
		int64_t result;
		if (!__builtin_sub_overflow(int64_t{0}, val, &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	return rt_unwrap(v.box()->neg());
}

PYLANG_EXPORT_OP("unary_pos", "obj", "obj")
py::PyObject *rt_unary_pos(py::PyObject *obj)
{
	auto v = py::RtValue::from_ptr(obj);
	if (v.is_tagged_int()) { return obj; }
	return rt_unwrap(obj->pos());
}

PYLANG_EXPORT_OP("unary_invert", "obj", "obj")
py::PyObject *rt_unary_invert(py::PyObject *obj)
{
	auto v = py::RtValue::from_ptr(obj);
	if (v.is_tagged_int()) {
		return py::RtValue::from_int(~v.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(obj->invert());
}

PYLANG_EXPORT_OP("unary_not", "obj", "obj")
py::PyObject *rt_unary_not(py::PyObject *obj)
{
	return py::RtValue::from_ptr(obj).is_truthy() ? py::py_false() : py::py_true();
}

// =============================================================================
// Tier 1: 增量赋值 — tagged int 是不可变的，直接走 binary 路径
// =============================================================================

namespace {

py::PyObject *try_inplace_method(py::PyObject *lhs, py::PyObject *rhs, const char *method_name)
{
	// ...existing code...
	auto name = py::PyString::create(method_name);
	if (name.is_err()) { return nullptr; }

	auto [result, found] = lhs->lookup_attribute(name.unwrap());
	if (found != py::LookupAttrResult::FOUND || result.is_err()) { return nullptr; }

	auto args = py::PyTuple::create(rhs);
	if (args.is_err()) { return nullptr; }

	auto call_result = result.unwrap()->call(args.unwrap(), nullptr);
	if (call_result.is_err()) { return nullptr; }

	auto *ret = call_result.unwrap();
	if (ret == py::not_implemented()) { return nullptr; }

	return ret;
}

} // namespace

// 宏: inplace 运算的通用模式
// 如果两边都是 tagged int，走 RtValue 快速路径
// 否则装箱后尝试 __ixxx__，再退回 __xxx__
#define INPLACE_OP(op_name, rtvalue_op, fallback_method, dunder)                \
	PYLANG_EXPORT_OP("inplace_" #op_name, "obj", "obj,obj")                    \
	py::PyObject *rt_inplace_##op_name(py::PyObject *lhs, py::PyObject *rhs)   \
	{                                                                           \
		auto l = py::RtValue::from_ptr(lhs);                                   \
		auto r = py::RtValue::from_ptr(rhs);                                   \
		if (l.is_tagged_int() && r.is_tagged_int()) {                          \
			return py::RtValue::rtvalue_op(l, r).as_pyobject_raw();            \
		}                                                                       \
		auto *lbox = l.box();                                                  \
		auto *rbox = r.box();                                                  \
		if (auto *ret = try_inplace_method(lbox, rbox, "__i" #dunder "__")) {  \
			return ret;                                                         \
		}                                                                       \
		return rt_unwrap(lbox->fallback_method(rbox));                         \
	}

INPLACE_OP(add, add, add, add)
INPLACE_OP(sub, sub, subtract, sub)
INPLACE_OP(mul, mul, multiply, mul)
INPLACE_OP(floordiv, floordiv, floordiv, floordiv)
INPLACE_OP(mod, mod, modulo, mod)

#undef INPLACE_OP

// 没有 tagged 快速路径的 inplace 运算
#define INPLACE_OP_BOXED(op_name, fallback_method, dunder)                      \
	PYLANG_EXPORT_OP("inplace_" #op_name, "obj", "obj,obj")                    \
	py::PyObject *rt_inplace_##op_name(py::PyObject *lhs, py::PyObject *rhs)   \
	{                                                                           \
		ENSURE_BOX(lhs);                                                       \
		ENSURE_BOX(rhs);                                                       \
		if (auto *ret = try_inplace_method(lhs, rhs, "__i" #dunder "__")) {    \
			return ret;                                                         \
		}                                                                       \
		return rt_unwrap(lhs->fallback_method(rhs));                           \
	}

INPLACE_OP_BOXED(truediv, truediv, truediv)
INPLACE_OP_BOXED(pow, pow(rhs, py::py_none()), pow)  // 需要特殊处理 pow 的第三参数
INPLACE_OP_BOXED(lshift, lshift, lshift)
INPLACE_OP_BOXED(rshift, rshift, rshift)

#undef INPLACE_OP_BOXED

// 位运算 inplace: tagged int 快速路径
PYLANG_EXPORT_OP("inplace_and", "obj", "obj,obj")
py::PyObject *rt_inplace_and(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() & r.as_int()).as_pyobject_raw();
	}
	auto *lbox = l.box(); auto *rbox = r.box();
	if (auto *ret = try_inplace_method(lbox, rbox, "__iand__")) { return ret; }
	return rt_unwrap(lbox->and_(rbox));
}

PYLANG_EXPORT_OP("inplace_or", "obj", "obj,obj")
py::PyObject *rt_inplace_or(py::PyObject *// filepath: /home/lux/code/language/python-cpp/src/runtime/export/rt_op.cpp
#include "rt_common.hpp"

#include "runtime/PyBool.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/NotImplemented.hpp"
#include "runtime/RtValue.hpp"

// =============================================================================
// Tier 1: 二元运算 — 通过 RtValue 统一快速路径
// =============================================================================

PYLANG_EXPORT_OP("binary_add", "obj", "obj,obj")
py::PyObject *rt_binary_add(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::add(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_sub", "obj", "obj,obj")
py::PyObject *rt_binary_sub(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::sub(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_mul", "obj", "obj,obj")
py::PyObject *rt_binary_mul(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::mul(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_truediv", "obj", "obj,obj")
py::PyObject *rt_binary_truediv(py::PyObject *lhs, py::PyObject *rhs)
{
	// truediv 总是返回 float，没有 tagged 快速路径
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->truediv(rhs));
}

PYLANG_EXPORT_OP("binary_floordiv", "obj", "obj,obj")
py::PyObject *rt_binary_floordiv(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::floordiv(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_mod", "obj", "obj,obj")
py::PyObject *rt_binary_mod(py::PyObject *lhs, py::PyObject *rhs)
{
	return py::RtValue::mod(
		py::RtValue::from_ptr(lhs),
		py::RtValue::from_ptr(rhs)
	).as_pyobject_raw();
}

PYLANG_EXPORT_OP("binary_pow", "obj", "obj,obj")
py::PyObject *rt_binary_pow(py::PyObject *lhs, py::PyObject *rhs)
{
	// pow 溢出概率高，直接走 runtime
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->pow(rhs, py::py_none()));
}

PYLANG_EXPORT_OP("binary_lshift", "obj", "obj,obj")
py::PyObject *rt_binary_lshift(py::PyObject *lhs, py::PyObject *rhs)
{
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->lshift(rhs));
}

PYLANG_EXPORT_OP("binary_rshift", "obj", "obj,obj")
py::PyObject *rt_binary_rshift(py::PyObject *lhs, py::PyObject *rhs)
{
	ENSURE_BOX(lhs);
	ENSURE_BOX(rhs);
	return rt_unwrap(lhs->rshift(rhs));
}

PYLANG_EXPORT_OP("binary_and", "obj", "obj,obj")
py::PyObject *rt_binary_and(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() & r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->and_(r.box()));
}

PYLANG_EXPORT_OP("binary_or", "obj", "obj,obj")
py::PyObject *rt_binary_or(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() | r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->or_(r.box()));
}

PYLANG_EXPORT_OP("binary_xor", "obj", "obj,obj")
py::PyObject *rt_binary_xor(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() ^ r.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(l.box()->xor_(r.box()));
}

// =============================================================================
// Tier 1: 一元运算
// =============================================================================

PYLANG_EXPORT_OP("unary_neg", "obj", "obj")
py::PyObject *rt_unary_neg(py::PyObject *obj)
{
	auto v = py::RtValue::from_ptr(obj);
	if (v.is_tagged_int()) {
		int64_t val = v.as_int();
		int64_t result;
		if (!__builtin_sub_overflow(int64_t{0}, val, &result)
			&& py::RtValue::fits_tagged_int(result)) {
			return py::RtValue::from_int(result).as_pyobject_raw();
		}
	}
	return rt_unwrap(v.box()->neg());
}

PYLANG_EXPORT_OP("unary_pos", "obj", "obj")
py::PyObject *rt_unary_pos(py::PyObject *obj)
{
	auto v = py::RtValue::from_ptr(obj);
	if (v.is_tagged_int()) { return obj; }
	return rt_unwrap(obj->pos());
}

PYLANG_EXPORT_OP("unary_invert", "obj", "obj")
py::PyObject *rt_unary_invert(py::PyObject *obj)
{
	auto v = py::RtValue::from_ptr(obj);
	if (v.is_tagged_int()) {
		return py::RtValue::from_int(~v.as_int()).as_pyobject_raw();
	}
	return rt_unwrap(obj->invert());
}

PYLANG_EXPORT_OP("unary_not", "obj", "obj")
py::PyObject *rt_unary_not(py::PyObject *obj)
{
	return py::RtValue::from_ptr(obj).is_truthy() ? py::py_false() : py::py_true();
}

// =============================================================================
// Tier 1: 增量赋值 — tagged int 是不可变的，直接走 binary 路径
// =============================================================================

namespace {

py::PyObject *try_inplace_method(py::PyObject *lhs, py::PyObject *rhs, const char *method_name)
{
	// ...existing code...
	auto name = py::PyString::create(method_name);
	if (name.is_err()) { return nullptr; }

	auto [result, found] = lhs->lookup_attribute(name.unwrap());
	if (found != py::LookupAttrResult::FOUND || result.is_err()) { return nullptr; }

	auto args = py::PyTuple::create(rhs);
	if (args.is_err()) { return nullptr; }

	auto call_result = result.unwrap()->call(args.unwrap(), nullptr);
	if (call_result.is_err()) { return nullptr; }

	auto *ret = call_result.unwrap();
	if (ret == py::not_implemented()) { return nullptr; }

	return ret;
}

} // namespace

// 宏: inplace 运算的通用模式
// 如果两边都是 tagged int，走 RtValue 快速路径
// 否则装箱后尝试 __ixxx__，再退回 __xxx__
#define INPLACE_OP(op_name, rtvalue_op, fallback_method, dunder)                \
	PYLANG_EXPORT_OP("inplace_" #op_name, "obj", "obj,obj")                    \
	py::PyObject *rt_inplace_##op_name(py::PyObject *lhs, py::PyObject *rhs)   \
	{                                                                           \
		auto l = py::RtValue::from_ptr(lhs);                                   \
		auto r = py::RtValue::from_ptr(rhs);                                   \
		if (l.is_tagged_int() && r.is_tagged_int()) {                          \
			return py::RtValue::rtvalue_op(l, r).as_pyobject_raw();            \
		}                                                                       \
		auto *lbox = l.box();                                                  \
		auto *rbox = r.box();                                                  \
		if (auto *ret = try_inplace_method(lbox, rbox, "__i" #dunder "__")) {  \
			return ret;                                                         \
		}                                                                       \
		return rt_unwrap(lbox->fallback_method(rbox));                         \
	}

INPLACE_OP(add, add, add, add)
INPLACE_OP(sub, sub, subtract, sub)
INPLACE_OP(mul, mul, multiply, mul)
INPLACE_OP(floordiv, floordiv, floordiv, floordiv)
INPLACE_OP(mod, mod, modulo, mod)

#undef INPLACE_OP

// 没有 tagged 快速路径的 inplace 运算
#define INPLACE_OP_BOXED(op_name, fallback_method, dunder)                      \
	PYLANG_EXPORT_OP("inplace_" #op_name, "obj", "obj,obj")                    \
	py::PyObject *rt_inplace_##op_name(py::PyObject *lhs, py::PyObject *rhs)   \
	{                                                                           \
		ENSURE_BOX(lhs);                                                       \
		ENSURE_BOX(rhs);                                                       \
		if (auto *ret = try_inplace_method(lhs, rhs, "__i" #dunder "__")) {    \
			return ret;                                                         \
		}                                                                       \
		return rt_unwrap(lhs->fallback_method(rhs));                           \
	}

INPLACE_OP_BOXED(truediv, truediv, truediv)
INPLACE_OP_BOXED(pow, pow(rhs, py::py_none()), pow)  // 需要特殊处理 pow 的第三参数
INPLACE_OP_BOXED(lshift, lshift, lshift)
INPLACE_OP_BOXED(rshift, rshift, rshift)

#undef INPLACE_OP_BOXED

// 位运算 inplace: tagged int 快速路径
PYLANG_EXPORT_OP("inplace_and", "obj", "obj,obj")
py::PyObject *rt_inplace_and(py::PyObject *lhs, py::PyObject *rhs)
{
	auto l = py::RtValue::from_ptr(lhs);
	auto r = py::RtValue::from_ptr(rhs);
	if (l.is_tagged_int() && r.is_tagged_int()) {
		return py::RtValue::from_int(l.as_int() & r.as_int()).as_pyobject_raw();
	}
	auto *lbox = l.box(); auto *rbox = r.box();
	if (auto *ret = try_inplace_method(lbox, rbox, "__iand__")) { return ret; }
	return rt_unwrap(lbox->and_(rbox));
}

PYLANG_EXPORT_OP("inplace_or", "obj", "obj,obj")
py::PyObject *rt_inplace_or(py::PyObject *