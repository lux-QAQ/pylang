

# GC 标记开销分析与优化方案

看了你的代码，问题根源很清晰：每个 `PyInteger` 内部持有 `mpz_class`，而 `mpz_class` 通过 `GC_MALLOC` 分配的 buffer 对 GC 来说是不透明的指针链。GC 必须保守扫描每一块内存来找到这些引用，导致 `GC_mark_from` 爆炸。

## 问题 1：减少 GC 标记开销

### 核心问题：GMP 内存不应该被 GC 追踪

你在 rt_lifecycle.cpp 中把 GMP 的分配器设成了 `GC_MALLOC_ATOMIC`，这是对的方向，但 `GC_MALLOC_ATOMIC` 分配的内存虽然不会被扫描内部指针，GC 仍然需要标记它为存活。真正的问题是：

1. 大量小整数运算产生海量临时 `PyInteger`，每个都是 GC 对象
2. `GC_MALLOC` 分配的 `PyInteger` 本身需要被扫描（因为内部有 `m_type` 等指针）
3. GMP 的 `mpz_class` 内部 buffer 虽然用了 `GC_MALLOC_ATOMIC`，但 `mpz_class` 对象本身嵌在 `PyInteger` 里，GC 扫描 `PyInteger` 时要遍历整个对象体

### 方案 A：小整数缓存（立竿见影）

Python 的 CPython 缓存 [-5, 256] 范围的整数。你的 runtime 没做这个优化，意味着 `for i in range(1000000)` 会创建 100 万个 `PyInteger`。

````cpp
// ...existing code...
class PyInteger : public Interface<PyNumber, PyInteger>
{
	// ...existing code...

  public:
	static PyResult<PyInteger *> create(int64_t);
	static PyResult<PyInteger *> create(BigIntType);

	// 新增：小整数缓存
	static constexpr int64_t kSmallIntMin = -5;
	static constexpr int64_t kSmallIntMax = 256;
	static void init_small_int_cache();

  private:
	static PyInteger *s_small_int_cache[256 - (-5) + 1];

	// ...existing code...
};
````

````cpp
// ...existing code...

PyInteger *PyInteger::s_small_int_cache[256 - (-5) + 1] = {};

void PyInteger::init_small_int_cache()
{
	for (int64_t i = kSmallIntMin; i <= kSmallIntMax; ++i) {
		auto *obj = PYLANG_ALLOC(PyInteger, BigIntType{i});
		s_small_int_cache[i - kSmallIntMin] = obj;
	}
}

PyResult<PyInteger *> PyInteger::create(int64_t value)
{
	if (value >= kSmallIntMin && value <= kSmallIntMax) {
		return Ok(s_small_int_cache[value - kSmallIntMin]);
	}
	return PyInteger::create(BigIntType{ value });
}

PyResult<PyInteger *> PyInteger::create(BigIntType value)
{
	if (value.fits_slong_p()) {
		auto v = value.get_si();
		if (v >= kSmallIntMin && v <= kSmallIntMax) {
			return Ok(s_small_int_cache[v - kSmallIntMin]);
		}
	}
	auto *result = PYLANG_ALLOC(PyInteger, std::move(value));
	PYLANG_CHECK_ALLOC(result, PyInteger);
	return Ok(result);
}

// ...existing code...
````

在 `rt_init` 中调用：

````cpp
// ...existing code...
PYLANG_EXPORT_LIFECYCLE("init", "void", "")
void rt_init()
{
#ifdef PYLANG_USE_Boehm_GC
    GC_INIT();
    GC_expand_hp(256 * 1024 * 1024);
    GC_set_free_space_divisor(100);
    GC_enable_incremental();
    GC_allow_register_threads();
    GC_set_warn_proc(pylang_gc_warn_proc);
    GC_set_finalize_on_demand(0);
    mp_set_memory_functions(pylang_gmp_alloc, pylang_gmp_realloc, pylang_gmp_free);
#endif

#ifdef PYLANG_USE_ARENA
    py::ArenaManager::initialize();
#endif

    if (!py::RuntimeContext::has_current()) {
        static thread_local py::RuntimeContext s_compiler_ctx;
        py::RuntimeContext::set_current(&s_compiler_ctx);
    }

    py::initialize_types();
    py::PyInteger::init_small_int_cache();
    py::register_all_builtins();
}
// ...existing code...
````

### 方案 B：GMP 内存脱离 GC 管理

GMP 的 buffer 是纯数据（不含指针），让 GC 追踪它们毫无意义。改用 `malloc/free` 而非 `GC_MALLOC_ATOMIC`：

````cpp
// ...existing code...
#ifdef PYLANG_USE_Boehm_GC
// ...existing code...
extern "C" {
static void *pylang_gmp_alloc(size_t alloc_size) { return std::malloc(alloc_size); }
static void *pylang_gmp_realloc(void *ptr, [[maybe_unused]] size_t old_size, size_t new_size)
{
	return std::realloc(ptr, new_size);
}
static void pylang_gmp_free(void *ptr, [[maybe_unused]] size_t size) { std::free(ptr); }
}
#endif
// ...existing code...
````

