#include "PyTuple.hpp"
#include "IndexError.hpp"
#include "MemoryError.hpp"
#include "PyBool.hpp"
#include "PyInteger.hpp"
#include "PyList.hpp"
#include "PySlice.hpp"
#include "PyString.hpp"
#include "StopIteration.hpp"
#include "TypeError.hpp"
#include "ValueError.hpp"
#include "interpreter/InterpreterCore.hpp"
#include "runtime/compat.hpp"
#include "taggered_pointer/RtValue.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


namespace py {

template<> PyTuple *as(PyObject *obj)
{
	if (obj->type() == types::tuple()) { return static_cast<PyTuple *>(obj); }
	return nullptr;
}

template<> const PyTuple *as(const PyObject *obj)
{
	if (obj->type() == types::tuple()) { return static_cast<const PyTuple *>(obj); }
	return nullptr;
}

namespace {
	std::vector<Value> make_value_vector(const std::vector<PyObject *> &elements)
	{
		ASSERT(std::all_of(
			elements.begin(), elements.end(), [](const auto &el) { return el != nullptr; }));
		std::vector<Value> result;
		result.reserve(elements.size());
		result.insert(result.end(), elements.begin(), elements.end());
		return result;
	}

	PyTuple *get_empty_tuple_singleton()
	{
		// C++11 保证局部静态变量初始化是线程安全的
		static PyTuple *empty = []() {
			auto *t = PYLANG_ALLOC_IMMORTAL(PyTuple, py::GCVector<Value>{});
			// 必须确保它被 GC 识别为根（如果 intern_roots_push_extra 可用）
			// intern_roots_push_extra(t);
			return t;
		}();
		return empty;
	}

