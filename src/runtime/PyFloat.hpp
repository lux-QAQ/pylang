#pragma once

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
	static PyResult<PyFloat *> create(PyType *type, double);
	// PyType *static_type() const override;;

	PyResult<int64_t> __hash__() const;

	double as_f64() const;

	PyResult<PyObject *> __round__(PyObject *ndigits) const;
	PyResult<PyObject *> __floor__() const;
	PyResult<PyObject *> __ceil__() const;
	PyResult<PyObject *> __trunc__() const;
	PyResult<PyObject *> is_integer() const;
	PyResult<PyObject *> hex() const;
	PyResult<PyObject *> as_integer_ratio() const;

	static PyResult<PyObject *> fromhex(PyType *type, PyTuple *args, PyDict *kwargs);
	static int64_t hash_f64(double value);

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
};

}// namespace py
