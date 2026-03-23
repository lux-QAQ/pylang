#pragma once

#include "PyObject.hpp"

namespace py {
class PyGenericAlias : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend ::Heap;
#endif
	friend class ::py::Arena;

	PyObject *m_origin{ nullptr };
	PyTuple *m_args{ nullptr };
	PyObject *m_parameters{ nullptr };

	PyGenericAlias(PyType *type);

	PyGenericAlias(PyObject *origin, PyTuple *args, PyObject *parameters);

  public:
	static PyResult<PyGenericAlias *>
		create(PyObject *origin, PyObject *args, PyObject *parameters);

	static PyResult<PyGenericAlias *> create(PyObject *origin, PyObject *args);

	PyResult<PyObject *> __repr__() const;
	std::string to_string() const override;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
	void visit_graph(Visitor &visitor) override;
};
}// namespace py
