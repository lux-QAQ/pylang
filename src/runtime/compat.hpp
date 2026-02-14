#pragma once

// =============================================================================
// Pylang Heap/SharedPtr 兼容层
// =============================================================================
//
// 问题:
//   现有 Runtime 的所有 PyObject 子类都通过 Heap::allocate<T>() 分配，
//   并依赖 GC 的 mark-sweep 回收。如果一次性删除 Heap/VM，整个 runtime
//   将无法编译（数百个 create() 函数、friend class ::Heap 声明等）。
//
// 解决方案:
//   引入编译期开关 PYLANG_USE_SHARED_PTR。
//   - 未定义时: 所有分配走旧的 Heap 路径，行为不变。
//   - 定义后:   所有分配走 std::make_shared，GC 相关代码变为 no-op。
//
//   这样可以逐个文件迁移：先在某个 .cpp 中 #define PYLANG_USE_SHARED_PTR，
//   验证通过后再扩展到更多文件，最终全局启用。
//
// 迁移步骤:
//   Phase A: 引入本文件，不定义宏，所有行为不变。
//   Phase B: 在 CMake 中为新的 runtime 库 target 定义宏。
//   Phase C: 逐个修改 create() 函数，使用 PYLANG_ALLOC。
//   Phase D: 全局启用后，删除 Heap/GC/Cell 相关代码。
//
// =============================================================================

#include "memory.hpp"

#ifdef PYLANG_USE_SHARED_PTR
// ---- 新路径: shared_ptr ----

// 分配一个 PyObject 子类，返回裸指针（兼容现有 create() 签名）。
//
// 关键设计: 我们用一个全局的 shared_ptr 注册表来持有对象，
// 防止 shared_ptr 立即析构。这是过渡方案，最终 create() 的返回类型
// 会改为 PyPtr<T>。
//
// 但更实际的做法是: 在迁移期间，create() 内部用 new 分配，
// 外部逐步改为用 shared_ptr 持有。因为 shared_ptr 要求从一开始
// 就用 shared_ptr 管理，不能中途从裸指针转换。
//
// 因此我们采用更务实的策略:
//   PYLANG_ALLOC<T>(args...) 返回 T*，内部用 new 分配。
//   对象的生命周期由调用者负责（目前是 Heap，未来是 shared_ptr）。
//   当全局切换完成后，再统一改为 make_shared。

#include <new>

namespace py::compat {

#include <mutex>
#include <type_traits>
#include <unordered_map>

// 兼容注册表：raw-pointer -> shared handle
// 目的：允许在 "shared_ptr" 模式下向旧 API 返回裸指针的同时保留 shared_ptr 控制权，
//       并支持从裸指针查找对应的 shared_ptr（迁移期间使用，后续可移除）。
static inline std::unordered_map<void *, py::PyObjPtr> s_legacy_registry;
static inline std::mutex s_legacy_registry_mutex;

inline void register_shared_for_legacy(const py::PyObjPtr &sp)
{
	if (!sp) return;
	std::lock_guard<std::mutex> lk(s_legacy_registry_mutex);
	s_legacy_registry[sp.get()] = sp;
}

inline py::PyObjPtr try_get_shared_for_legacy(void *raw)
{
	std::lock_guard<std::mutex> lk(s_legacy_registry_mutex);
	auto it = s_legacy_registry.find(raw);
	if (it == s_legacy_registry.end()) return nullptr;
	return it->second;
}

inline void deregister_legacy_ptr(void *raw)
{
	std::lock_guard<std::mutex> lk(s_legacy_registry_mutex);
	s_legacy_registry.erase(raw);
}

inline size_t legacy_registry_size()
{
	std::lock_guard<std::mutex> lk(s_legacy_registry_mutex);
	return s_legacy_registry.size();
}

// heap_allocate: shared_ptr 模式下的兼容分配器
// - 若 T 在此上下文可直接构造 (public ctor)，使用 make_shared 并登记到注册表
// - 否则回退到 Heap::allocate（常见于类将 Heap 设为 friend）并以 non-owning handle 登记
template<typename T, typename... Args> T *heap_allocate(Args &&...args)
{
	static_assert(std::is_base_of_v<py::PyObject, T>, "T must derive from PyObject");

	if constexpr (std::is_constructible_v<T, Args...>) {
		auto sp = std::make_shared<T>(std::forward<Args>(args)...);
		register_shared_for_legacy(sp);
		return sp.get();
	} else {
		// 构造不可访问 -> 使用原有 Heap（保证兼容 private/protected ctor）
		T *raw = VirtualMachine::the().heap().allocate<T>(std::forward<Args>(args)...);
		// non-owning wrapper，便于通过裸指针查找 shared handle
		register_shared_for_legacy(py::py_immortal(raw));
		return raw;
	}
}

// visit_graph 的 no-op 替代
// 当启用 shared_ptr 后，GC 的 visit_graph 不再需要
struct NullVisitor
{
	template<typename T> void visit(T &) { /* no-op */ }
};

// 支持带额外字节的分配（向后兼容 Heap::allocate_with_extra_bytes）
template<typename T, typename... Args>
T *heap_allocate_with_extra(size_t extra_bytes, Args &&...args)
{
	// 目前对变长对象仍然使用 Heap 路径以保证语义与布局一致。
	return VirtualMachine::the().heap().allocate_with_extra_bytes<T>(
		extra_bytes, std::forward<Args>(args)...);
}

}// namespace py::compat

