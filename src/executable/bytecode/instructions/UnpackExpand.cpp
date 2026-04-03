#include "UnpackExpand.hpp"
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

PyResult<Value> UnpackExpand::execute(VirtualMachine &vm, Interpreter &) const
{
	const auto &source = vm.reg(m_source);

	return [&]() -> PyResult<Value> {
		if (source.is_heap_object()) {
			auto *obj_ptr = source.as_ptr();
			if (auto *pytuple = as<PyTuple>(obj_ptr)) {
				if (pytuple->elements().size() < m_destination.size()) {
					return Err(
						value_error("not enough values to unpack (expected at least {}, got 0)",
							m_destination.size()));
				} else {
					size_t i = 0;
					for (; i < m_destination.size(); ++i) {
						vm.reg(m_destination[i]) = pytuple->elements()[i];
					}
					std::vector<Value> rest;
					for (; i < pytuple->elements().size(); ++i) {
						rest.push_back(pytuple->elements()[i]);
					}
					return PyList::create(std::move(rest)).and_then([this, &vm](auto *rest) {
						vm.reg(m_rest) = rest;
						return Ok(Value{ py_none() });
					});
				}
			} else if (auto *pylist = as<PyList>(obj_ptr)) {
				if (pylist->elements().size() < m_destination.size()) {
					return Err(value_error("not enough values to unpack (expected {}, got {})",
						m_destination.size(),
						pylist->elements().size()));
				} else {
					size_t i = 0;
					for (; i < m_destination.size(); ++i) {
						vm.reg(m_destination[i]) = pylist->elements()[i];
					}
					std::vector<Value> rest;
					for (; i < pylist->elements().size(); ++i) {
						rest.push_back(pylist->elements()[i]);
					}
					return PyList::create(std::move(rest)).and_then([this, &vm](auto *rest) {
						vm.reg(m_rest) = rest;
						return Ok(Value{ py_none() });
					});
				}
			} else if (auto *pystr = as<PyString>(obj_ptr)) {
				const auto &str = pystr->value();
				if (str.size() < m_destination.size()) {
					return Err(value_error("not enough values to unpack (expected {}, got {})",
						m_destination.size(),
						str.size()));
				} else {
					size_t i = 0;
					for (; i < m_destination.size(); ++i) {
						auto r = PyString::create(std::string{ str[i] });
						if (r.is_err()) { return Err(r.unwrap_err()); }
						vm.reg(m_destination[i]) = Value::from_ptr(r.unwrap());
					}
					std::vector<Value> rest;
					for (; i < str.size(); ++i) {
						auto r = PyString::create(std::string{ str[i] });
						if (r.is_err()) { return Err(r.unwrap_err()); }
						rest.push_back(Value::from_ptr(r.unwrap()));
					}
					return PyList::create(std::move(rest)).and_then([this, &vm](auto *rest) {
						vm.reg(m_rest) = rest;
						return Ok(Value{ py_none() });
					});
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
				if (len < m_destination.size()) {
					return Err(value_error("not enough values to unpack (expected {}, got {})",
						m_destination.size(),
						len));
				}
				TODO();
			}
		} else if (source.is_tagged_int()) {
			return Err(type_error("cannot unpack non-iterable int object"));
		} else {
			return Err(type_error("cannot unpack non-iterable object"));
		}
	}();
}

std::vector<uint8_t> UnpackExpand::serialize() const
{
	std::vector<uint8_t> bytes{
		UNPACK_SEQUENCE,
	};
	::serialize(m_destination, bytes);
	::serialize(m_rest, bytes);
	::serialize(m_source, bytes);

	return bytes;
}
