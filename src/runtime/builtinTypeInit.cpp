#include "runtime/BaseException.hpp"
#include "runtime/Import.hpp"
#include "runtime/KeyError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/NotImplemented.hpp"
#include "runtime/PyBool.hpp"
#include "runtime/PyCode.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyEllipsis.hpp"
#include "runtime/PyFrame.hpp"
#include "runtime/PyFunction.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyNone.hpp"
#include "runtime/PyNumber.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/PyType.hpp"
#include "runtime/Value.hpp"
#include "runtime/modules/Modules.hpp"
#include "runtime/modules/config.hpp"
#include "runtime/types/builtin.hpp"

namespace py {

void initialize_types()
{
#ifndef PYLANG_USE_ARENA
	[[maybe_unused]] auto scope_static_alloc =
		VirtualMachine::the().heap().scoped_static_allocation();

#endif
	// 优先级 1: 基础设施类型 (被其他类型的 ready() 所依赖)
	types::slot_wrapper();// 必须在所有 ready() 之前，因为 ready 需要创建 SlotWrapper
	types::str();// 必须在所有 ready() 之前，因为 ready 需要创建属性名字符串
	types::tuple();// 必须在所有 ready() 之前，因为 ready 需要创建 MRO 元组
	types::dict();// 必须在所有 ready() 之前，因为 ready 需要创建 __dict__


	// 然后是核心元类和基类
	types::type();
	types::object();

	types::super();
	types::bool_();
	types::bytes();
	types::bytes_iterator();
	types::bytearray();
	types::bytearray_iterator();
	types::ellipsis();
	//types::str();
	types::str_iterator();
	types::float_();
	types::integer();
	types::none();
	types::module();
	types::memoryview();
	types::dict();
	types::dict_items();
	types::dict_items_iterator();
	types::dict_keys();
	types::dict_key_iterator();
	types::dict_values();
	types::dict_value_iterator();
	types::mappingproxy();
	types::list();
	types::list_iterator();
	types::list_reverseiterator();
	//types::tuple();
	types::tuple_iterator();
	types::set();
	types::frozenset();
	types::set_iterator();
	types::range();
	types::range_iterator();
	types::reversed();
	types::zip();
	types::map();
	types::enumerate();
	types::slice();
	types::iterator();
	types::function();
	types::native_function();
	types::llvm_function();
	types::code();
	types::cell();
	types::builtin_method();
	//types::slot_wrapper();
	types::bound_method();
	types::method_wrapper();
	types::classmethod_descriptor();
	types::getset_descriptor();
	types::static_method();
	types::property();
	types::classmethod();
	types::member_descriptor();
	types::traceback();
	types::not_implemented();
	types::frame();
	types::namespace_();
	types::generator();
	types::coroutine();
	types::async_generator();
	types::generic_alias();

	types::base_exception();
	types::exception();
	types::type_error();
	types::assertion_error();
	types::attribute_error();
	types::value_error();
	types::name_error();
	types::runtime_error();
	types::import_error();
	types::key_error();
	types::not_implemented_error();
	types::module_not_found_error();
	types::os_error();
	types::lookup_error();
	types::index_error();
	types::warning();
	types::import_warning();
	types::syntax_error();
	types::memory_error();
	types::stop_iteration();
	types::unbound_local_error();


	(void)py_none();
	(void)py_true();
	(void)py_false();
	(void)py_ellipsis();
	(void)not_implemented();
}

}// namespace py