#pragma once

#include "PyObject.hpp"

namespace py {

class NotImplemented : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	friend NotImplemented *not_implemented();

	NotImplemented();
	NotImplemented(PyType *type);

	static PyResult<NotImplemented *> create();

  public:
	std::string to_string() const override;

	void visit_graph(Visitor &) override {}

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};

NotImplemented *not_implemented();

}// namespace py