	std::vector<Value> make_value_vector(std::vector<PyObject *> &&elements)
	{
		ASSERT(std::all_of(
			elements.begin(), elements.end(), [](const auto &el) { return el != nullptr; }));
		std::vector<Value> result;
		result.reserve(elements.size());
		result.insert(result.end(), elements.begin(), elements.end());
		return result;
	}
}// namespace

PyTuple::PyTuple(PyType *type) : PyBaseObject(type) {}

PyTuple::PyTuple(PyType *type, std::vector<Value> elements)
	: PyBaseObject(type), m_elements(std::make_move_iterator(elements.begin()),
							  std::make_move_iterator(elements.end()))// 修复迭代移动
{
	ASSERT(std::all_of(m_elements.begin(), m_elements.end(), [](const auto &el) {
		if (el.is_heap_object()) return el.as_ptr() != nullptr;
		return true;
	}));
}

PyTuple::PyTuple(std::vector<Value> &&elements)
	: PyBaseObject(types::BuiltinTypes::the().tuple()),
	  m_elements(std::make_move_iterator(elements.begin()),
		  std::make_move_iterator(elements.end()))// 修复迭代移动
{
	ASSERT(std::all_of(m_elements.begin(), m_elements.end(), [](const auto &el) {
		if (el.is_heap_object()) return el.as_ptr() != nullptr;
		return true;
	}));
}

PyTuple::PyTuple(py::GCVector<Value> &&elements)
	: PyBaseObject(types::tuple()), m_elements(std::move(elements))
{}

PyTuple::PyTuple(PyType *type, py::GCVector<Value> &&elements)
	: PyBaseObject(type), m_elements(std::move(elements))
{}

PyTuple::PyTuple() : PyTuple(std::vector<Value>{}) {}

PyTuple::PyTuple(const std::vector<PyObject *> &elements) : PyTuple(make_value_vector(elements)) {}

PyTuple::PyTuple(PyType *type, const std::vector<PyObject *> &elements)
	: PyTuple(type, make_value_vector(elements))
{}

// PyResult<PyTuple *> PyTuple::create()
// {

// 	if (auto *obj = PYLANG_ALLOC(PyTuple, )) { return Ok(obj); }
// 	return Err(memory_error(sizeof(PyTuple)));
// }

PyResult<PyTuple *> PyTuple::create() { return PyTuple::create(py::GCVector<Value>{}); }

PyResult<PyTuple *> PyTuple::create(py::GCVector<Value> &&elements)
{
	// 如果要创建的是0元素元组，永远复用单例！极大减缓冲击。
	if (elements.empty()) { return Ok(get_empty_tuple_singleton()); }

	auto *obj = PYLANG_ALLOC(PyTuple, std::move(elements));
	if (!obj) return Err(memory_error(sizeof(PyTuple)));
	return Ok(obj);
}

PyResult<PyTuple *> PyTuple::create(std::vector<Value> &&elements)
{
	py::GCVector<Value> tmp(
		std::make_move_iterator(elements.begin()), std::make_move_iterator(elements.end()));
	return PyTuple::create(std::move(tmp));
}

PyResult<PyTuple *> PyTuple::create(PyType *type, std::vector<Value> elements)
{

	if (auto *obj = PYLANG_ALLOC(PyTuple, type, std::move(elements))) { return Ok(obj); }
	return Err(memory_error(sizeof(PyTuple)));
}

PyResult<PyTuple *> PyTuple::create(PyType *type, const std::vector<PyObject *> &elements)
{

	if (auto *obj = PYLANG_ALLOC(PyTuple, type, elements)) { return Ok(obj); }
	return Err(memory_error(sizeof(PyTuple)));
}

PyResult<PyTuple *> PyTuple::create(const std::vector<PyObject *> &elements)
{

	if (auto *obj = PYLANG_ALLOC(PyTuple, elements)) { return Ok(obj); }
	return Err(memory_error(sizeof(PyTuple)));
}

PyResult<PyTuple *> PyTuple::create(std::vector<PyObject *> &&elements)
{

	if (auto *obj = PYLANG_ALLOC(PyTuple, make_value_vector(std::move(elements)))) {
		return Ok(obj);
	}
	return Err(memory_error(sizeof(PyTuple)));
}

std::string PyTuple::to_string() const
{
	std::ostringstream os;

	os << "(";
	if (!m_elements.empty()) {
		auto it = m_elements.begin();
		while (std::next(it) != m_elements.end()) {
			[&os](const auto &v) {
				auto r = v.box()->repr();
				ASSERT(r.is_ok());
				os << r.unwrap()->to_string();
			}(*it);
			std::advance(it, 1);
			os << ", ";
		}
		[&os](const auto &v) {
			auto r = v.box()->repr();
			ASSERT(r.is_ok());
			os << r.unwrap()->to_string();
		}(*it);
	}
	if (m_elements.size() == 1) { os << ','; }
	os << ")";

	return os.str();
}

PyResult<PyObject *> PyTuple::__new__(const PyType *type, PyTuple *args, PyDict *kwargs)
{
	if (!type->issubclass(types::tuple())) {
		return Err(type_error(
			"tuple.__new__({}): {} is not a subtype of tuple", type->name(), type->name()));
	}

	auto result = PyArgsParser<PyObject *>::unpack_tuple(args,
		kwargs,
		"tuple",
		std::integral_constant<size_t, 0>{},
		std::integral_constant<size_t, 1>{},
		nullptr /* iterable */);

	if (result.is_err()) { return Err(result.unwrap_err()); }

	auto [iterable] = result.unwrap();
	if (!iterable) {

		if (auto *obj = PYLANG_ALLOC(PyTuple, const_cast<PyType *>(type))) { return Ok(obj); }
		return Err(memory_error(sizeof(PyTuple)));
	}

	auto iterator_ = iterable->iter();
	if (iterator_.is_err()) { return iterator_; }
	auto iterator = iterator_.unwrap();

	auto els_ = PyList::create();
	if (els_.is_err()) { return Err(els_.unwrap_err()); }
	auto els = els_.unwrap();

	auto value = iterator->next();
	while (value.is_ok()) {
		els->elements().push_back(value.unwrap());
		value = iterator->next();
	}

	if (!value.unwrap_err()->type()->issubclass(stop_iteration()->type())) { return value; }

	// 修改：用 GCVector 而不是退化到 std::vector 去做临时中转
	py::GCVector<Value> tmp_vec(std::make_move_iterator(els->elements().begin()),
		std::make_move_iterator(els->elements().end()));
	return PyTuple::create(std::move(tmp_vec));
}

PyResult<PyObject *> PyTuple::__repr__() const { return PyString::create(to_string()); }

PyResult<PyObject *> PyTuple::__iter__() const { return PyTupleIterator::create(*this); }

PyResult<size_t> PyTuple::__len__() const { return Ok(m_elements.size()); }

PyResult<PyObject *> PyTuple::__add__(const PyObject *other) const
{
	auto *b = as<PyTuple>(other);
	if (!b) {
		return Err(
			type_error("can only concatenate tuple (not \"{}\") to tuple", other->type()->name()));
	}
	if (m_elements.empty()) return Ok(const_cast<PyObject *>(other));

	// 修改：因为 m_elements 现在是 GCVector，不能直接赋值给 std::vector
	// 通过迭代器区间将其拷贝进普通的 C++ 临时 vector，以便传递给后续操作
	std::vector<Value> elements(m_elements.begin(), m_elements.end());
	elements.insert(elements.end(), b->elements().begin(), b->elements().end());
	return PyTuple::create(std::move(elements));
}

PyResult<PyObject *> PyTuple::__eq__(const PyObject *other) const
{
	if (!as<PyTuple>(other)) { return Ok(py_false()); }

	auto *other_tuple = as<PyTuple>(other);
	if (m_elements.size() != other_tuple->elements().size()) { return Ok(py_false()); }

	// 修改：使用 RuntimeContext 替代 VirtualMachine::the().interpreter()
	const bool result = std::equal(m_elements.begin(),
		m_elements.end(),
		other_tuple->elements().begin(),
		[](const auto &lhs, const auto &rhs) -> bool {
			if (!RuntimeContext::has_current()) { return false; }
			auto &ctx = RuntimeContext::current();
			auto *eq_result =
				ctx.equals(PyObject::from(lhs).unwrap(), PyObject::from(rhs).unwrap());
			return ctx.is_true(eq_result);
		});
	return Ok(result ? py_true() : py_false());
}

PyResult<PyObject *> PyTuple::__getitem__(int64_t index)
{
	if (index < 0) {
		if (static_cast<size_t>(std::abs(index)) > m_elements.size()) {
			return Err(index_error("tuple index out of range"));
		}
		index += m_elements.size();
	}
	ASSERT(index >= 0);
	if (static_cast<size_t>(index) >= m_elements.size()) {
		return Err(index_error("tuple index out of range"));
	}
	return PyObject::from(m_elements[index]);
}

PyResult<PyObject *> PyTuple::__getitem__(PyObject *index)
{
	if (auto index_int = as<PyInteger>(index)) {
		const auto i = index_int->as_i64();
		return __getitem__(i);
	} else if (auto slice = as<PySlice>(index)) {
		auto indices_ = slice->unpack();
		if (indices_.is_err()) return Err(indices_.unwrap_err());
		const auto [start_, end_, step] = indices_.unwrap();

		const auto [start, end, slice_length] =
			PySlice::adjust_indices(start_, end_, step, m_elements.size());

		if (slice_length == 0) { return PyTuple::create(); }
		if (start == 0 && end == static_cast<int64_t>(m_elements.size()) && step == 1) {
			return Ok(this);
		}

		std::vector<Value> new_tuple_values;
		new_tuple_values.reserve(slice_length);
		for (int64_t idx = start, i = 0; i < slice_length; idx += step, ++i) {
			new_tuple_values.push_back(m_elements[idx]);
		}
		return PyTuple::create(new_tuple_values);
	} else {
		return Err(
			type_error("tuple indices must be integers or slices, not {}", index->type()->name()));
	}
}

PyTupleIterator PyTuple::begin() const { return PyTupleIterator(*this); }

PyTupleIterator PyTuple::end() const { return PyTupleIterator(*this, m_elements.size()); }

PyResult<PyObject *> PyTuple::operator[](size_t idx) const
{
	return PyObject::from(m_elements[idx]);
}

void PyTuple::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	for (auto &el : m_elements) {
		if (el.is_heap_object()) {
			if (el.as_ptr() != this) { visitor.visit(*el.as_ptr()); }
		}
	}
}

