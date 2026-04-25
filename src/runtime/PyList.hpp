#pragma once

#include "PyObject.hpp"
#include "memory/GCTracingAllocator.hpp"

namespace py {

class PyList
	: public PyBaseObject
	, PySequence
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	mutable py::GCVector<Value> m_elements;
	mutable size_t m_front_offset{ 0 };

	PyList(PyType *);

  public:
	static PyResult<PyList *> create(std::vector<Value> elements);
	static PyResult<PyList *> create(std::span<const Value> elements);
	static PyResult<PyList *> create();

	std::string to_string() const override;

	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __iter__() const;
	PyResult<PyObject *> __getitem__(PyObject *index);
	PyResult<std::monostate> __setitem__(PyObject *index, PyObject *value);
	PyResult<size_t> __len__() const;

	PyResult<PyObject *> __getitem__(int64_t index);
	PyResult<std::monostate> __setitem__(int64_t index, PyObject *value);
	PyResult<std::monostate> __delitem__(PyObject *key);

	PyResult<PyObject *> __add__(const PyObject *other) const;
	PyResult<PyObject *> __mul__(size_t count) const;
	PyResult<PyObject *> __eq__(const PyObject *other) const;

	size_t logical_size() const noexcept { return m_elements.size() - m_front_offset; }
	bool empty() const noexcept { return logical_size() == 0; }

	const Value &unchecked_at(size_t index) const { return m_elements[m_front_offset + index]; }
	Value &unchecked_at(size_t index) { return m_elements[m_front_offset + index]; }

	void normalize_storage() const;
	void push_front_raw(Value value);
	void append_raw(Value value);
	Value pop_back_raw();
	void insert_raw_clamped(int64_t index, Value value);

	const py::GCVector<Value> &elements() const
	{
		normalize_storage();
		return m_elements;
	}
	py::GCVector<Value> &elements()
	{
		normalize_storage();
		return m_elements;
	}

	void visit_graph(Visitor &) override;

	PyResult<PyObject *> append(PyObject *element);
	PyResult<PyObject *> extend(PyObject *iterable);
	PyResult<PyObject *> pop(PyObject *index);
	PyResult<PyObject *> insert(PyTuple *args, PyDict *kwargs);

	PyResult<PyObject *> __class_getitem__(PyType *cls, PyObject *args);
	PyResult<PyObject *> __reversed__() const;

	PyResult<PyObject *> sort(PyTuple *args, PyDict *kwargs);

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;

  private:
	PyList();
	PyList(std::vector<Value> elements);
};


class PyListIterator : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	const PyList &m_pylist;
	size_t m_current_index{ 0 };

  public:
	PyListIterator(const PyList &pylist);

	std::string to_string() const override;

	void visit_graph(Visitor &) override;

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __next__();

	// [性能优化] 零分配快速路径：直接返回 PyObject* 而不经过 Value::box()
	PyObject *next_raw();

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};

class PyListReverseIterator : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	std::optional<std::reference_wrapper<PyList>> m_pylist;
	size_t m_current_index{ 0 };

  private:
	PyListReverseIterator(PyType *);

	PyListReverseIterator(PyList &pylist, size_t start_index);

  public:
	static PyResult<PyListReverseIterator *> create(PyList &);

	void visit_graph(Visitor &) override;

	PyResult<PyObject *> __iter__() const;
	PyResult<PyObject *> __next__();

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};

}// namespace py
