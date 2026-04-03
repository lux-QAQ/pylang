#pragma once

#include "PyObject.hpp"
#include "memory/GCTracingAllocator.hpp"// 新增

namespace py {

class PyTuple
	: public PyBaseObject
	, PySequence
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	friend class PyTupleIterator;

	const py::GCVector<Value> m_elements;// 修改为 GCVector

  protected:
	PyTuple(PyType *);

	PyTuple();
	PyTuple(std::vector<Value> &&elements);
	PyTuple(PyType *type, std::vector<Value> elements);// 移除 const 以匹配 .cpp 第 68 行
	PyTuple(const std::vector<PyObject *> &elements);
	PyTuple(PyType *type, const std::vector<PyObject *> &elements);

	void visit_graph(Visitor &) override;

	// Type trait helpers to prevent vararg template greediness for containers
	template<typename T> struct is_container_arg : std::false_type
	{
	};
	template<typename... TArgs> struct is_container_arg<std::vector<TArgs...>> : std::true_type
	{
	};
	template<typename T_E, std::size_t E>
	struct is_container_arg<std::span<T_E, E>> : std::true_type
	{
	};

  public:
	static PyResult<PyTuple *> create();

	// [关键拦截器]：拦截非 const 的 GCVector 引用
	static PyResult<PyTuple *> create(py::GCVector<Value> &elements)
	{
		return PyTuple::create(const_cast<const py::GCVector<Value> &>(elements));
	}

	// [关键拦截器]：拦截 const 的 GCVector 引用
	static PyResult<PyTuple *> create(const py::GCVector<Value> &elements)
	{
		py::GCVector<Value> tmp(elements.begin(), elements.end());
		return PyTuple::create(std::move(tmp));
	}

	// [关键拦截器]：拦截非 const 的 std::vector 引用
	static PyResult<PyTuple *> create(std::vector<Value> &elements)
	{
		return PyTuple::create(const_cast<const std::vector<Value> &>(elements));
	}

	// [关键拦截器]：拦截 const 的 std::vector 引用
	static PyResult<PyTuple *> create(const std::vector<Value> &elements)
	{
		py::GCVector<Value> tmp(elements.begin(), elements.end());
		return PyTuple::create(std::move(tmp));
	}

	static PyResult<PyTuple *> create(std::vector<Value> &&elements);
	static PyResult<PyTuple *> create(PyType *type, std::vector<Value> elements);
	static PyResult<PyTuple *> create(std::vector<PyObject *> &&elements);
	static PyResult<PyTuple *> create(const std::vector<PyObject *> &elements);
	static PyResult<PyTuple *> create(PyType *type, const std::vector<PyObject *> &elements);


	static PyResult<PyTuple *> create(std::vector<PyObject *> &elements)
	{
		return PyTuple::create(const_cast<const std::vector<PyObject *> &>(elements));
	}

	// 接受 initializer_list，解决 PyTuple::create({a, b}) 的歧义
	static PyResult<PyTuple *> create(std::initializer_list<Value> il)
	{
		return PyTuple::create(py::GCVector<Value>(il.begin(), il.end()));
	}

	// 零拷贝核心接口
	static PyResult<PyTuple *> create(py::GCVector<Value> &&elements);
	PyTuple(py::GCVector<Value> &&elements);
	PyTuple(PyType *type, py::GCVector<Value> &&elements);// 新增：匹配 .cpp 第 93 行

	static PyResult<PyTuple *> create(std::span<Value> elements)
	{
		return PyTuple::create(py::GCVector<Value>(elements.begin(), elements.end()));
	}

	// 使用 requires 进行完美物理隔离，彻底断绝链接器实例化未定义对象的可能
	template<typename T>
		requires std::is_pointer_v<std::remove_cvref_t<T>>
	static PyObject *to_pyobj_ptr(T &&val)
	{
		return reinterpret_cast<PyObject *>(val);
	}

	template<typename T>
		requires(!std::is_pointer_v<std::remove_cvref_t<T>>)
	static PyObject *to_pyobj_ptr(T &&val)
	{
		return PyObject::from(std::forward<T>(val)).unwrap();
	}

	// 变长模板：仅用于处理多个参数的情况。拒绝拦截单元素的 vector 等内部组合类型
	template<typename... Args>
		requires(
			!(sizeof...(Args) == 1 && (is_container_arg<std::remove_cvref_t<Args>>::value || ...)))
	static PyResult<PyTuple *> create(Args &&...args)
	{
		return PyTuple::create(
			py::GCVector<Value>{ RtValue(to_pyobj_ptr(std::forward<Args>(args)))... });
	}

	std::string to_string() const override;

	static PyResult<PyObject *> __new__(const PyType *type, PyTuple *args, PyDict *kwargs);

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __iter__() const;
	PyResult<size_t> __len__() const;
	PyResult<PyObject *> __add__(const PyObject *other) const;
	PyResult<PyObject *> __eq__(const PyObject *other) const;
	PyResult<PyObject *> __getitem__(PyObject *key);

	PyResult<PyObject *> __getitem__(int64_t index);

	PyTupleIterator begin() const;
	PyTupleIterator end() const;

	// 修改：将返回类型与底层对齐
	const py::GCVector<Value> &elements() const { return m_elements; }
	size_t size() const { return m_elements.size(); }
	PyResult<PyObject *> operator[](size_t idx) const;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};

template<> PyTuple *as(PyObject *obj);
template<> const PyTuple *as(const PyObject *obj);

class PyTupleIterator : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	friend PyTuple;

	const PyTuple &m_pytuple;
	size_t m_current_index{ 0 };

	PyTupleIterator(const PyTuple &pytuple);
	PyTupleIterator(const PyTuple &pytuple, size_t position);

  protected:
	void visit_graph(Visitor &) override;

  public:
	// 修正：使用 GCVector 的差值类型
	using difference_type = py::GCVector<Value>::difference_type;
	using value_type = PyObject *;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

	static PyResult<PyTupleIterator *> create(const PyTuple &pytuple);

	std::string to_string() const override;

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __next__();

	bool operator==(const PyTupleIterator &) const;
	PyResult<PyObject *> operator*() const;
	PyTupleIterator &operator++();
	PyTupleIterator &operator--();

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};

/// 将可迭代对象解包到固定大小的输出数组
/// 对应 Python 的 UNPACK_SEQUENCE 字节码语义
PyResult<std::monostate> unpack_sequence(PyObject *iterable, int32_t count, PyObject **out);

/// 带星号的扩展解包
/// a, *b, c = iterable
///   before_count: 星号前的变量数量
///   after_count:  星号后的变量数量
///   out 数组大小: before_count + 1(list) + after_count
/// 对应 Python 的 UNPACK_EX 字节码语义
PyResult<std::monostate>
	unpack_ex(PyObject *iterable, int32_t before_count, int32_t after_count, PyObject **out);

}// namespace py
