#include "RtValue.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyComplex.hpp"
#include "runtime/PyFloat.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyNumber.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/Value.hpp"

#include "runtime/types/api.hpp"

#include <iostream>
#include <variant>

namespace py {

PyObject *RtValue::box() const
{
	if (is_heap_object() || is_null()) { return as_ptr(); }
	if (is_tagged_int()) {
		int64_t val = as_int();

		// [修复]：绝对禁止使用 thread_local！
		// 将其变成全局的静态区域。Boehm GC (BDWGC) 自动强力扫描全程序的 .bss 与 .data 段。
		// 在分配完毕后留存在这里的指针将永远被 GC 看作 Root（生命线存活），永远不会被野悬空收集！
		if (val >= -5 && val <= 4096) {
			static PyObject *s_cache[4102] = { nullptr };
			PyObject *&slot = s_cache[val + 5];
			if (!slot) { slot = PyInteger::create(val).unwrap(); }
			return slot;
		}

		// 对于无法通过缓存复用的数据才走分配
		auto result = PyInteger::create(val);
		return result.unwrap();
	}
	__builtin_unreachable();
}

RtValue RtValue::flatten(PyObject *ptr)
{
	RtValue rt = from_ptr(ptr);
	if (rt.is_tagged_int() || rt.is_null()) { return rt; }

	// [修复] 恢复使用 Pylang 原本正确的内建自研 RTTI 进行判定！动态层绝不失效
	if (auto *pyint = py::as<PyInteger>(ptr)) {
		const auto &num = pyint->value();
		if (std::holds_alternative<mpz_class>(num.value)) {
			const auto &gmp_val = std::get<mpz_class>(num.value);
			if (gmp_val.fits_slong_p()) {
				long raw_val = gmp_val.get_si();
				if (fits_tagged_int(raw_val)) { return from_int(raw_val); }
			}
		}
	}

	// [补充] Python 3.9 语义：bool 无缝参与算术等效于 0 或 1。
	// 如果由于某种原因被强行送进运算的 ptr 实质是个真/假的 bool Heap对象。我们也直接短路。
	if (auto *pybool = py::as<PyBool>(ptr)) { return from_int(pybool->value() ? 1 : 0); }

	return rt;
}

RtValue RtValue::from_int_or_box(int64_t value)
{
	if (fits_tagged_int(value)) { return from_int(value); }
	auto result = PyInteger::create(value);
	return from_ptr(result.unwrap());
}

// =============================================================================
// 数学运算
// =============================================================================

RtValue RtValue::add(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		int64_t result;
		bool isint = (!__builtin_add_overflow(l, r, &result) && fits_tagged_int(result));
		// std::cout <<"add(isint): " << isint << "\n";
		if (isint) {
			// std::cout <<"from_int(result); " << isint << "\n";
			return from_int(result);
		}
	}
	PyObject *res = lhs.box()->add(rhs.box()).unwrap();
	return from_ptr(res);
}

RtValue RtValue::sub(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		int64_t result;
		if (!__builtin_sub_overflow(l, r, &result) && fits_tagged_int(result)) {
			return from_int(result);
		}
	}
	PyObject *res = lhs.box()->subtract(rhs.box()).unwrap();
	return from_ptr(res);
}

RtValue RtValue::mul(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		int64_t result;
		if (!__builtin_mul_overflow(l, r, &result) && fits_tagged_int(result)) {
			return from_int(result);
		}
	}
	PyObject *res = lhs.box()->multiply(rhs.box()).unwrap();
	return from_ptr(res);
}

RtValue RtValue::floordiv(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		if (r != 0) {
			int64_t q = l / r;
			int64_t rem = l % r;
			if ((rem != 0) && ((rem ^ r) < 0)) { q--; }
			if (fits_tagged_int(q)) { return from_int(q); }
		}
	}
	PyObject *res = lhs.box()->floordiv(rhs.box()).unwrap();
	return from_ptr(res);
}

RtValue RtValue::mod(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		if (r != 0) {
			int64_t rem = l % r;
			if ((rem != 0) && ((rem ^ r) < 0)) { rem += r; }
			return from_int(rem);
		}
	}
	PyObject *res = lhs.box()->modulo(rhs.box()).unwrap();
	return from_ptr(res);
}

RtValue RtValue::bit_and(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) { return from_int(lhs.as_int() & rhs.as_int()); }
	return from_ptr(lhs.box()->and_(rhs.box()).unwrap());
}

RtValue RtValue::bit_or(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) { return from_int(lhs.as_int() | rhs.as_int()); }
	return from_ptr(lhs.box()->or_(rhs.box()).unwrap());
}

RtValue RtValue::bit_xor(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) { return from_int(lhs.as_int() ^ rhs.as_int()); }
	return from_ptr(lhs.box()->xor_(rhs.box()).unwrap());
}

