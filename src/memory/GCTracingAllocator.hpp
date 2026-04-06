#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// 将所有条件包含的标准库提前到 namespace 外部！
#if __has_include(<optional>)
#include <optional>
#endif
#if __has_include(<variant>)
#include <variant>
#endif
#include <atomic>

#if __has_include(<expected>)
#include <expected>
#endif

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#define PYLANG_HAS_STD_EXPECTED 1
#else
#define PYLANG_HAS_STD_EXPECTED 0
#endif

// 针对 C++23 屏蔽第三方库 yalantinglibs/frozen 的弃用警告
#ifdef __clang__
#pragma clang system_header
#elif defined(__GNUC__)
#pragma GCC system_header
#endif

// 引入 ylt reflection，用于聚合体成员递归推导
#include "ylt/reflection/member_names.hpp"
#include "ylt/reflection/member_ptr.hpp"

#ifdef PYLANG_USE_Boehm_GC
#include <gc.h>
#endif

namespace py {

// 使得推导过程中能预见我们自己定制的分配器特性
template<typename T> struct GCTracingAllocator;

namespace gc {

	// ===================================================================================
	// 战线 1：Atomic 免扫描探测系统 (针对 GC_MALLOC_ATOMIC)
	// ===================================================================================
	template<typename T> struct is_atomic;

	namespace detail {
		template<typename Tuple> struct tuple_all_atomic;
		template<> struct tuple_all_atomic<std::tuple<>> : std::true_type
		{
		};
		template<typename... Ts>
		struct tuple_all_atomic<std::tuple<Ts...>>
			: std::bool_constant<(is_atomic<std::remove_cvref_t<Ts>>::value && ...)>
		{
		};

		// [新增]: 安全探测 std::pair 的助手，避免直接访问 T::first_type
		template<typename T> struct is_pair_helper : std::false_type
		{
		};
		template<typename U, typename V> struct is_pair_helper<std::pair<U, V>> : std::true_type
		{
		};

		// 探测并拦截标准库内部节点
		template<typename T> struct is_std_internal : std::false_type
		{
		};

		// 针对 libstdc++ 和 libc++ 的常见内部节点特征进行模糊匹配或直接拦截
		template<typename T>
			requires(
				requires { typename T::__node_type; } || requires { typename T::value_type; })
		struct is_std_internal<T>
			: std::bool_constant<std::is_scoped_enum_v<T> == false
								 && !is_pair_helper<T>::value>// 使用助手替代 std::is_same_v
		{
		};

		template<typename T,
			bool IsReflectable = (std::is_aggregate_v<T> || ylt::reflection::is_ylt_refl_v<T>)
								 && !std::is_array_v<T> && !std::is_union_v<T>
								 && !is_std_internal<T>::value>// 拦截标准库内部节点
		struct safe_members_count : std::integral_constant<std::size_t, 0>
		{
		};

		template<typename T>
		struct safe_members_count<T, true>
			: std::integral_constant<std::size_t, ylt::reflection::members_count_v<T>>
		{
		};

		template<typename T, typename = void> struct composite_is_atomic : std::false_type
		{
		};

		template<typename T>
		struct composite_is_atomic<T, std::enable_if_t<(safe_members_count<T>::value > 0)>>
		{
			using members_tuple = decltype(ylt::reflection::object_to_tuple(std::declval<T &>()));
			static constexpr bool value = tuple_all_atomic<members_tuple>::value;
		};

		template<typename T>
		struct composite_is_atomic<T,
			std::enable_if_t<((std::is_aggregate_v<T> || ylt::reflection::is_ylt_refl_v<T>)
								 && !std::is_array_v<T> && !std::is_union_v<T>)
							 && (safe_members_count<T>::value == 0)>> : std::true_type
		{
		};
	}// namespace detail

	// 主干：算术值或反射判定全免扫描的聚合体
	template<typename T, typename = void> struct is_atomic_impl
	{
		static constexpr bool value =
			(std::is_arithmetic_v<T> || std::is_enum_v<T>) ? true
			: ((std::is_aggregate_v<T> || ylt::reflection::is_ylt_refl_v<T>)
				  && !std::is_array_v<T> && !std::is_union_v<T>)
				? detail::composite_is_atomic<T>::value
				: false;
	};

	// 【各大大标准底层容器特化】
	template<typename T, std::size_t N> struct is_atomic_impl<std::array<T, N>> : is_atomic<T>
	{
	};
	template<typename T1, typename T2>
	struct is_atomic_impl<std::pair<T1, T2>>
		: std::bool_constant<is_atomic<T1>::value && is_atomic<T2>::value>
	{
	};
	template<typename... Ts>
	struct is_atomic_impl<std::tuple<Ts...>> : std::bool_constant<(is_atomic<Ts>::value && ...)>
	{
	};
	template<typename T, std::size_t N> struct is_atomic_impl<T[N]> : is_atomic<T>
	{
	};
	template<typename T> struct is_atomic_impl<T[]> : is_atomic<T>
	{
	};
#if __has_include(<optional>)
	template<typename T> struct is_atomic_impl<std::optional<T>> : is_atomic<T>
	{
	};
#endif
#if __has_include(<variant>)
	template<typename... Ts>
	struct is_atomic_impl<std::variant<Ts...>> : std::bool_constant<(is_atomic<Ts>::value && ...)>
	{
	};
#endif
#if PYLANG_HAS_STD_EXPECTED
	template<typename T, typename E>
	struct is_atomic_impl<std::expected<T, E>>
		: std::bool_constant<is_atomic<T>::value && is_atomic<E>::value>
	{
	};
#endif
	template<typename T> struct is_atomic_impl<std::atomic<T>> : is_atomic<T>
	{
	};

