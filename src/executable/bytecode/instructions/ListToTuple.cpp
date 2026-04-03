#include "ListToTuple.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyTuple.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<RtValue> ListToTuple::execute(VirtualMachine &vm, Interpreter &) const
{
	auto &list = vm.reg(m_list);

	ASSERT(list.is_heap_object());

	auto *pylist = list.as_ptr();
	ASSERT(as<PyList>(pylist));

	auto result = PyTuple::create(as<PyList>(pylist)->elements());
	if (result.is_ok()) {
		vm.reg(m_tuple) = result.unwrap();
		return Ok(RtValue{ result.unwrap() });
	}
	return Err(result.unwrap_err());
}

std::vector<uint8_t> ListToTuple::serialize() const
{
	return {
		LIST_TO_TUPLE,
		m_tuple,
		m_list,
	};
}