RtValue RtValue::lshift(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		// Python 语义：负数位移抛出 ValueError，交由底层 Box 逻辑去抛出
		if (r >= 0 && r < 63) {
			int64_t result;
			// lshift 相当于乘 2^r。使用内置函数防止溢出改变符号位
			if (!__builtin_mul_overflow(l, 1LL << r, &result) && fits_tagged_int(result)) {
				return from_int(result);
			}
		}
	}
	return from_ptr(lhs.box()->lshift(rhs.box()).unwrap());
}

RtValue RtValue::rshift(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		int64_t l = lhs.as_int();
		int64_t r = rhs.as_int();
		if (r >= 0) {
			// C++ 中右移有符号数是算术右移。超出 64 位时行为未定义，需手动截断
			int64_t result = (r >= 64) ? (l < 0 ? -1 : 0) : (l >> r);
			return from_int(result);
		}
	}
	return from_ptr(lhs.box()->rshift(rhs.box()).unwrap());
}

// =============================================================================
// 比较运算
// =============================================================================

namespace {

	inline bool heap_object_may_break_reflexive_equality(PyObject *obj) noexcept
	{
		PyType *type = obj->type();
		// 仅对可能包含 NaN 且因此 a == a 可能为 false 的类型回退慢路径。
		// 这里用 type 指针比较，成本极低；普通对象仍保留原始 identity fast-path。
		return type == types::float_() || type == types::complex();
	}

}// namespace

RtValue RtValue::compare_eq(RtValue lhs, RtValue rhs)
{
	if (lhs.m_bits == rhs.m_bits) {
		if (lhs.is_tagged_int() || lhs.is_null()) { return from_ptr(py_true()); }

		PyObject *obj = lhs.as_ptr();
		if (!heap_object_may_break_reflexive_equality(obj)) { return from_ptr(py_true()); }

		return from_ptr(obj->eq(obj).unwrap());
	}
	if (RtValue::are_both_tagged_int(lhs, rhs)) { return from_ptr(py_false()); }
	return from_ptr(lhs.box()->eq(rhs.box()).unwrap());
}

RtValue RtValue::compare_ne(RtValue lhs, RtValue rhs)
{
	if (lhs.m_bits == rhs.m_bits) {
		if (lhs.is_tagged_int() || lhs.is_null()) { return from_ptr(py_false()); }

		PyObject *obj = lhs.as_ptr();
		if (!heap_object_may_break_reflexive_equality(obj)) { return from_ptr(py_false()); }

		return from_ptr(obj->ne(obj).unwrap());
	}
	if (RtValue::are_both_tagged_int(lhs, rhs)) { return from_ptr(py_true()); }
	return from_ptr(lhs.box()->ne(rhs.box()).unwrap());
}

RtValue RtValue::compare_lt(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		return from_ptr(lhs.as_int() < rhs.as_int() ? py_true() : py_false());
	}
	return from_ptr(lhs.box()->lt(rhs.box()).unwrap());
}

RtValue RtValue::compare_le(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		return from_ptr(lhs.as_int() <= rhs.as_int() ? py_true() : py_false());
	}
	return from_ptr(lhs.box()->le(rhs.box()).unwrap());
}

RtValue RtValue::compare_gt(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		return from_ptr(lhs.as_int() > rhs.as_int() ? py_true() : py_false());
	}
	return from_ptr(lhs.box()->gt(rhs.box()).unwrap());
}

RtValue RtValue::compare_ge(RtValue lhs, RtValue rhs)
{
	if (RtValue::are_both_tagged_int(lhs, rhs)) {
		return from_ptr(lhs.as_int() >= rhs.as_int() ? py_true() : py_false());
	}
	return from_ptr(lhs.box()->ge(rhs.box()).unwrap());
}

bool RtValue::is_truthy() const
{
	if (is_null()) { return false; }
	if (is_tagged_int()) { return as_int() != 0; }
	return box()->true_().unwrap();
}

// =============================================================================
// 极速方法缓存实现
// =============================================================================

struct MethodCacheEntry
{
	PyObject *instance;
	const char *name;
	PyObject *bound_method;
};

// 128容量的碰撞哈希，足以拦截 Sieve 中所有的函数反复提取 (step1 / 2 / 3)
// 函数内 static：避免全局符号 + 避免 LLVM DCE
static inline MethodCacheEntry *get_method_cache()
{
	static MethodCacheEntry cache[128] = {};// 正确 zero-init
	return cache;
}

PyObject *MethodCache::load_method(PyObject *obj, const char *name)
{
	auto *cache = get_method_cache();

	// hash
	size_t hash =
		(reinterpret_cast<uintptr_t>(obj) >> 4) ^ (reinterpret_cast<uintptr_t>(name) >> 4);
	size_t idx = hash & 127;

	auto &entry = cache[idx];

	// fast path
	if (entry.instance == obj && entry.name == name) { return entry.bound_method; }

	// miss
	auto method_res = obj->get_method(py::PyString::intern(name));
	if (method_res.is_err()) return nullptr;

	PyObject *m = method_res.unwrap();

	// update
	entry.instance = obj;
	entry.name = name;
	entry.bound_method = m;

	return m;
}

}// namespace py