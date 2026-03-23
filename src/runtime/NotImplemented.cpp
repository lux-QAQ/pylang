#include "NotImplemented.hpp"
#include "MemoryError.hpp"
#include "runtime/compat.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"

namespace py {

NotImplemented::NotImplemented(PyType *type) : PyBaseObject(type) {}

NotImplemented::NotImplemented() : PyBaseObject(types::BuiltinTypes::the().not_implemented()) {}

PyResult<NotImplemented *> NotImplemented::create()
{

	auto *result = PYLANG_ALLOC(NotImplemented, );
	if (!result) { return Err(memory_error(sizeof(NotImplemented))); }
	return Ok(result);
}

/*
PyType *NotImplemented::static_type() const { return types::not_implemented(); }
*/

std::string NotImplemented::to_string() const { return "NotImplemented"; }

namespace {
	std::once_flag not_implemented_flag;

	std::unique_ptr<TypePrototype> register_not_implemented()
	{
		return std::move(klass<NotImplemented>("NotImplemented").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> NotImplemented::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(not_implemented_flag, []() { type = register_not_implemented(); });
		return std::move(type);
	};
}

// NotImplemented *not_implemented()
// {
// 	static NotImplemented *value = nullptr;
// 	if (!value) { value = NotImplemented::create().unwrap(); }
// 	return value;
// }
NotImplemented *not_implemented()
{
	static NotImplemented *value = nullptr;
	if (!value) {
		Arena *saved = Arena::has_current() ? &Arena::current() : nullptr;
		Arena::set_current(&ArenaManager::program_arena());
		value = NotImplemented::create().unwrap();
		if (saved) Arena::set_current(saved);
	}
	return value;
}
}// namespace py
