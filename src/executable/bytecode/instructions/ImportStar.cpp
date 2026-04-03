#include "ImportStar.hpp"
#include "interpreter/Interpreter.hpp"
#include "runtime/AttributeError.hpp"
#include "runtime/IndexError.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyType.hpp"
#include "runtime/TypeError.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<Value> ImportStar::execute(VirtualMachine &vm, Interpreter &interpreter) const
{
	auto module_ = vm.reg(m_src);
	ASSERT(module_.is_heap_object());
	auto *obj = module_.as_ptr();
	ASSERT(as<PyModule>(obj));
	auto *module_obj = as<PyModule>(obj);
	auto *symbol_table = module_obj->symbol_table();
	auto all_str = PyString::create("__all__").unwrap();
	if (const auto it = symbol_table->map().find(Value::from_ptr(all_str));
		it != symbol_table->map().end()) {
		auto all_ = PyObject::from(it->second);
		if (all_.is_err()) { return all_; }
		auto all_sequence_ = all_.unwrap()->as_sequence();
		if (all_sequence_.is_err()) { return Err(all_sequence_.unwrap_err()); }
		auto all_sequence = all_sequence_.unwrap();
		for (int64_t i = 0;; ++i) {
			auto el = [&] {
				[[maybe_unused]] RAIIStoreNonCallInstructionData non_call_instruction_data;
				return all_sequence.getitem(i);
			}();
			if (el.is_err()) {
				if (el.unwrap_err()->type()->issubclass(IndexError::class_type())) { break; }
				return Err(el.unwrap_err());
			}
			auto name = as<PyString>(el.unwrap());
			if (!name) {
				return Err(type_error("Item in {}.__all__ must be str, not {}",
					module_obj->name()->value(),
					el.unwrap()->type()->name()));
			}
			auto it = symbol_table->map().find(Value::from_ptr(name));
			if (it == symbol_table->map().end()) {
				return Err(attribute_error("module '{}' has no attribute '{}'",
					module_obj->name()->value(),
					name->value()));
			}
			if (auto r = interpreter.store_object(name->value(), it->second); r.is_err()) {
				return Err(r.unwrap_err());
			}
		}
	} else {
		for (const auto &[key, value] : symbol_table->map()) {
			const auto &k = key;
			const auto key_str = [key = k]() -> std::string {
				if (key.is_heap_object()) {
					ASSERT(as<PyString>(key.as_ptr()));
					return as<PyString>(key.as_ptr())->value();
				} else {
					TODO();
				}
			}();

			ASSERT(!key_str.empty());
			if (key_str[0] != '_') {
				if (auto r = interpreter.store_object(key_str, value); r.is_err()) {
					return Err(r.unwrap_err());
				}
			}
		}
	}
	return Ok(py_none());
}

std::vector<uint8_t> ImportStar::serialize() const
{
	return {
		IMPORT_STAR,
		m_src,
	};
}
