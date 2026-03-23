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

static void ensure_ready(PyType *type)
{
    if (!type) return;
    if (!type->underlying_type().is_ready) {
        auto res = type->ready();
        ASSERT(res.is_ok());
    }
}

void initialize_types()
{
#ifndef PYLANG_USE_ARENA
    [[maybe_unused]] auto scope_static_alloc =
        VirtualMachine::the().heap().scoped_static_allocation();
#endif

    // Stage1: 先注册所有内建类型（不做 ready）
    types::type();
    types::str();
    types::slot_wrapper();
    types::tuple();
    types::dict();
    types::object();
    types::native_function();
    types::builtin_method();
    types::method_wrapper();
    types::classmethod_descriptor();
    types::getset_descriptor();
    types::static_method();
    types::property();
    types::classmethod();
    types::member_descriptor();
    types::not_implemented();

    types::super();
    types::bool_();
    types::bytes();
    types::bytes_iterator();
    types::bytearray();
    types::bytearray_iterator();
    types::ellipsis();
    types::str_iterator();
    types::float_();
    types::integer();
    types::complex();
    types::none();
    types::module();
    types::memoryview();
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
    types::llvm_function();
    types::code();
    types::cell();
    types::bound_method();
    types::traceback();
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
    types::deprecation_warning();
    types::import_warning();
    types::pending_deprecation_warning();
    types::resource_warning();
    types::syntax_error();
    types::memory_error();
    types::stop_iteration();
    types::unbound_local_error();

    // Stage2: 核心依赖 ready
    ensure_ready(types::str());
    ensure_ready(types::slot_wrapper());
    ensure_ready(types::tuple());
    ensure_ready(types::dict());
    ensure_ready(types::object());

    ensure_ready(types::native_function());
    ensure_ready(types::builtin_method());
    ensure_ready(types::method_wrapper());
    ensure_ready(types::classmethod_descriptor());
    ensure_ready(types::getset_descriptor());
    ensure_ready(types::static_method());
    ensure_ready(types::property());
    ensure_ready(types::classmethod());
    ensure_ready(types::member_descriptor());
    ensure_ready(types::not_implemented());

    ensure_ready(types::type());

    // Stage3: 其余 ready
    ensure_ready(types::super());
    ensure_ready(types::bool_());
    ensure_ready(types::bytes());
    ensure_ready(types::bytes_iterator());
    ensure_ready(types::bytearray());
    ensure_ready(types::bytearray_iterator());
    ensure_ready(types::ellipsis());
    ensure_ready(types::str_iterator());
    ensure_ready(types::float_());
    ensure_ready(types::integer());
    ensure_ready(types::complex());
    ensure_ready(types::none());
    ensure_ready(types::module());
    ensure_ready(types::memoryview());
    ensure_ready(types::dict_items());
    ensure_ready(types::dict_items_iterator());
    ensure_ready(types::dict_keys());
    ensure_ready(types::dict_key_iterator());
    ensure_ready(types::dict_values());
    ensure_ready(types::dict_value_iterator());
    ensure_ready(types::mappingproxy());
    ensure_ready(types::list());
    ensure_ready(types::list_iterator());
    ensure_ready(types::list_reverseiterator());
    ensure_ready(types::tuple_iterator());
    ensure_ready(types::set());
    ensure_ready(types::frozenset());
    ensure_ready(types::set_iterator());
    ensure_ready(types::range());
    ensure_ready(types::range_iterator());
    ensure_ready(types::reversed());
    ensure_ready(types::zip());
    ensure_ready(types::map());
    ensure_ready(types::enumerate());
    ensure_ready(types::slice());
    ensure_ready(types::iterator());
    ensure_ready(types::function());
    ensure_ready(types::native_function());
    ensure_ready(types::llvm_function());
    ensure_ready(types::code());
    ensure_ready(types::cell());
    ensure_ready(types::bound_method());
    ensure_ready(types::traceback());
    ensure_ready(types::frame());
    ensure_ready(types::namespace_());
    ensure_ready(types::generator());
    ensure_ready(types::coroutine());
    ensure_ready(types::async_generator());
    ensure_ready(types::generic_alias());

    ensure_ready(types::base_exception());
    ensure_ready(types::exception());
    ensure_ready(types::type_error());
    ensure_ready(types::assertion_error());
    ensure_ready(types::attribute_error());
    ensure_ready(types::value_error());
    ensure_ready(types::name_error());
    ensure_ready(types::runtime_error());
    ensure_ready(types::import_error());
    ensure_ready(types::key_error());
    ensure_ready(types::not_implemented_error());
    ensure_ready(types::module_not_found_error());
    ensure_ready(types::os_error());
    ensure_ready(types::lookup_error());
    ensure_ready(types::index_error());
    ensure_ready(types::warning());
    ensure_ready(types::deprecation_warning());
    ensure_ready(types::import_warning());
    ensure_ready(types::pending_deprecation_warning());
    ensure_ready(types::resource_warning());
    ensure_ready(types::syntax_error());
    ensure_ready(types::memory_error());
    ensure_ready(types::stop_iteration());
    ensure_ready(types::unbound_local_error());

    // Stage4: 单例
    (void)py_none();
    (void)py_true();
    (void)py_false();
    (void)py_ellipsis();
    (void)not_implemented();
}

} // namespace py// namespace py