#include "PyBool.hpp"
#include "PyString.hpp"
#include "runtime/compat.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


namespace py {

namespace {
	PyObject *s_py_true = nullptr;
	PyObject *s_py_false = nullptr;
}// namespace

template<> PyBool *as(PyObject *node)
{
	if (node->type() == types::bool_()) { return static_cast<PyBool *>(node); }
	return nullptr;
}

template<> const PyBool *as(const PyObject *node)
{
	if (node->type() == types::bool_()) { return static_cast<const PyBool *>(node); }
	return nullptr;
}

PyBool::PyBool(PyType *type) : PyInteger(type) {}

PyBool::PyBool(bool value) : PyInteger(types::bool_(), value) {}

std::string PyBool::to_string() const { return value() ? "True" : "False"; }

bool PyBool::value() const { return static_cast<bool>(m_value); }

PyResult<PyObject *> PyBool::__new__(const PyType *type, PyTuple *args, PyDict *kwargs)
{
	ASSERT(!kwargs || kwargs->map().size() == 0);
	ASSERT(args && args->size() == 1);
	ASSERT(type == types::bool_());

	const auto &value = PyObject::from(args->elements()[0]);

	if (value.is_err()) return value;

	if (value.unwrap()->type() == types::bool_()) return value;

	return value.unwrap()->true_().and_then(
		[](const auto &v) { return Ok(v ? py_true() : py_false()); });
}

PyResult<PyObject *> PyBool::__repr__() const { return PyString::create(to_string()); }

PyResult<bool> PyBool::true_() { return Ok(value()); }

PyResult<PyBool *> PyBool::create(bool value)
{

	auto *result = PYLANG_ALLOC(PyBool, value);
	ASSERT(result);
	return Ok(result);
}

// PyType *PyBool::static_type() const { return types::bool_(); }

// PyObject *py_true()
// {
// 	static PyObject *value = nullptr;

// 	if (!value) { value = PyBool::create(true).unwrap(); }

// 	return value;
// }

// PyObject *py_false()
// {
// 	static PyObject *value = nullptr;

// 	if (!value) { value = PyBool::create(false).unwrap(); }

// 	return value;
// }
PyObject *py_true()
{
	if (__builtin_expect(s_py_true != nullptr, 1)) { return s_py_true; }
	Arena *saved = Arena::has_current() ? &Arena::current() : nullptr;
	Arena::set_current(&ArenaManager::program_arena());
	s_py_true = PyBool::create(true).unwrap();
	if (saved) {
		Arena::set_current(saved);
	} else {
		Arena::set_current(nullptr);
	}
	return s_py_true;
}

PyObject *py_false()
{
	if (__builtin_expect(s_py_false != nullptr, 1)) { return s_py_false; }
	Arena *saved = Arena::has_current() ? &Arena::current() : nullptr;
	Arena::set_current(&ArenaManager::program_arena());
	s_py_false = PyBool::create(false).unwrap();
	if (saved) {
		Arena::set_current(saved);
	} else {
		Arena::set_current(nullptr);
	}
	return s_py_false;
}

void initialize_bool_singletons()
{
	(void)py_true();
	(void)py_false();
}

namespace {

	std::once_flag bool_flag;

	std::unique_ptr<TypePrototype> register_bool()
	{
		return std::move(klass<PyBool>("bool", types::integer()).type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyBool::type_factory()
{
	return []() {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(bool_flag, []() { type = register_bool(); });
		return std::move(type);
	};
}

}// namespace py
