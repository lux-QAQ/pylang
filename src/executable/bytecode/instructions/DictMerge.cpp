#include "DictMerge.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyTuple.hpp"
#include "vm/VM.hpp"

using namespace py;

PyResult<RtValue> DictMerge::execute(VirtualMachine &vm, Interpreter &) const
{
	auto this_dict = vm.reg(m_this_dict);
	auto other_dict = vm.reg(m_other_dict);

	auto *this_pydict = this_dict.box();
	ASSERT(this_pydict);
	ASSERT(as<PyDict>(this_pydict));

	auto *other_pydict = other_dict.box();
	ASSERT(other_pydict);
	ASSERT(as<PyDict>(other_pydict));

	auto args = PyTuple::create(other_pydict);
	if (args.is_err()) { return Err(args.unwrap_err()); }
	return as<PyDict>(this_pydict)->merge(args.unwrap(), nullptr);
}

std::vector<uint8_t> DictMerge::serialize() const
{
	return {
		DICT_MERGE,
		m_this_dict,
		m_other_dict,
	};
}
