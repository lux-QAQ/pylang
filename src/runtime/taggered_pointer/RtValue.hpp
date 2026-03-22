#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include "runtime/Value.hpp"

namespace py {

class PyObject;

// =============================================================================
// RtValue — 零开销的统一值表示
//
// 内存布局 (64-bit platform):
//   bit 0 = 0 → 真实对象指针 `PyObject*`，对齐到 8 字节边界。
//   bit 0 = 1 → Thin 立即数 (Tagged Pointer)，不需要跨代追踪。
//     bit 1~63 → 63位有符号整数 (算术右移恢复原值)
// =============================================================================
class RtValue
{
  private:
	uintptr_t m_bits;

	// 强制内部显式构造，避免任何隐式转换导致的悬空错误
	constexpr explicit RtValue(uintptr_t bits) noexcept : m_bits(bits) {}

  public:
	// --- Tag 掩码与极值常量 ---
	static constexpr uintptr_t kTagMask = 1ULL;
	static constexpr uintptr_t kIntTag = 1ULL;
	static constexpr int kTagShift = 1;

	static constexpr int64_t kTaggedIntMax = (1LL << 62) - 1;
	static constexpr int64_t kTaggedIntMin = -(1LL << 62);

	// --- 构造机制 ---

	constexpr RtValue() noexcept : m_bits(0) {}

	/// 从真实的 PyObject* 构造
	[[nodiscard]] static constexpr RtValue from_ptr(const PyObject *obj) noexcept
	{
		return RtValue(reinterpret_cast<uintptr_t>(obj));
	}

	/// 从立即数构造 (调用前请确保数值在 kTaggedIntMin/Max 之间)
	[[nodiscard]] static constexpr RtValue from_int(int64_t value) noexcept
	{
		return RtValue((static_cast<uintptr_t>(value) << kTagShift) | kIntTag);
	}

	/// 安全构造: 如果可以直接作为 thin_int，则构造；否则强制装箱分配
	[[nodiscard]] static RtValue from_int_or_box(int64_t value);

	// --- 判断逻辑 (极致内联) ---

	[[nodiscard]] constexpr bool is_tagged_int() const noexcept
	{
		return (m_bits & kTagMask) == kIntTag;
	}
	[[nodiscard]] constexpr bool is_heap_object() const noexcept
	{
		return (m_bits & kTagMask) == 0 && m_bits != 0;
	}
	[[nodiscard]] constexpr bool is_null() const noexcept { return m_bits == 0; }

	[[nodiscard]] static constexpr bool fits_tagged_int(int64_t v) noexcept
	{
		return v >= kTaggedIntMin && v <= kTaggedIntMax;
	}

	/// [新增]：一词位运算同时检查两个原始指针是否都是 Tagged Integer
	/// 避免了在导出层分别调用 is_tagged_int() 导致的两次分支检查
	[[nodiscard]] static inline bool are_both_tagged_int(const PyObject *a,
		const PyObject *b) noexcept
	{
		return (reinterpret_cast<uintptr_t>(a) & reinterpret_cast<uintptr_t>(b) & kIntTag);
	}

	/// [新增]：重载版本，用于直接检查 RtValue 包装对象，避免在 RtValue.cpp 中调用 as_ptr()
	[[nodiscard]] static inline bool are_both_tagged_int(RtValue a, RtValue b) noexcept
	{
		return (a.m_bits & b.m_bits & kIntTag);
	}

	// --- 值提取 (极致内联) ---

	[[nodiscard]] constexpr PyObject *as_ptr() const noexcept
	{
		return reinterpret_cast<PyObject *>(m_bits);
	}

	[[nodiscard]] constexpr int64_t as_int() const noexcept
	{
		// 算术右移，自动处理符号位
		return static_cast<int64_t>(m_bits) >> kTagShift;
	}

	/// 用于与 LLVM IR 兼容：将值重新伪装回 PyObject*
	[[nodiscard]] constexpr PyObject *as_pyobject_raw() const noexcept
	{
		return reinterpret_cast<PyObject *>(m_bits);
	}

	// --- 核心装箱 ---

	/// 强制装箱：任何需要传入容器 / 获取属性的地方，必须先调用此方法
	/// 保证返回的是真实的 Heap 上的 PyObject*
	[[nodiscard]] PyObject *box() const;

	/// [新增]：转换为底层 variant 容器 Value
	[[nodiscard]] py::Value to_value() const;

	/// [新增]：从底层 variant 容器 Value 恢复为 RtValue
	[[nodiscard]] static RtValue from_value(const py::Value &v);

	/// 强制碾平：尝试将已经是堆对象的 PyInteger，剥离回零开销的 TaggedPointer
	[[nodiscard]] static RtValue flatten(PyObject *ptr);

	// --- 运算支持 (快速短路 + 慢速降级) ---

	[[nodiscard]] static RtValue add(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue sub(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue mul(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue floordiv(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue mod(RtValue lhs, RtValue rhs);

	[[nodiscard]] static RtValue bit_and(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue bit_or(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue bit_xor(RtValue lhs, RtValue rhs);

	[[nodiscard]] static RtValue lshift(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue rshift(RtValue lhs, RtValue rhs);

	[[nodiscard]] static RtValue compare_eq(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue compare_ne(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue compare_lt(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue compare_le(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue compare_gt(RtValue lhs, RtValue rhs);
	[[nodiscard]] static RtValue compare_ge(RtValue lhs, RtValue rhs);

	[[nodiscard]] bool is_truthy() const;
};

// =============================================================================
// 无锁内联方法缓存 (用于极速拦截 AOT 深层循环中的绑定方法分配地狱)
// =============================================================================
class MethodCache
{
  public:
	static PyObject *load_method(PyObject *obj, const char *name);
};

// =============================================================================
// 便捷内联装箱函数
// 用法:
//   PyObject* real_ptr = ensure_box(raw_ptr);
// =============================================================================
[[nodiscard]] inline PyObject *ensure_box(PyObject *ptr) { return RtValue::from_ptr(ptr).box(); }

}// namespace py