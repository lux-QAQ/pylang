#include "GetYieldFromIter.hpp"
#include "runtime/PyCoroutine.hpp"
#include "runtime/PyGenerator.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<RtValue> GetYieldFromIter::execute(VirtualMachine &vm, Interpreter &) const
{
	auto iterable_value = vm.reg(m_src);
	ASSERT(iterable_value.is_heap_object());

	if (auto *generator = as<PyGenerator>(iterable_value.as_ptr())) {
		vm.reg(m_dst) = generator;
		return Ok(RtValue(generator));
	} else if (auto *coro = as<PyCoroutine>(iterable_value.as_ptr())) {
		(void)coro;
		TODO();
	} else {
		[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
		return iterable_value.as_ptr()->iter().and_then([this, &vm](PyObject *obj) {
			vm.reg(m_dst) = obj;
			return Ok(RtValue(obj));
		});
	}
}

std::vector<uint8_t> GetYieldFromIter::serialize() const
{
	return {
		GET_YIELD_FROM_ITER,
		m_dst,
		m_src,
	};
}
