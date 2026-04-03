#include "Unary.hpp"
#include "interpreter/Interpreter.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/TypeError.hpp"
#include "vm/VM.hpp"

using namespace py;

namespace {
PyResult<Value> unary_positive(const Value &val)
{
	if (auto r = val.box()->pos(); r.is_ok()) {
		return Ok(Value{ r.unwrap() });
	} else {
		return Err(r.unwrap_err());
	}
}

PyResult<Value> unary_negative(const Value &val)
{
	if (auto r = val.box()->neg(); r.is_ok()) {
		return Ok(Value{ r.unwrap() });
	} else {
		return Err(r.unwrap_err());
	}
}

PyResult<Value> unary_not(const Value &val, Interpreter &interpreter)
{
	return truthy(val, interpreter).and_then([](bool v) { return Ok(v ? py_false() : py_true()); });
}

PyResult<Value> unary_invert(const Value &val)
{
	if (auto r = val.box()->invert(); r.is_ok()) {
		return Ok(Value{ r.unwrap() });
	} else {
		return Err(r.unwrap_err());
	}
}

}// namespace

PyResult<Value> Unary::execute(VirtualMachine &vm, Interpreter &interpreter) const
{
	const auto &val = vm.reg(m_source);
	auto result = [&]() -> PyResult<Value> {
		switch (m_operation) {
		case Operation::POSITIVE: {
			[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
			return unary_positive(val);
		} break;
		case Operation::NEGATIVE: {
			[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
			return unary_negative(val);
		} break;
		case Operation::INVERT: {
			[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
			return unary_invert(val);
		} break;
		case Operation::NOT: {
			[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
			return unary_not(val, interpreter);
		} break;
		}
		ASSERT_NOT_REACHED();
	}();

	if (result.is_err()) return result;
	vm.reg(m_destination) = result.unwrap();
	return result;
}

std::vector<uint8_t> Unary::serialize() const
{
	return {
		UNARY,
		m_destination,
		m_source,
		static_cast<uint8_t>(m_operation),
	};
}
