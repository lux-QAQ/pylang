#pragma once

#include "Exception.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"
#include "vm/VM.hpp"
#include "runtime/compat.hpp"
namespace py {

class StopIteration : public Exception
{
	friend class ::Heap;
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

	PyType *static_type() const override;

	static PyType *class_type();
};

template<typename... Args> inline BaseException *stop_iteration(Args &&...args)
{
	auto args_tuple = PyTuple::create(std::forward<Args>(args)...);
	if (args_tuple.is_err()) { TODO(); }
	return StopIteration::create(args_tuple.unwrap());
}

}// namespace py