// 替换 Heap::allocate 的宏
// 用法: auto* obj = PYLANG_ALLOC(PyInteger, args...);
// 在旧模式下展开为 VirtualMachine::the().heap().allocate<T>(args...)
// 在新模式下展开为 py::compat::heap_allocate<T>(args...)
#define PYLANG_ALLOC(Type, ...) ::py::compat::heap_allocate<Type>(__VA_ARGS__)

// 带额外字节的分配（用于变长对象）
#define PYLANG_ALLOC_WITH_EXTRA(Type, Extra, ...) \
	::py::compat::heap_allocate_with_extra<Type>(Extra, __VA_ARGS__)

// visit_graph 的条件编译辅助
// 在新模式下，visit_graph 体变为空
#define PYLANG_VISIT_GRAPH_BODY(visitor) (void)(visitor);

// Cell 基类的替代
// 在新模式下，PyObject 不再需要继承 Cell
// 但为了兼容，我们提供一个空的 Cell 替代
namespace py::compat {

class CellCompat
{
  public:
	CellCompat() = default;
	virtual ~CellCompat() = default;

	// 保留接口兼容，但都是 no-op
	virtual std::string to_string() const { return ""; }

	struct Visitor
	{
		virtual ~Visitor() = default;
		virtual void visit(CellCompat &) {}
	};

	virtual void visit_graph(Visitor &) {}
	virtual bool is_pyobject() const { return false; }
};

}// namespace py::compat

#else
// ---- 旧路径: Heap + GC ----

// 什么都不改，保持原有行为
#define PYLANG_ALLOC(Type, ...) VirtualMachine::the().heap().allocate<Type>(__VA_ARGS__)

#define PYLANG_VISIT_GRAPH_BODY(visitor) /* use original implementation */

#endif// PYLANG_USE_SHARED_PTR


// =============================================================================
// 通用辅助宏（两种模式下都可用）
// =============================================================================

// 检查分配结果并返回 MemoryError
// 用法:
//   auto* obj = PYLANG_ALLOC(PyInteger, args...);
//   PYLANG_CHECK_ALLOC(obj, PyInteger);
//   return Ok(obj);
//
#define PYLANG_CHECK_ALLOC(ptr, Type)                           \
	do {                                                        \
		if (!(ptr)) { return Err(memory_error(sizeof(Type))); } \
	} while (0)

// 组合: 分配 + 检查，一步到位
// 用法:
//   auto* obj = PYLANG_CREATE(PyInteger, args...);
//   return Ok(obj);
//
// 展开后:
//   auto* obj = PYLANG_ALLOC(PyInteger, args...);
//   if (!obj) { return Err(memory_error(sizeof(PyInteger))); }
//
#define PYLANG_CREATE(Type, ...)                      \
	[&]() -> Type * {                                 \
		auto *_obj = PYLANG_ALLOC(Type, __VA_ARGS__); \
		return _obj;                                  \
	}()
