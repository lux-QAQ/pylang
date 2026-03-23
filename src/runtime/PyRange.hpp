#pragma once

#include "PyObject.hpp"
#include <optional>


namespace py {

class PyRange : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	const BigIntType m_start{ 0 };
	const BigIntType m_stop;
	const BigIntType m_step{ 1 };

	PyRange(PyType *);

  public:
	std::string to_string() const override;

	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);
	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __iter__() const;
	PyResult<PyObject *> __getitem__(int64_t index) const;
	PyResult<PyObject *> __getitem__(PyObject *key) const;

	const BigIntType &start() const { return m_start; }
	const BigIntType &stop() const { return m_stop; }
	const BigIntType &step() const { return m_step; }

	PyResult<PyObject *> __reversed__() const;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;

  private:
	PyRange(BigIntType start, BigIntType stop, BigIntType step);
	PyRange(PyInteger *stop);
	PyRange(PyInteger *start, PyInteger *stop);
	PyRange(PyInteger *start, PyInteger *stop, PyInteger *step);
};


class PyRangeIterator : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	const PyRange &m_pyrange;
	BigIntType m_current_index;

  public:
	PyRangeIterator(const PyRange &);

	std::string to_string() const override;
	std::optional<int64_t> next_fast();

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __next__();
	PyResult<PyObject *> __iter__() const;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();

	void visit_graph(Visitor &) override;

	// PyType *static_type() const override;;
};

}// namespace py
