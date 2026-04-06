Top Hotspots
    Function	Module	CPU Time	% of CPU Time
    py::RtValue::flatten	test	5.636s	10.9%
    rt_getattr_ic	test	4.759s	9.2%
    rt_call_method_ic_ptrs	test	4.653s	9.0%
    rt_getitem	test	3.311s	6.4%
    std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>::insert	test	3.220s	6.2%
    [Others]	N/A*	29.963s	58.1%

Function Stack	CPU Time: Total	CPU Time: Self	Module	Function (Full)	Source File	Start Address
main	98.9%	0s	test	main	[Unknown]	0x40d90
  PyInit_test	97.4%	0s	test	PyInit_test	[Unknown]	0x3d9d0
    rt_call_raw_ptrs	97.4%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x46860
      py::PyNativeFunction::call_fast_ptrs	97.4%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x13fe70
        test.<module>.0:0.run_stress_test.141:0	97.4%	0s	test	test.<module>.0:0.run_stress_test.141:0	[Unknown]	0x3ff50
          rt_call_raw_ptrs	97.4%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x46860
            py::PyNativeFunction::call_fast_ptrs	97.4%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x13fe70
              test.<module>.0:0.find.84:0	97.4%	0s	test	test.<module>.0:0.find.84:0	[Unknown]	0x3f240
                rt_call_method_ic_ptrs	62.9%	0.096s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x543e0
                  test.<module>.0:0.Sieve.8:0.calc.64:4	49.6%	0s	test	test.<module>.0:0.Sieve.8:0.calc.64:4	[Unknown]	0x3edd0
                    rt_call_method_ic_ptrs	49.6%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x543e0
                      test.<module>.0:0.Sieve.8:0.loop_x.56:4	49.0%	0s	test	test.<module>.0:0.Sieve.8:0.loop_x.56:4	[Unknown]	0x3ec60
                        rt_call_method_ic_ptrs	48.9%	0.008s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x543e0
                          test.<module>.0:0.Sieve.8:0.loop_y.48:4	48.9%	0.073s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3ead0
                            rt_call_method_ic_ptrs	44.5%	4.504s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x543e0
                              test.<module>.0:0.Sieve.8:0.step1.33:4	14.0%	0.305s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
                                rt_getitem	2.5%	1.191s	test	rt_getitem(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x50e40
                                rt_getattr_ic	2.5%	1.062s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x52720
                                rt_is_true_fast	1.8%	0.208s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x482f0
                                rt_binary_mul	1.6%	0.398s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d480
                                rt_compare_le	1.2%	0.064s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41ed0
                                rt_binary_mod	0.9%	0.420s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d920
                                rt_binary_add	0.8%	0.200s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d160
                                py::RtValue::from_int_or_box	0.6%	0.310s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x55a20
                                rt_unary_not	0.3%	0.060s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4e710
                                py::RtValue::compare_le	0.3%	0.113s	test	py::RtValue::compare_le(py::RtValue, py::RtValue)	RtValue.cpp	0x57b00
                                rt_setitem	0.3%	0.048s	test	rt_setitem(py::PyObject*, py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x516c0
                                py::RtValue::compare_eq	0.2%	0.092s	test	py::RtValue::compare_eq(py::RtValue, py::RtValue)	RtValue.cpp	0x572a0
                                rt_integer_from_i64	0.1%	0.076s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43800
                                rt_compare_eq	0.1%	0.031s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e40
                                rt_value_array_get	0.1%	0.048s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x460a0
                              test.<module>.0:0.Sieve.8:0.step2.38:4	10.5%	0.264s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
                                rt_binary_mul	2.1%	0.594s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d480
                                rt_getattr_ic	1.8%	0.794s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x52720
                                rt_is_true_fast	1.6%	0.192s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x482f0
                                rt_compare_le	1.1%	0.043s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41ed0
                                rt_getitem	0.9%	0.402s	test	rt_getitem(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x50e40
                                rt_binary_add	0.6%	0.185s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d160
                                rt_binary_mod	0.5%	0.205s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d920
                                py::RtValue::from_int_or_box	0.4%	0.227s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x55a20
                                py::RtValue::compare_le	0.2%	0.083s	test	py::RtValue::compare_le(py::RtValue, py::RtValue)	RtValue.cpp	0x57b00
                                rt_value_array_get	0.1%	0.070s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x460a0
                                rt_unary_not	0.1%	0.008s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4e710
                                rt_compare_eq	0.1%	0.020s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e40
                                py::RtValue::compare_eq	0.1%	0.028s	test	py::RtValue::compare_eq(py::RtValue, py::RtValue)	RtValue.cpp	0x572a0
                                rt_none	0.1%	0.032s	test	rt_none(void)	rt_singleton.cpp	0x50150
                                rt_setitem	0.1%	0.012s	test	rt_setitem(py::PyObject*, py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x516c0
                                rt_integer_from_i64	0.1%	0.028s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43800
                              test.<module>.0:0.Sieve.8:0.step3.43:4	9.5%	0.284s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
                              py::RtValue::box	0.4%	0.193s	test	py::RtValue::box(void) const	RtValue.cpp	0x55830
                              __memmove_avx_unaligned_erms	0.3%	0.179s	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
                              py::types::list	0.3%	0.176s	test	py::types::list(void)	builtin.cpp	0xdd2d0
                              py::types::dict	0.3%	0.169s	test	py::types::dict(void)	builtin.cpp	0xdb150
                              py::types::type	0.1%	0.060s	test	py::types::type(void)	builtin.cpp	0xd6a20
                              py::types::native_function	0.1%	0.060s	test	py::types::native_function(void)	builtin.cpp	0xe1a00
                              py::PyType::global_version	0.1%	0.054s	test	py::PyType::global_version(void)	atomic_base.h	0x20a290
                              func@0x39130	0.0%	0.019s	test	func@0x39130	[Unknown]	0x39130
                            rt_getattr_ic	1.5%	0.723s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x52720
                            rt_compare_lt_bool	1.0%	0.056s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x480f0
                            rt_inplace_add	1.0%	0.076s	test	rt_inplace_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e750
                            rt_binary_mul	0.5%	0.079s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d480
                            py::RtValue::from_int_or_box	0.3%	0.133s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x55a20
                            rt_integer_from_i64	0.0%	0.020s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43800
                        rt_call_raw_ptrs	0.1%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x46860
                      test.<module>.0:0.Sieve.8:0.omit_squares.22:4	0.7%	0s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x3e350
                  test.<module>.0:0.Sieve.8:0.to_list.14:4	6.5%	0.043s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
                  std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>::insert	6.2%	3.220s	test	std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>::insert(__gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>, py::RtValue&&)	stl_vector.h	0x26c770
                  py::PyDictItems::create	0.2%	0s	test	py::PyDictItems::create(py::PyDict const&)	PyDict.cpp	0x12efc0
                  py::PyList::sort	0.1%	0s	test	py::PyList::sort(py::PyTuple*, py::PyDict*)	PyList.cpp	0x15b650
                  py::PyList::pop	0.1%	0.020s	test	py::PyList::pop(py::PyObject*)	PyList.cpp	0x154790
                  rt_list_append	0.0%	0s	test	rt_list_append(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x51aa0
                  rt_unwrap<py::PyDictItems*>	0.0%	0s	test	rt_unwrap<py::PyDictItems*>(py::PyResult<py::PyDictItems*>)	rt_common.hpp	0x26c500
                  __strcmp_avx2	0.0%	0.008s	libc.so.6	__strcmp_avx2	strcmp-avx2.S	0x18b010
                rt_call_raw_ptrs	32.1%	0.008s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x46860
                  py::PyNativeFunction::call_fast_ptrs	29.4%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x13fe70
                    test.<module>.0:0.generate_trie.70:0	29.4%	0.062s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3eed0
                      rt_call_raw_ptrs	15.0%	0.068s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x46860
                        py::PyType::call_fast_ptrs	14.6%	0.510s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x1913c0
                          py::PyObject::init_fast_ptrs	8.9%	0.044s	test	py::PyObject::init_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x194420
                            py::PyNativeFunction::call_fast_ptrs	4.0%	0.057s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x13fe70
                              test.<module>.0:0.Node.3:0.__init__.4:4	3.9%	0.024s	test	test.<module>.0:0.Node.3:0.__init__.4:4	[Unknown]	0x3dce0
                                rt_build_dict	3.0%	0.024s	test	rt_build_dict(int, py::PyObject**, py::PyObject**)	rt_create.cpp	0x444e0
                                  py::PyDict::create	2.8%	0.030s	test	py::PyDict::create(void)	PyDict.cpp	0x125930
                                  _ZNR2py8PyResultIPNS_6PyDictEE6unwrapEv	0.1%	0.076s	test	_ZNR2py8PyResultIPNS_6PyDictEE6unwrapEv	Value.hpp	0x26b0d0
                                rt_setattr_ic	0.8%	0.325s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53760
                                rt_value_array_get	0.0%	0.007s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x460a0
                                rt_false	0.0%	0.007s	test	rt_false(void)	rt_singleton.cpp	0x50170
                            py::PyObject::lookup_attribute	3.8%	0.177s	test	py::PyObject::lookup_attribute(py::PyObject*) const	PyObject.cpp	0x1947a0
                            py::PyString::intern	0.8%	0.020s	test	py::PyString::intern(char const*)	PyString.cpp	0x1d19e0
                            _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.2%	0.094s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x263030
                          py::PyString::create_raw	2.2%	0s	test	py::PyString::create_raw(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0x1d31b0
                          py::PyType::heap_object_allocation	1.8%	0.065s	test	py::PyType::heap_object_allocation(py::PyType*)	PyType.cpp	0x249350
                            std::_Function_handler<py::PyResult<py::PyObject*> (py::PyType*), py::TypePrototype::create<py::PyObject>(std::unique_ptr<py::TypePrototype, std::default_delete<py::TypePrototype>>, std::basic_string_view<char, std::char_traits<char>>, char&&)::{lambda(py::PyType*)#1}>::_M_invoke	1.7%	0.028s	test	std::_Function_handler<py::PyResult<py::PyObject*> (py::PyType*), py::TypePrototype::create<py::PyObject>(std::unique_ptr<py::TypePrototype, std::default_delete<py::TypePrototype>>, std::basic_string_view<char, std::char_traits<char>>, char&&)::{lambda(py::PyType*)#1}>::_M_invoke(std::_Any_data const&, py::PyType*&&)	std_function.h	0x402200
                              py::TypePrototype::create<py::PyObject>(std::basic_string_view<char, std::char_traits<char>>, py::TypePrototype&&)::{lambda(py::PyType*)#1}::operator()(py::PyType*) const::{lambda()#1}::operator()	1.6%	0.017s	test	py::TypePrototype::create<py::PyObject>(std::basic_string_view<char, std::char_traits<char>>, py::TypePrototype&&)::{lambda(py::PyType*)#1}::operator()(py::PyType*) const::{lambda()#1}::operator()(void) const	PyObject.hpp	0x402370
                          std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve	0.4%	0.024s	test	std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve(unsigned long)	vector.tcc	0x3fb5d0
                          py::PyResult<py::PyObject*>::PyResult<py::PyString*>	0.1%	0s	test	py::PyResult<py::PyObject*>::PyResult<py::PyString*>(py::PyResult<py::PyString*> const&)	Value.hpp	0x278ca0
                          __memset_avx2_unaligned_erms	0.1%	0.032s	libc.so.6	__memset_avx2_unaligned_erms	memset-vec-unaligned-erms.S	0x189480
                          py::types::type	0.1%	0.029s	test	py::types::type(void)	builtin.cpp	0xd6a20
                          std::_Function_handler<py::PyResult<py::PyObject*> (py::PyType*), py::PyResult<py::PyObject*> (py::PyType*)*>::_M_invoke	0.0%	0.026s	test	std::_Function_handler<py::PyResult<py::PyObject*> (py::PyType*), py::PyResult<py::PyObject*> (py::PyType*)*>::_M_invoke(std::_Any_data const&, py::PyType*&&)	invoke.h	0x49d0e0
                          _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.0%	0.008s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x263030
                        _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.3%	0.148s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x263030
                        py::RtValue::box	0.0%	0.012s	test	py::RtValue::box(void) const	RtValue.cpp	0x55830
                      rt_load_global	3.4%	0.155s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x40e30
                        py::PyModule::find_symbol_cstr	1.9%	0.260s	test	py::PyModule::find_symbol_cstr(char const*) const	PyModule.cpp	0x1821a0
                        py::ModuleRegistry::find	1.1%	0.028s	test	py::ModuleRegistry::find(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>> const&) const	ModuleRegistry.cpp	0x100890
                        _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.1%	0.064s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x263030
                        py::ModuleRegistry::instance	0.0%	0.023s	test	py::ModuleRegistry::instance(void)	ModuleRegistry.cpp	0x100330
                        py::RtValue::box	0.0%	0.011s	test	py::RtValue::box(void) const	RtValue.cpp	0x55830
                      rt_setitem	2.9%	0.036s	test	rt_setitem(py::PyObject*, py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x516c0
                      rt_getattr_ic	1.9%	0.873s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x52720
                      rt_getitem	1.9%	0.772s	test	rt_getitem(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x50e40
                      rt_compare_not_in	1.9%	0.806s	test	rt_compare_not_in(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x42540
                      rt_iter_next	1.3%	0.117s	test	rt_iter_next(py::PyObject*, bool*)	rt_subscr.cpp	0x50280
                      rt_get_iter	0.3%	0.020s	test	rt_get_iter(py::PyObject*)	rt_subscr.cpp	0x501a0
                      rt_is_true_fast	0.3%	0.066s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x482f0
                      rt_setattr_ic	0.2%	0.095s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53760
                  py::PyType::call_fast_ptrs	2.7%	0.013s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x1913c0
                  _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.0%	0.012s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x263030
                rt_build_tuple	0.8%	0s	test	rt_build_tuple(int, py::PyObject**)	rt_create.cpp	0x43030
                rt_binary_add	0.6%	0.008s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d160
                rt_get_iter	0.3%	0s	test	rt_get_iter(py::PyObject*)	rt_subscr.cpp	0x501a0
                rt_is_true_fast	0.2%	0.021s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x482f0
                rt_getattr_ic	0.2%	0.085s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x52720
                rt_load_global	0.2%	0s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x40e30
                rt_iter_next_unpack2	0.1%	0s	test	rt_iter_next_unpack2(py::PyObject*, py::PyObject**, py::PyObject**)	rt_subscr.cpp	0x508f0
                rt_unpack_sequence	0.1%	0s	test	rt_unpack_sequence(py::PyObject*, int, py::PyObject**)	rt_subscr.cpp	0x50800
                py::RtValue::from_int_or_box	0.0%	0.008s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x55a20
                rt_string_from_cstr	0.0%	0s	test	rt_string_from_cstr(char const*, long)	rt_create.cpp	0x42d40
  rt_init	1.5%	0.020s	test	rt_init(void)	rt_lifecycle.cpp	0x49c90

Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
py::RtValue::flatten	5.636s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x55910
  rt_binary_mul	1.635s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d480
  rt_is_true_fast	1.372s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x482f0
  rt_compare_le	0.954s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41ed0
  rt_binary_add	0.383s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d160
  ↖ rt_inplace_add ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.263s	test	rt_inplace_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e750
  rt_compare_lt_bool	0.232s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x480f0
  ↖ rt_compare_gt ← test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.183s	test	rt_compare_gt(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41f00
  rt_getitem	0.173s	test	rt_getitem(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x50e40
  rt_binary_mod	0.129s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d920
  ↖ rt_binary_sub ← test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.108s	test	rt_binary_sub(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d2f0
  rt_compare_eq	0.079s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e40
  rt_setitem	0.047s	test	rt_setitem(py::PyObject*, py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x516c0
  rt_unary_not	0.044s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4e710
  ↖ rt_compare_not_in ← test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.024s	test	rt_compare_not_in(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x42540
  ↖ rt_list_append ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.011s	test	rt_list_append(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x51aa0
rt_getattr_ic	4.759s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x52720
rt_call_method_ic_ptrs	4.653s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x543e0
  ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	4.504s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3ead0
  ↖ test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.096s	test	test.<module>.0:0.find.84:0	[Unknown]	0x3f240
  ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.045s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
  ↖ test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.008s	test	test.<module>.0:0.Sieve.8:0.loop_x.56:4	[Unknown]	0x3ec60
rt_getitem	3.311s	test	rt_getitem(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x50e40
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	1.191s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
  ↖ test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.772s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3eed0
  ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.557s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.402s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.389s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>::insert	3.220s	test	std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>::insert(__gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>, py::RtValue&&)	stl_vector.h	0x26c770
  ↖ rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	3.220s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x543e0
rt_binary_mul	1.657s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4d480
py::RtValue::box	1.217s	test	py::RtValue::box(void) const	RtValue.cpp	0x55830
std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find	0.848s	test	std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find(std::basic_string_view<char, std::char_traits<char>> const&)	hashtable.h	0x41f160
py::RtValue::from_int_or_box	0.817s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x55a20
rt_compare_not_in	0.806s	test	rt_compare_not_in(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x42540
__memset_avx2_unaligned_erms	0.800s	libc.so.6	__memset_avx2_unaligned_erms	memset-vec-unaligned-erms.S	0x189480
rt_is_true_fast	0.793s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x482f0
py::types::integer	0.789s	test	py::types::integer(void)	builtin.cpp	0xd9830