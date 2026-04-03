#include "ImportName.hpp"

#include "interpreter/Interpreter.hpp"
#include "runtime/ImportError.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFrame.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/TypeError.hpp"
#include "vm/VM.hpp"

#include "../serialization/serialize.hpp"

#include <numeric>

using namespace py;

PyResult<Value> ImportName::execute(VirtualMachine &vm, Interpreter &interpreter) const
{
	const std::string name = interpreter.execution_frame()->names(m_name);
	const auto &from_list = vm.reg(m_from_list);
	const auto &level = vm.reg(m_level);

	auto *builtins = interpreter.execution_frame()->builtins();
	auto import_str = PyString::create("__import__");
	if (import_str.is_err()) return import_str;

	auto import_func_opt = builtins->symbol_table()->operator[](import_str.unwrap());
	if (!import_func_opt) { return Err(import_error("__import__ not found")); }

	const auto import_func = *import_func_opt;

	if (!import_func.is_heap_object()) { return Err(type_error("__import__ is not callable")); }
	if (!import_func.as_ptr()) { return Err(import_error("__import__ not available")); }

	auto arg0 = PyString::create(name);
	if (arg0.is_err()) return arg0;

	auto args = PyTuple::create(arg0.unwrap(),
		interpreter.execution_frame()->globals(),
		interpreter.execution_frame()->locals(),
		PyObject::from(from_list).unwrap(),
		PyObject::from(level).unwrap());
	if (args.is_err()) return args;

	auto module = import_func.as_ptr()->call(args.unwrap(), nullptr);

	return module.and_then([&](auto *m) {
		vm.reg(m_destination) = m;
		return Ok(m);
	});
}

std::vector<uint8_t> ImportName::serialize() const
{
	std::vector<uint8_t> result{
		IMPORT_NAME,
		m_destination,
		m_name,
		m_from_list,
		m_level,
	};

	return result;
}