/*
PyType *PyTuple::static_type() const { return types::tuple(); }
*/

namespace {

	std::once_flag tuple_flag;

	std::unique_ptr<TypePrototype> register_tuple()
	{
		return std::move(klass<PyTuple>("tuple").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyTuple::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(tuple_flag, []() { type = register_tuple(); });
		return std::move(type);
	};
}


PyTupleIterator::PyTupleIterator(const PyTuple &pytuple)
	: PyBaseObject(types::BuiltinTypes::the().tuple_iterator()), m_pytuple(pytuple)
{}

PyTupleIterator::PyTupleIterator(const PyTuple &pytuple, size_t position) : PyTupleIterator(pytuple)
{
	m_current_index = position;
}

PyResult<PyTupleIterator *> PyTupleIterator::create(const PyTuple &pytuple)
{

	auto *obj = PYLANG_ALLOC(PyTupleIterator, pytuple);
	if (!obj) return Err(memory_error(sizeof(PyTupleIterator)));
	return Ok(obj);
}

std::string PyTupleIterator::to_string() const
{
	return fmt::format("<tuple_iterator at {}>", static_cast<const void *>(this));
}

PyResult<PyObject *> PyTupleIterator::__repr__() const { return PyString::create(to_string()); }

PyResult<PyObject *> PyTupleIterator::__next__()
{
	if (m_current_index < m_pytuple.elements().size())
		return PyObject::from(m_pytuple.elements()[m_current_index++]);
	return Err(stop_iteration());
}

bool PyTupleIterator::operator==(const PyTupleIterator &other) const
{
	return &m_pytuple == &other.m_pytuple && m_current_index == other.m_current_index;
}

PyTupleIterator &PyTupleIterator::operator++()
{
	m_current_index++;
	return *this;
}

PyTupleIterator &PyTupleIterator::operator--()
{
	m_current_index--;
	return *this;
}

PyResult<PyObject *> PyTupleIterator::operator*() const
{
	return PyObject::from(m_pytuple.elements()[m_current_index]);
}

void PyTupleIterator::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	visitor.visit(const_cast<PyTuple &>(m_pytuple));
}


