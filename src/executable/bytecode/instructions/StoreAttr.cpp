#include "StoreAttr.hpp"
#include "interpreter/Interpreter.hpp"
#include "runtime/PyFrame.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<Value> StoreAttr::execute(VirtualMachine &vm, Interpreter &intepreter) const
{
	auto this_value = vm.reg(m_dst);
	const auto &attr_name_ = intepreter.execution_frame()->names(m_attr_name);
	spdlog::debug("This object: {}",
		this_value.is_heap_object() ? this_value.as_ptr()->to_string() : "non-heap object");
	if (this_value.is_heap_object()) {
		auto *this_obj = this_value.as_ptr();
		auto other_obj = PyObject::from(vm.reg(m_src));
		if (other_obj.is_err()) return Err(other_obj.unwrap_err());
		auto attr_name = PyString::create(attr_name_);
		if (attr_name.is_err()) { return Err(attr_name.unwrap_err()); }
		[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
		if (auto result = this_obj->setattribute(attr_name.unwrap(), other_obj.unwrap());
			result.is_ok()) {
			return Ok(py_none());
		} else {
			return Err(result.unwrap_err());
		}
	} else {
		TODO();
		return Err(nullptr);
	}
}

std::vector<uint8_t> StoreAttr::serialize() const
{
	return {
		STORE_ATTR,
		m_dst,
		m_src,
		m_attr_name,
	};
}
