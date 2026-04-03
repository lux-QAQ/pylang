#include "LoadAttr.hpp"
#include "interpreter/Interpreter.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFrame.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyString.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<Value> LoadAttr::execute(VirtualMachine &vm, Interpreter &interpreter) const
{
	auto this_value = vm.reg(m_value_source);
	const auto &attribute_name = interpreter.execution_frame()->names(m_attr_name);
	if (auto obj_res = PyObject::from(this_value); obj_res.is_ok()) {
		spdlog::debug("This object: {}", obj_res.unwrap()->to_string());
	}
	auto result = [&]() -> PyResult<Value> {
		[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
		if (this_value.is_heap_object()) {
			auto name = PyString::create(attribute_name);
			if (auto r = this_value.as_ptr()->get_attribute(name.unwrap()); r.is_ok()) {
				return Ok(Value{ r.unwrap() });
			} else {
				return Err(r.unwrap_err());
			}
		} else {
			auto result = PyObject::from(this_value).and_then([&attribute_name](PyObject *obj) {
				auto name = PyString::create(attribute_name);
				return obj->get_attribute(name.unwrap());
			});
			if (result.is_ok()) {
				return Ok(Value{ result.unwrap() });
			} else {
				return Err(result.unwrap_err());
			}
		}
	}();

	if (result.is_ok()) { vm.reg(m_destination) = result.unwrap(); }
	return result;
}

std::vector<uint8_t> LoadAttr::serialize() const
{
	return {
		LOAD_ATTR,
		m_destination,
		m_value_source,
		m_attr_name,
	};
}