/*
PyType *PyTupleIterator::static_type() const { return types::tuple_iterator(); }
*/

namespace {

	std::once_flag tuple_iterator_flag;

	std::unique_ptr<TypePrototype> register_tuple_iterator()
	{
		return std::move(klass<PyTupleIterator>("tuple_iterator").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyTupleIterator::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(tuple_iterator_flag, []() { type = register_tuple_iterator(); });
		return std::move(type);
	};
}

PyResult<std::monostate> unpack_sequence(PyObject *iterable, int32_t count, PyObject **out)
{
	auto iter_result = iterable->iter();
	if (iter_result.is_err()) return Err(iter_result.unwrap_err());
	auto *iter = iter_result.unwrap();

	for (int32_t i = 0; i < count; ++i) {
		auto value = iter->next();
		if (value.is_err()) {
			if (value.unwrap_err()->type() == stop_iteration()->type()) {
				return Err(
					value_error("not enough values to unpack (expected {}, got {})", count, i));
			}
			return Err(value.unwrap_err());
		}
		out[i] = value.unwrap();
	}

	auto extra = iter->next();
	if (extra.is_ok()) {
		return Err(value_error("too many values to unpack (expected {})", count));
	}
	if (extra.unwrap_err()->type() != stop_iteration()->type()) { return Err(extra.unwrap_err()); }

	return Ok(std::monostate{});
}

PyResult<std::monostate>
	unpack_ex(PyObject *iterable, int32_t before_count, int32_t after_count, PyObject **out)
{
	// --- Step 1: 获取迭代器 ---
	auto iter_result = iterable->iter();
	if (iter_result.is_err()) { return Err(iter_result.unwrap_err()); }
	auto *iter = iter_result.unwrap();

	// --- Step 2: 收集所有元素 ---
	std::vector<PyObject *> elements;
	while (true) {
		auto next = iter->next();
		if (next.is_err()) {
			if (next.unwrap_err()->type()->issubclass(stop_iteration()->type())) { break; }
			return Err(next.unwrap_err());
		}
		elements.push_back(next.unwrap());
	}

	// --- Step 3: 数量校验 ---
	const int32_t total = static_cast<int32_t>(elements.size());
	const int32_t min_required = before_count + after_count;
	if (total < min_required) {
		return Err(value_error(
			"not enough values to unpack (expected at least {}, got {})", min_required, total));
	}

	// --- Step 4: 填充 before_count 个前置元素 ---
	for (int32_t i = 0; i < before_count; ++i) { out[i] = elements[i]; }

	// --- Step 5: 中间 list（星号变量）---
	const int32_t middle_count = total - before_count - after_count;
	std::vector<Value> middle_vals;
	middle_vals.reserve(middle_count);
	for (int32_t i = before_count; i < before_count + middle_count; ++i) {
		middle_vals.push_back(elements[i]);
	}
	auto middle_list = PyList::create(middle_vals);
	if (middle_list.is_err()) { return Err(middle_list.unwrap_err()); }
	out[before_count] = middle_list.unwrap();

	// --- Step 6: 填充 after_count 个后置元素 ---
	for (int32_t i = 0; i < after_count; ++i) {
		out[before_count + 1 + i] = elements[before_count + middle_count + i];
	}

	return Ok(std::monostate{});
}

}// namespace py
