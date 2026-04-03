#include "DictUpdate.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyTuple.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<Value> DictUpdate::execute(VirtualMachine &vm, Interpreter &) const
{
	auto &dst = vm.reg(m_dst);
	auto &src = vm.reg(m_src);

	ASSERT(dst.is_heap_object());
	ASSERT(src.is_heap_object());

	auto *dst_dict = as<PyDict>(dst.as_ptr());
	ASSERT(dst_dict);
	auto args = PyTuple::create(src.as_ptr(), py_true());
	return dst_dict->merge(args.unwrap(), nullptr);
}

std::vector<uint8_t> DictUpdate::serialize() const
{
	std::vector<uint8_t> bytes{
		DICT_UPDATE,
		m_dst,
		m_src,
	};
	return bytes;
}
