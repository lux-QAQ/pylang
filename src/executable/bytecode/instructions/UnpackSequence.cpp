#include "UnpackSequence.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyEllipsis.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/types/api.hpp"

#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyNumber.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/TypeError.hpp"
#include "runtime/ValueError.hpp"
#include "vm/VM.hpp"

#include "../serialization/serialize.hpp"

using namespace py;

PyResult<Value> UnpackSequence::execute(VirtualMachine &vm, Interpreter &) const
{
	const auto &source = vm.reg(m_source);

	return [&]() -> PyResult<Value> {
		if (source.is_heap_object()) {
			auto *obj_ptr = source.as_ptr();
			if (auto *pytuple = as<PyTuple>(obj_ptr)) {
				if (pytuple->elements().size() > m_destination.size()) {
					return Err(value_error(
						"too many values to unpack (expected {})", m_destination.size()));
				} else if (pytuple->elements().size() < m_destination.size()) {
					return Err(value_error("not enough values to unpack (expected {}, got {})",
						m_destination.size(),
						pytuple->elements().size()));
				} else {
					size_t idx{ 0 };
					for (const auto &el : pytuple->elements()) {
						vm.reg(m_destination[idx++]) = el;
					}
					return Ok(Value{ py_none() });
				}
			} else if (auto *pylist = as<PyList>(obj_ptr)) {
				if (pylist->elements().size() > m_destination.size()) {
					return Err(value_error(
						"too many values to unpack (expected {})", m_destination.size()));
				} else if (pylist->elements().size() < m_destination.size()) {
					return Err(value_error("not enough values to unpack (expected {}, got {})",
						m_destination.size(),
						pylist->elements().size()));
				} else {
					size_t idx{ 0 };
					for (const auto &el : pylist->elements()) { vm.reg(m_destination[idx++]) = el; }
					return Ok(Value{ py_none() });
				}
			} else {
				auto mapping = obj_ptr->as_mapping();
				if (mapping.is_err()) { return Err(mapping.unwrap_err()); }
				const auto source_size = [&] {
					[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
					return mapping.unwrap().len();
				}();
				if (source_size.is_err()) { return Err(source_size.unwrap_err()); }
				auto len = source_size.unwrap();
				if (len != m_destination.size()) {
					return Err(value_error(
						"too many values to unpack (expected {})", m_destination.size()));
				}
				TODO();
			}
		} else {
			return Err(type_error("cannot unpack non-iterable object"));
		}
	}();
}

std::vector<uint8_t> UnpackSequence::serialize() const
{
	std::vector<uint8_t> bytes{
		UNPACK_SEQUENCE,
	};
	::serialize(m_destination, bytes);
	::serialize(m_source, bytes);

	return bytes;
}