这样做的前提是 `PyInteger` 的 finalizer 必须正常运行（调用 `~mpz_class()` 来 `free` buffer）。你当前 `gc_needs_finalizer` 返回 `false`（第 42 行 `if constexpr (1) { return false; }`），这会导致 GMP buffer 泄漏。修复：

````cpp
// ...existing code...
template<typename T> constexpr bool gc_needs_finalizer()
{
#ifdef PYLANG_USE_Boehm_GC
	// GMP buffer 用 malloc 分配，必须通过析构函数释放
	// 只有真正 trivially destructible 的类型才能跳过 finalizer
	return !std::is_trivially_destructible_v<T>;
#else
	return !std::is_trivially_destructible_v<T>;
#endif
}
// ...existing code...
````

但这会重新引入 finalizer 开销。所以需要配合方案 C。

### 方案 C：对 PyInteger 使用 `GC_MALLOC_ATOMIC` + 手动类型指针

`PyInteger` 的内存布局中，唯一需要 GC 追踪的指针是 `m_type`。如果把 `m_type` 改为全局查表（整数类型是固定的），整个 `PyInteger` 就不含 GC 指针，可以用 `GC_MALLOC_ATOMIC` 分配——GC 完全不扫描它的内部。

这个改动侵入性较大，但效果最好。简化版：在 Arena 中为"叶子对象"提供 atomic 分配路径：

````cpp
// ...existing code...
	/// 分配叶子对象（内部不含 GC 指针），GC 不扫描其内部
	template<typename T, typename... Args> T *allocate_atomic(Args &&...args)
	{
#ifdef PYLANG_USE_Boehm_GC
		void *mem = GC_MALLOC_ATOMIC(sizeof(T));
		if (!mem) return nullptr;
		T *obj = new (mem) T(std::forward<Args>(args)...);
		if constexpr (!std::is_trivially_destructible_v<T>) {
			GC_register_finalizer_no_order(mem, gc_finalizer_proxy<T>, nullptr, nullptr, nullptr);
		}
		return obj;
#else
		return allocate<T>(std::forward<Args>(args)...);
#endif
	}
// ...existing code...
````

然后为 `PyInteger` 和 `PyFloat` 使用 atomic 分配。这需要一个新的 `PYLANG_ALLOC_ATOMIC` 宏，以及确保这些类型的 `m_type` 不依赖 GC 追踪（`types::integer()` 是全局 immortal 对象，GC root 已经持有它）。

---


### 方案 E：Tagged Pointer / Unboxed Integer（终极方案）

这是 V8、LuaJIT、CPython 3.12+ 都在用的技术。核心思想：小整数不分配堆内存，直接编码在指针里。

````cpp
#pragma once
#include <cstdint>

namespace py {

// 利用指针最低位做标记：
//   bit 0 = 0 → 真实 PyObject* 指针
//   bit 0 = 1 → 立即数整数，高 63 位存值
class TaggedValue
{
	uintptr_t m_bits;

  public:
	static constexpr int kTagBits = 1;
	static constexpr uintptr_t kIntTag = 1;
	static constexpr int64_t kIntMax = (1LL << 62) - 1;
	static constexpr int64_t kIntMin = -(1LL << 62);

	// 从 PyObject* 构造
	explicit TaggedValue(PyObject *obj) : m_bits(reinterpret_cast<uintptr_t>(obj)) {}

	// 从立即数整数构造
	static TaggedValue from_int(int64_t v)
	{
		TaggedValue tv;
		tv.m_bits = (static_cast<uintptr_t>(v) << kTagBits) | kIntTag;
		return tv;
	}

	bool is_int() const { return (m_bits & kIntTag) != 0; }
	bool is_ptr() const { return (m_bits & kIntTag) == 0; }

	int64_t as_int() const { return static_cast<int64_t>(m_bits) >> kTagBits; }
	PyObject *as_ptr() const { return reinterpret_cast<PyObject *>(m_bits); }

  private:
	TaggedValue() = default;
};

}// namespace py
````

这个方案改动最大，但效果也最好——63 位范围内的整数运算零分配。需要修改编译器生成的 IR，在算术运算前检查 tag，走快速路径。

---

## 推荐实施顺序

| 优先级 | 方案 | 预期效果 | 改动量 |
|--------|------|----------|--------|
| 1 | A: 小整数缓存 | 减少 50-80% 的 PyInteger 分配 | 小 |
| 2 | B: GMP 用 malloc | 减少 GC 追踪对象数 30%+ | 小 |
| 2 | 修复 `gc_needs_finalizer` | 防止 GMP buffer 泄漏 | 一行 |
| 3 | C: atomic 分配 | GC 标记时间减少 50%+ | 中 |
| 4 | D: Scoped Arena | Arena 模式下内存不再只增不减 | 中 |
| 5 | E: Tagged Pointer | 小整数零分配，彻底解决问题 | 大 |

方案 A + B + 修复 finalizer 三个一起做，改动量小，应该能把 `GC_mark_from` 的开销降到可接受范围。