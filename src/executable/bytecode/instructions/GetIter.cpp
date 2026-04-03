#include "GetIter.hpp"
#include "runtime/PyObject.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<RtValue> GetIter::execute(VirtualMachine &vm, Interpreter &) const
{
	auto iterable_value = vm.reg(m_src);
	auto result = [&]() {
		[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
		if (iterable_value.is_heap_object()) {
			return iterable_value.as_ptr()->iter();
		} else {
			if (auto obj = PyObject::from(iterable_value); obj.is_ok()) {
				return obj.unwrap()->iter();
			} else {
				return obj;
			}
		}
	}();
	if (result.is_ok()) {
		vm.reg(m_dst) = result.unwrap();
		return Ok(RtValue{ result.unwrap() });
	} else {
		return Err(result.unwrap_err());
	}
}

std::vector<uint8_t> GetIter::serialize() const
{
	return {
		GET_ITER,
		m_dst,
		m_src,
	};
}
