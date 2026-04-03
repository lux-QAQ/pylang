#include "DictAdd.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyNone.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<RtValue> DictAdd::execute(VirtualMachine &vm, Interpreter &) const
{
	auto &dict = vm.reg(m_dict);
	auto &key = vm.reg(m_key);
	auto &value = vm.reg(m_value);

	ASSERT(dict.is_heap_object());

	auto *pydict = dict.as_ptr();
	ASSERT(pydict);
	ASSERT(as<PyDict>(pydict));

	as<PyDict>(pydict)->insert(key.as_ptr(), value.as_ptr());

	return Ok(py_none());
}

std::vector<uint8_t> DictAdd::serialize() const
{
	return {
		DICT_ADD,
		m_dict,
		m_key,
		m_value,
	};
}