	// 输出终端口
	template<typename T> struct is_atomic : is_atomic_impl<std::remove_cv_t<T>>
	{
	};
	template<typename T> constexpr bool is_atomic_v = is_atomic<T>::value;


	// ===================================================================================
	// 战线 2：Trivially Destructible 零终结器探测系统 (挂载免除)
	// ===================================================================================
	template<typename T> struct is_trivial_dtor;

	// 主干：默认依赖 C++ 本身的标准推导
	template<typename T, typename = void>
	struct is_trivial_dtor_impl : std::is_trivially_destructible<T>
	{
	};

	// 【绝杀 1】：基于追踪分配器的 GCVector 一旦 T 是豁免的，那它本身自动全面豁免！
	// 因为它所处的 buffer 块属于 GC，我们已关闭了手动释放的 no-op 等待。
	template<typename T>
	struct is_trivial_dtor_impl<std::vector<T, GCTracingAllocator<T>>> : is_trivial_dtor<T>
	{
	};

	// 其它基础类型的联动豁免链
	template<typename T, std::size_t N>
	struct is_trivial_dtor_impl<std::array<T, N>> : is_trivial_dtor<T>
	{
	};
	template<typename T1, typename T2>
	struct is_trivial_dtor_impl<std::pair<T1, T2>>
		: std::bool_constant<is_trivial_dtor<T1>::value && is_trivial_dtor<T2>::value>
	{
	};
	template<typename... Ts>
	struct is_trivial_dtor_impl<std::tuple<Ts...>>
		: std::bool_constant<(is_trivial_dtor<Ts>::value && ...)>
	{
	};

#if __has_include(<optional>)
	template<typename T> struct is_trivial_dtor_impl<std::optional<T>> : is_trivial_dtor<T>
	{
	};
#endif
#if __has_include(<variant>)
	template<typename... Ts>
	struct is_trivial_dtor_impl<std::variant<Ts...>>
		: std::bool_constant<(is_trivial_dtor<Ts>::value && ...)>
	{
	};
#endif
#if PYLANG_HAS_STD_EXPECTED
	template<typename T, typename E>
	struct is_trivial_dtor_impl<std::expected<T, E>>
		: std::bool_constant<is_trivial_dtor<T>::value && is_trivial_dtor<E>::value>
	{
	};
#endif

	// 输出终端口
	template<typename T> struct is_trivial_dtor : is_trivial_dtor_impl<std::remove_cv_t<T>>
	{
	};
	template<typename T> constexpr bool is_trivial_dtor_v = is_trivial_dtor<T>::value;

}// namespace gc

// ===================================================================================
// 提供专门为人为打点设计的强制免扫与免析构宏接口
// （用在像特权内置实现 PyInteger 里直接跳过标记的情况）
// ===================================================================================
#define PYLANG_GC_FORCE_TRIVIAL(Type)                            \
	namespace py::gc {                                           \
		template<> struct is_trivial_dtor<Type> : std::true_type \
		{                                                        \
		};                                                       \
	}

#define PYLANG_GC_FORCE_ATOMIC(Type)                       \
	namespace py::gc {                                     \
		template<> struct is_atomic<Type> : std::true_type \
		{                                                  \
		};                                                 \
	}

#define PYLANG_GC_FORCE_NON_ATOMIC(Type)                    \
	namespace py::gc {                                      \
		template<> struct is_atomic<Type> : std::false_type \
		{                                                   \
		};                                                  \
	}

// ===================================================================================
// Allocator 实现
// ===================================================================================

#ifdef PYLANG_USE_Boehm_GC
template<typename T> struct GCTracingAllocator
{
	using value_type = T;

	GCTracingAllocator() noexcept = default;
	template<typename U> GCTracingAllocator(const GCTracingAllocator<U> &) noexcept {}

	T *allocate(std::size_t n)
	{
		void *p = nullptr;
		if constexpr (py::gc::is_atomic_v<T>) {
			p = GC_MALLOC_ATOMIC(n * sizeof(T));
		} else {
			p = GC_MALLOC(n * sizeof(T));
		}
		if (!p) throw std::bad_alloc();
		return static_cast<T *>(p);
	}

	// [核心修复]: 当容器（如 vector 或 ordered_map 桶）扩容释放老内存时，
	//                 立刻通知 GC_FREE 回收，而不是等 GC 去做全盘清扫！彻底消灭扩容垃圾！
	void deallocate(T *p, std::size_t) noexcept
	{
		if (p) { GC_FREE(static_cast<void *>(p)); }
	}

	template<typename U> bool operator==(const GCTracingAllocator<U> &) const noexcept
	{
		return true;
	}
	template<typename U> bool operator!=(const GCTracingAllocator<U> &) const noexcept
	{
		return false;
	}
};
#else
template<typename T> using GCTracingAllocator = std::allocator<T>;
#endif

template<typename T> using GCVector = std::vector<T, GCTracingAllocator<T>>;

}// namespace py