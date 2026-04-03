#include "FunctionCall.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyTuple.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<RtValue> FunctionCall::execute(VirtualMachine &vm, Interpreter &) const
{
	auto func_val = vm.reg(m_function_name);
	auto callable_object = func_val.box();

	std::vector<RtValue> args;
	args.reserve(m_size);
	if (m_size > 0) {
		auto *el = vm.sp() - m_size;
		for (; el < vm.sp(); ++el) { args.push_back(*el); }
	}

	auto args_tuple = PyTuple::create(args);
	if (args_tuple.is_err()) { return Err(args_tuple.unwrap_err()); }
	spdlog::debug("args_tuple: {}", (void *)args_tuple.unwrap());
	spdlog::debug("calling function: \'{}\'", callable_object->to_string());

	auto result = callable_object->call(args_tuple.unwrap(), nullptr);
	if (result.is_ok()) {
		vm.reg(0) = result.unwrap();
		return Ok(RtValue{ result.unwrap() });
	}
	return Err(result.unwrap_err());
}

std::vector<uint8_t> FunctionCall::serialize() const
{
	ASSERT(m_size < std::numeric_limits<uint8_t>::max());
	ASSERT(m_stack_offset < std::numeric_limits<uint8_t>::max());

	return {
		FUNCTION_CALL,
		m_function_name,
		static_cast<uint8_t>(m_size),
		static_cast<uint8_t>(m_stack_offset),
	};
}