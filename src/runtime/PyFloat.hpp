#include "PyNumber.hpp"

namespace py {

class PyFloat final : public PyNumber
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	PyFloat(double);

	PyFloat(PyType *);

  public:
	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);

	static PyResult<PyFloat *> create(double);
	// PyType *static_type() const override;;

	double as_f64() const;

	PyResult<PyObject *> __round__(PyObject *ndigits) const;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
};

}// namespace py
