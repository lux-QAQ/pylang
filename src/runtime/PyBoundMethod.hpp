#pragma once

#include "PyObject.hpp"

namespace py {

class PyBoundMethod : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	PyObject *m_self;
	PyObject *m_method;

	PyBoundMethod(PyObject *self, PyObject *method);

	PyBoundMethod(PyType *);

  public:
	static PyResult<PyBoundMethod *> create(PyObject *self, PyObject *method);

	PyObject *self() { return m_self; }
	PyObject *method() { return m_method; }

	std::string to_string() const override;

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __call__(PyTuple *args, PyDict *kwargs);
	PyResult<PyObject *> call_raw(std::span<const Value> args, PyDict *kwargs) override;

	void visit_graph(Visitor &visitor) override;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	PyType *static_type() const override;
};

}// namespace py
