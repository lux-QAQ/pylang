#pragma once

#include <memory>
#include <type_traits>
#include <utility>

// =============================================================================
// Pylang RAII Memory Management
// =============================================================================
//
// 本文件定义了基于 shared_ptr 的内存管理基础设施，用于逐步替换 Heap+GC。
//
// 设计原则:
//   1. PyObjPtr 是所有 PyObject 的统一持有类型
//   2. make_py<T>(...) 是唯一的对象创建入口
//   3. py_cast<T>(ptr) 提供安全的向下转型
//   4. 与旧 Heap 系统通过 compat.hpp 桥接，两套可以共存
//
// =============================================================================

namespace py {

class PyObject;

// ---- 核心类型别名 ----

// 所有 PyObject 的统一持有类型
using PyObjPtr = std::shared_ptr<PyObject>;

// 弱引用（打破循环引用）
using PyWeakObjPtr = std::weak_ptr<PyObject>;

// 具体类型的智能指针
template<typename T> using PyPtr = std::shared_ptr<T>;

template<typename T> using PyWeakPtr = std::weak_ptr<T>;


// ---- 对象创建 ----

// make_py<T>(args...) : 创建一个 T 类型的 PyObject，返回 PyPtr<T>
//
// 用法:
//   auto integer = make_py<PyInteger>(42);
//   auto str     = make_py<PyString>("hello");
//
// 内部使用 make_shared 以获得单次分配优化（对象和控制块合并）。
//
template<typename T, typename... Args>
	requires std::is_base_of_v<PyObject, T>
PyPtr<T> make_py(Args &&...args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}


// ---- 安全向下转型 ----

// py_cast<T>(shared_ptr<PyObject>) -> shared_ptr<T>
//
// 使用 dynamic_pointer_cast 进行安全转型。
// 如果类型不匹配，返回 nullptr（空 shared_ptr）。
//
// 用法:
//   PyObjPtr obj = make_py<PyInteger>(42);
//   auto integer = py_cast<PyInteger>(obj);  // OK
//   auto str     = py_cast<PyString>(obj);   // nullptr
//
template<typename T>
	requires std::is_base_of_v<PyObject, T>
PyPtr<T> py_cast(const PyObjPtr &obj)
{
	return std::dynamic_pointer_cast<T>(obj);
}

// 非 const 版本
template<typename T>
	requires std::is_base_of_v<PyObject, T>
PyPtr<T> py_cast(PyObjPtr &&obj)
{
	return std::dynamic_pointer_cast<T>(std::move(obj));
}

// 从裸指针 + shared_from_this 获取 shared_ptr
// 前提: T 必须继承 enable_shared_from_this<PyObject>
// 注意: 仅在对象已经被 shared_ptr 管理时才安全
template<typename T>
	requires std::is_base_of_v<PyObject, T>
PyPtr<T> py_shared(T *raw)
{
	if (!raw) return nullptr;
	// shared_from_this 返回 shared_ptr<PyObject>，再 static_pointer_cast
	return std::static_pointer_cast<T>(raw->shared_from_this());
}


// ---- 静态转型（已知类型安全时使用，比 dynamic_cast 快）----

template<typename T>
	requires std::is_base_of_v<PyObject, T>
PyPtr<T> py_static_cast(const PyObjPtr &obj)
{
	return std::static_pointer_cast<T>(obj);
}


// ---- 辅助：从 shared_ptr 提取裸指针（用于与旧代码交互）----
//
// 警告: 调用者必须确保 shared_ptr 的生命周期覆盖裸指针的使用期。
// 这是过渡期的桥接工具，最终应该消除所有裸指针用法。
//
template<typename T> T *py_raw(const PyPtr<T> &ptr) { return ptr.get(); }


// ---- no-op deleter：用于将栈上/静态对象包装为 shared_ptr ----
//
// 某些单例对象（如 None, True, False）不需要被释放。
// 用法:
//   auto none_ptr = PyObjPtr(py_none_raw(), py_prevent_destroy{});
//
struct py_prevent_destroy
{
	void operator()(PyObject *) const noexcept { /* intentionally empty */ }
};

// 将一个不需要释放的裸指针包装为 shared_ptr
template<typename T>
	requires std::is_base_of_v<PyObject, T>
PyPtr<T> py_immortal(T *raw)
{
	return PyPtr<T>(raw, py_prevent_destroy{});
}


// ---- PyResult 兼容辅助 ----
//
// 现有代码大量使用 PyResult<T*>，迁移期间需要在 PyResult<PyPtr<T>> 和
// PyResult<T*> 之间转换。这些辅助函数简化转换。
//

// 从 PyPtr<T> 提取裸指针构造 Ok(T*)
// 用于: 新代码创建对象后返回给旧接口
template<typename T> T *unwrap_raw(const PyPtr<T> &ptr) { return ptr.get(); }

}// namespace py
