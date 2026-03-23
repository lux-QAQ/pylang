#pragma once

#include "Exception.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"

#include "runtime/compat.hpp"
namespace py {

class StopIteration : public Exception
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	template<typename... Args> friend BaseException *stop_iteration(Args &&...args);

  private:
	StopIteration(PyType *type);

	StopIteration(PyTuple *args);

  public:
	static StopIteration *create(PyTuple *args)
	{
		auto *result = PYLANG_ALLOC(StopIteration, args);

		return result;
	}
	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();

	// PyType *static_type() const override;;

	static PyType *class_type();
};

// template<typename... Args> inline BaseException *stop_iteration(Args &&...args)
// {
// 	auto args_tuple = PyTuple::create(std::forward<Args>(args)...);
// 	if (args_tuple.is_err()) { TODO(); }
// 	return StopIteration::create(args_tuple.unwrap());
// }

BaseException *stop_iteration_empty();

template<typename... Args> inline BaseException *stop_iteration(Args &&...args)
{
	if constexpr (sizeof...(Args) == 0) {
		// [修复] 调用 cpp 中实现的单例获取函数
		return stop_iteration_empty();
	}
	auto args_tuple = PyTuple::create(std::forward<Args>(args)...);
	if (args_tuple.is_err()) { TODO(); }
	return StopIteration::create(args_tuple.unwrap());
}
}// namespace py
