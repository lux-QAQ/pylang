#include "YieldValue.hpp"
#include "interpreter/Interpreter.hpp"
#include "runtime/PyFrame.hpp"
#include "vm/VM.hpp"


using namespace py;

PyResult<Value> YieldValue::execute(VirtualMachine &vm, Interpreter &interpreter) const
{
	auto result = vm.reg(m_source);

	if (auto obj_res = PyObject::from(result); obj_res.is_ok()) {
		spdlog::debug("Return value: {}", obj_res.unwrap()->to_string());
	}

	ASSERT(interpreter.execution_frame()->generator() != nullptr);

	vm.reg(0) = result;

	vm.pop_frame(true);

	return Ok(result);
}

std::vector<uint8_t> YieldValue::serialize() const
{
	return {
		YIELD_VALUE,
		m_source,
	};
}
