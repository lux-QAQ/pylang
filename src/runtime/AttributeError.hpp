#pragma once

#include "Exception.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"


#include "runtime/compat.hpp"

namespace py {
class AttributeError : public Exception
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	template<typename... Args>
	friend BaseException *attribute_error(const std::string &message, Args &&...args);

  private:
	AttributeError(PyType *type);

	AttributeError(PyTuple *args);

	static AttributeError *create(PyTuple *args) { return PYLANG_ALLOC(AttributeError, args); }

  public:
	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();

	PyType *static_type() const override;

	static PyType *class_type();
};


// template<typename... Args>
// inline BaseException *attribute_error(const std::string &message, Args &&...args)
// {
// 	auto msg = PyString::create(fmt::format(message, std::forward<Args>(args)...));
// 	ASSERT(msg.is_ok());
// 	auto args_tuple = PyTuple::create(msg.unwrap());
// 	ASSERT(args_tuple.is_ok());
// 	return AttributeError::create(args_tuple.unwrap());
// }


template<typename... Args>
inline BaseException *attribute_error(const std::string &message, Args &&...args)
{
    // 只有真正报错时才格式化字符串
    std::string formatted = fmt::format(message, std::forward<Args>(args)...);
    auto msg_res = PyString::create(std::move(formatted));
	ASSERT(msg_res.is_ok());
    auto args_tuple = PyTuple::create(msg_res.unwrap());
    return AttributeError::create(args_tuple.unwrap());
}
}// namespace py
