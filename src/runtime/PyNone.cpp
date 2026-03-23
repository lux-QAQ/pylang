#include "PyNone.hpp"
#include "PyString.hpp"
#include "runtime/compat.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


namespace py {

PyNone::PyNone() : PyBaseObject(types::BuiltinTypes::the().none()) {}

PyNone::PyNone(PyType *type) : PyBaseObject(type) {}

std::string PyNone::to_string() const { return "None"; }

PyResult<PyObject *> PyNone::__repr__() const { return PyString::create(to_string()); }

PyResult<PyObject *> PyNone::__add__(const PyObject *other) const
{
	return Err(type_error(
		"unsupported operand type(s) for +: 'NoneType' and '{}'", other->type()->name()));
}

PyResult<bool> PyNone::__bool__() const { return Ok(false); }

PyResult<bool> PyNone::true_() { return Ok(false); }

PyNone *PyNone::create() { return PYLANG_ALLOC(PyNone, ); }

/*
PyType *PyNone::static_type() const { return types::none(); }
*/

namespace {

	std::once_flag none_flag;

	std::unique_ptr<TypePrototype> register_none()
	{
		return std::move(klass<PyNone>("NoneType").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyNone::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(none_flag, []() { type = register_none(); });
		return std::move(type);
	};
}

// PyObject *py_none()
// {
// 	static PyObject *value = nullptr;

// 	if (!value) { value = PyNone::create(); }

// 	return value;
// }
PyObject *py_none()
{
	static PyObject *value = nullptr;
	if (!value) {
		Arena *saved = Arena::has_current() ? &Arena::current() : nullptr;
		Arena::set_current(&ArenaManager::program_arena());
		value = PyNone::create();
		if (saved) Arena::set_current(saved);
	}
	return value;
}

}// namespace py
