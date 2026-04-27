Top Hotspots
    Function	Module	CPU Time	% of CPU Time
    rt_binary_mul	test	1.720s	8.2%
    py::RtValue::flatten	test	1.336s	6.4%
    rt_call_method_ic_ptrs	test	1.210s	5.8%
    rt_getattr_ic	test	1.023s	4.9%
    rt_list_getitem_i64_not	test	0.716s	3.4%
    [Others]	N/A*	14.899s	71.3%



Function Stack	CPU Time: Total	CPU Time: Self	Module	Function (Full)	Source File	Start Address
    main	96.0%	0s	test	main	[Unknown]	0x1e930
      rt_call_raw_ptrs	96.0%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x216e0
        py::PyNativeFunction::call_fast_ptrs	96.0%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x98ec0
          test.<module>.0:0.run_stress_test.141:0	96.0%	0s	test	test.<module>.0:0.run_stress_test.141:0	[Unknown]	0x1df60
            rt_call_raw_ptrs	96.0%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x216e0
              py::PyNativeFunction::call_fast_ptrs	96.0%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x98ec0
                test.<module>.0:0.find.84:0	96.0%	0.011s	test	test.<module>.0:0.find.84:0	[Unknown]	0x1d6a0
                  rt_call_method_ic_ptrs	58.7%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x28b50
                    test.<module>.0:0.Sieve.8:0.calc.64:4	51.3%	0s	test	test.<module>.0:0.Sieve.8:0.calc.64:4	[Unknown]	0x1d310
                      rt_call_method_ic_ptrs	51.3%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x28b50
                        test.<module>.0:0.Sieve.8:0.loop_x.56:4	50.3%	0s	test	test.<module>.0:0.Sieve.8:0.loop_x.56:4	[Unknown]	0x1d190
                          rt_call_method_ic_ptrs	50.3%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x28b50
                            test.<module>.0:0.Sieve.8:0.loop_y.48:4	50.3%	0.213s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x1d020
                              rt_call_method_ic_ptrs	43.2%	1.199s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x28b50
                                test.<module>.0:0.Sieve.8:0.step1.33:4	12.4%	0.201s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x1cc70
                                  rt_compare_le_bool	3.1%	0.108s	test	rt_compare_le_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x222a0
                                  rt_binary_mul	2.4%	0.492s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x25040
                                  rt_list_getitem_i64_not	1.8%	0.377s	test	rt_list_getitem_i64_not(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22880
                                  rt_binary_mod	1.5%	0.324s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x253c0
                                  rt_getattr_ic	0.9%	0.157s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                                  py::RtValue::from_int_or_box	0.6%	0.115s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x29670
                                  rt_setitem_fast	0.4%	0.034s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x239e0
                                  rt_binary_add	0.3%	0.071s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x24e00
                                  rt_value_array_get	0.2%	0.037s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x21340
                                  rt_integer_from_i64	0.2%	0.033s	test	rt_integer_from_i64(long)	rt_create.cpp	0x1fe40
                                  rt_compare_eq_bool	0.0%	0.008s	test	rt_compare_eq_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22380
                                test.<module>.0:0.Sieve.8:0.step2.38:4	11.6%	0.179s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x1cdc0
                                  rt_compare_le_bool	3.8%	0.090s	test	rt_compare_le_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x222a0
                                    py::RtValue::flatten	3.4%	0.436s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x29580
                                      __gmpz_fits_slong_p	0.6%	0.123s	libgmp.so.10	__gmpz_fits_slong_p	[Unknown]	0x198d0
                                      __gmpz_get_si	0.3%	0.063s	libgmp.so.10	__gmpz_get_si	[Unknown]	0x19a20
                                      func@0x1b520	0.2%	0.034s	test	func@0x1b520	[Unknown]	0x1b520
                                      py::py_false	0.1%	0.026s	test	py::py_false(void)	PyBool.cpp	0x7dfa0
                                      py::py_true	0.1%	0.011s	test	py::py_true(void)	PyBool.cpp	0x7def0
                                      func@0x1bd20	0.1%	0.011s	test	func@0x1bd20	[Unknown]	0x1bd20
                                  rt_binary_mul	2.4%	0.505s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x25040
                                  rt_list_getitem_i64_not	1.2%	0.245s	test	rt_list_getitem_i64_not(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22880
                                  rt_binary_mod	1.1%	0.220s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x253c0
                                  rt_getattr_ic	0.9%	0.175s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                                  rt_binary_add	0.4%	0.086s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x24e00
                                  rt_integer_from_i64	0.3%	0.067s	test	rt_integer_from_i64(long)	rt_create.cpp	0x1fe40
                                  rt_setitem_fast	0.2%	0.011s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x239e0
                                  rt_compare_eq_bool	0.2%	0.037s	test	rt_compare_eq_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22380
                                  py::RtValue::from_int_or_box	0.1%	0.030s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x29670
                                  rt_value_array_get	0.1%	0.022s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x21340
                                test.<module>.0:0.Sieve.8:0.step3.43:4	8.3%	0.254s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x1cee0
                                  rt_binary_mul	2.5%	0.517s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x25040
                                  rt_compare_le_bool	1.3%	0.034s	test	rt_compare_le_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x222a0
                                    py::RtValue::flatten	1.2%	0.130s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x29580
                                      __gmpz_fits_slong_p	0.2%	0.038s	libgmp.so.10	__gmpz_fits_slong_p	[Unknown]	0x198d0
                                      __gmpz_get_si	0.1%	0.026s	libgmp.so.10	__gmpz_get_si	[Unknown]	0x19a20
                                      py::py_true	0.1%	0.026s	test	py::py_true(void)	PyBool.cpp	0x7def0
                                      py::py_false	0.1%	0.015s	test	py::py_false(void)	PyBool.cpp	0x7dfa0
                                      py::PyInteger::as_big_int	0.1%	0.011s	test	py::PyInteger::as_big_int(void) const	PyInteger.cpp	0x9e350
                                  rt_getattr_ic	0.6%	0.112s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                                  rt_binary_sub	0.5%	0.097s	test	rt_binary_sub(py::PyObject*, py::PyObject*)	rt_op.cpp	0x24f20
                                  rt_list_getitem_i64_not	0.4%	0.094s	test	rt_list_getitem_i64_not(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22880
                                  rt_value_array_get	0.4%	0.093s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x21340
                                  rt_setitem_fast	0.4%	0.045s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x239e0
                                  rt_binary_mod	0.3%	0.067s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x253c0
                                  rt_compare_gt_bool	0.3%	0.056s	test	rt_compare_gt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22310
                                  rt_integer_from_i64	0.2%	0.044s	test	rt_integer_from_i64(long)	rt_create.cpp	0x1fe40
                                  py::RtValue::from_int_or_box	0.1%	0.011s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x29670
                                  rt_compare_eq_bool	0.1%	0.011s	test	rt_compare_eq_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22380
                                py::types::dict	1.5%	0.309s	test	py::types::dict(void)	builtin.cpp	0x635d0
                                py::types::native_function	1.2%	0.261s	test	py::types::native_function(void)	builtin.cpp	0x68bc0
                                py::types::list	1.0%	0.205s	test	py::types::list(void)	builtin.cpp	0x65150
                                __memmove_avx_unaligned_erms	0.6%	0.134s	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
                                py::py_none	0.4%	0.093s	test	py::py_none(void)	PyNone.cpp	0xb91f0
                                py::PyType::global_version	0.2%	0.048s	test	py::PyType::global_version(void)	atomic_base.h	0xeed20
                                func@0x1b0e0	0.1%	0.026s	test	func@0x1b0e0	[Unknown]	0x1b0e0
                              rt_compare_lt_bool	3.6%	0.114s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22230
                              rt_binary_mul	0.9%	0.187s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x25040
                              rt_getattr_ic	0.8%	0.164s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                              rt_inplace_add	0.7%	0.138s	test	rt_inplace_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x25dc0
                              rt_integer_from_i64	0.1%	0.022s	test	rt_integer_from_i64(long)	rt_create.cpp	0x1fe40
                        test.<module>.0:0.Sieve.8:0.omit_squares.22:4	1.0%	0s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x1cb00
                    test.<module>.0:0.Sieve.8:0.to_list.14:4	7.2%	0.064s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x1c910
                      rt_iter_next	4.9%	0.059s	test	rt_iter_next(py::PyObject*, bool*)	rt_subscr.cpp	0x26c60
                        py::PyRangeIterator::next_fast	4.4%	0.115s	test	py::PyRangeIterator::next_fast(void)	PyRange.cpp	0xcfe00
                          __gmpz_fits_slong_p	1.0%	0.212s	libgmp.so.10	__gmpz_fits_slong_p	[Unknown]	0x198d0
                          __gmpz_cmp	1.0%	0.212s	libgmp.so.10	__gmpz_cmp	[Unknown]	0xe9f0
                          __gmpz_add	0.9%	0.196s	libgmp.so.10	__gmpz_add	[Unknown]	0x12990
                          func@0x1b160	0.3%	0.067s	test	func@0x1b160	[Unknown]	0x1b160
                          func@0x1b520	0.3%	0.064s	test	func@0x1b520	[Unknown]	0x1b520
                          func@0x1bd10	0.1%	0.030s	test	func@0x1bd10	[Unknown]	0x1bd10
                          __gmpz_get_si	0.1%	0.023s	libgmp.so.10	__gmpz_get_si	[Unknown]	0x19a20
                          func@0x1bd20	0.1%	0.011s	test	func@0x1bd20	[Unknown]	0x1bd20
                        py::RtValue::from_int_or_box	0.1%	0.030s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x29670
                      rt_list_getitem_i64_truthy	1.0%	0.212s	test	rt_list_getitem_i64_truthy(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x226c0
                      rt_getattr_ic	0.8%	0.156s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                      rt_call_method_ic_ptrs	0.2%	0.011s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x28b50
                    __strcmp_avx2	0.1%	0.011s	libc.so.6	__strcmp_avx2	strcmp-avx2.S	0x18b010
                    rt_list_append	0.1%	0s	test	rt_list_append(py::PyObject*, py::PyObject*)	rt_subscr.cpp	0x277d0
                    rt_unwrap<py::PyObject*>	0.0%	0s	test	rt_unwrap<py::PyObject*>(py::PyResult<py::PyObject*>)	rt_common.hpp	0x113e00
                    py::PyList::pop	0.0%	0s	test	py::PyList::pop(py::PyObject*)	PyList.cpp	0xa2e10
                    py::PyList::sort	0.0%	0s	test	py::PyList::sort(py::PyTuple*, py::PyDict*)	PyList.cpp	0xa6020
                  rt_call_raw_ptrs	34.7%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x216e0
                    py::PyNativeFunction::call_fast_ptrs	34.1%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x98ec0
                      test.<module>.0:0.generate_trie.70:0	32.8%	0.026s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x1d3f0
                        rt_call_raw_ptrs	9.2%	0.037s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x216e0
                          py::PyType::call_fast_ptrs	8.8%	0.138s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0xbe120
                            py::PyObject::init_fast_ptrs	4.6%	0.076s	test	py::PyObject::init_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0xbf310
                              py::PyNativeFunction::call_fast_ptrs	4.1%	0.056s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x98ec0
                                test.<module>.0:0.Node.3:0.__init__.4:4	3.7%	0.011s	test	test.<module>.0:0.Node.3:0.__init__.4:4	[Unknown]	0x1c5d0
                                  rt_build_dict	2.6%	0s	test	rt_build_dict(int, py::PyObject**, py::PyObject**)	rt_create.cpp	0x20360
                                    py::PyDict::create	2.5%	0.011s	test	py::PyDict::create(void)	PyDict.cpp	0x8ca60
                                      py::Arena::allocate<py::PyDict>	2.2%	0.112s	test	py::Arena::allocate<py::PyDict>()	Arena.hpp	0x1a6820
                                      _pylang_debug_log_alloc	0.3%	0.060s	test	_pylang_debug_log_alloc(char const*, std::atomic<unsigned long>&)	compat.hpp	0x10fa20
                                    _ZNR2py8PyResultIPNS_6PyDictEE6unwrapEv	0.0%	0.008s	test	_ZNR2py8PyResultIPNS_6PyDictEE6unwrapEv	Value.hpp	0x1133b0
                                  rt_setattr_ic	1.1%	0.097s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28660
                                py::py_none	0.1%	0.030s	test	py::py_none(void)	PyNone.cpp	0xb91f0
                              py::PyType::global_version	0.1%	0.011s	test	py::PyType::global_version(void)	atomic_base.h	0xeed20
                              [Unknown stack frame(s)]	0.0%	0s	[Unknown]	[Unknown stack frame(s)]	[Unknown]	0
                            py::PyType::heap_object_allocation	1.5%	0s	test	py::PyType::heap_object_allocation(py::PyType*)	PyType.cpp	0x106da0
                            py::PyString::create_raw	0.9%	0s	test	py::PyString::create_raw(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0xd8bd0
                            std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve	0.7%	0.011s	test	std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve(unsigned long)	vector.tcc	0x1d2ea0
                            py::PyResult<py::PyObject*>::PyResult<py::PyString*>	0.2%	0.011s	test	py::PyResult<py::PyObject*>::PyResult<py::PyString*>(py::PyResult<py::PyString*> const&)	Value.hpp	0x11a000
                            py::PyType::issubclass	0.1%	0.011s	test	py::PyType::issubclass(py::PyType const*) const	PyType.cpp	0x1010c0
                            py::types::str	0.1%	0.011s	test	py::types::str(void)	builtin.cpp	0x616e0
                            [Unknown stack frame(s)]	0.1%	0s	[Unknown]	[Unknown stack frame(s)]	[Unknown]	0
                          _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.2%	0.037s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x10f2f0
                          [Unknown stack frame(s)]	0.1%	0s	[Unknown]	[Unknown stack frame(s)]	[Unknown]	0
                        rt_list_getitem_i64	5.9%	0.100s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x223f0
                          py::PyObject::getitem	4.3%	0.116s	test	py::PyObject::getitem(py::PyObject*)	PyObject.cpp	0xc9180
                            _ZN2py12_GLOBAL__N_19call_slotISt8functionIFNS_8PyResultIPNS_8PyObjectEEES5_S5_EES6_JS5_RS5_EEET0_RKSt7variantIJT_S5_EEDpOT1_Qsr3stdE9is_same_vINSA_6OkTypeES5_E	2.5%	0.138s	test	_ZN2py12_GLOBAL__N_19call_slotISt8functionIFNS_8PyResultIPNS_8PyObjectEEES5_S5_EES6_JS5_RS5_EEET0_RKSt7variantIJT_S5_EEDpOT1_Qsr3stdE9is_same_vINSA_6OkTypeES5_E	PyObject.cpp	0xc6b10
                              py::PyDict::__getitem__	1.8%	0.019s	test	py::PyDict::__getitem__(py::PyObject*)	PyDict.cpp	0x8d0c0
                                py::find_dict_key	1.3%	0.277s	test	py::find_dict_key(tsl::ordered_map<py::RtValue, py::RtValue, py::RtValueHash, py::RtValueEq, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>, std::vector<std::pair<py::RtValue, py::RtValue>, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>>, unsigned int>&, py::PyObject*)	PyDict.cpp	0x8d410
                                py::PyObject::from<py::RtValue>	0.3%	0.049s	test	py::PyObject::from<py::RtValue>(py::RtValue const&)	PyObject.cpp	0xc28e0
                              std::_Function_handler<py::PyResult<py::PyObject*> (py::PyObject*, py::PyObject*), py::PyResult<py::PyObject*> (py::PyObject*, py::PyObject*)*>::_M_invoke	0.1%	0.019s	test	std::_Function_handler<py::PyResult<py::PyObject*> (py::PyObject*, py::PyObject*), py::PyResult<py::PyObject*> (py::PyObject*, py::PyObject*)*>::_M_invoke(std::_Any_data const&, py::PyObject*&&, py::PyObject*&&)	invoke.h	0x15c220
                            py::PyObject::type_prototype	0.9%	0.179s	test	py::PyObject::type_prototype(void) const	PyObject.cpp	0xbd3e0
                            py::PyObject::as_mapping	0.4%	0.011s	test	py::PyObject::as_mapping(void)	PyObject.cpp	0xbd8c0
                          py::RtValue::flatten	0.5%	0.045s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x29580
                          _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.5%	0.096s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x10f2f0
                          py::types::list	0.2%	0.032s	test	py::types::list(void)	builtin.cpp	0x65150
                        rt_load_global	5.6%	0.138s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x1e9b0
                          py::PyModule::find_symbol_cstr	3.0%	0.115s	test	py::PyModule::find_symbol_cstr(char const*) const	PyModule.cpp	0xb7810
                          py::ModuleRegistry::find	1.7%	0.029s	test	py::ModuleRegistry::find(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>> const&) const	ModuleRegistry.cpp	0x7b9a0
                          _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.1%	0.022s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x10f2f0
                          py::ModuleRegistry::instance	0.1%	0.019s	test	py::ModuleRegistry::instance(void)	ModuleRegistry.cpp	0x7b6b0
                        rt_iter_next	4.7%	0.067s	test	rt_iter_next(py::PyObject*, bool*)	rt_subscr.cpp	0x26c60
                        rt_setitem_fast	3.5%	0s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x239e0
                        rt_compare_not_in_bool	2.1%	0.397s	test	rt_compare_not_in_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x234b0
                        rt_getattr_ic	1.0%	0.205s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                        rt_get_iter	0.6%	0.011s	test	rt_get_iter(py::PyObject*)	rt_subscr.cpp	0x26bd0
                        rt_setattr_ic	0.1%	0s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28660
                      [Unknown stack frame(s)]	1.2%	0s	[Unknown]	[Unknown stack frame(s)]	[Unknown]	0
                    py::PyType::call_fast_ptrs	0.6%	0s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0xbe120
                  rt_binary_add	1.2%	0s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x24e00
                  rt_dict_items_iter_for_loop	0.4%	0.040s	test	rt_dict_items_iter_for_loop(py::PyObject*)	rt_subscr.cpp	0x27f20
                  rt_load_global	0.3%	0.022s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x1e9b0
                  rt_iter_next_unpack2	0.2%	0.011s	test	rt_iter_next_unpack2(py::PyObject*, py::PyObject**, py::PyObject**)	rt_subscr.cpp	0x26f30
                  py::rt::truthy	0.1%	0s	test	py::rt::truthy(py::PyObject*)	rt_tagged_ops.hpp	0x114b10
                  rt_getattr_ic	0.1%	0.026s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
                  rt_list_insert_0_tuple2	0.1%	0s	test	rt_list_insert_0_tuple2(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x23090
                  rt_unpack_sequence	0.1%	0s	test	rt_unpack_sequence(py::PyObject*, int, py::PyObject**)	rt_subscr.cpp	0x26ea0
__clone3	3.7%	0s	libc.so.6	__clone3	clone3.S	0x129c40


Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
rt_binary_mul	1.720s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x25040
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.517s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x1cee0
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.505s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x1cdc0
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.492s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x1cc70
  ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.187s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x1d020
  ↖ test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.019s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x1cb00
py::RtValue::flatten	1.336s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x29580
  rt_compare_le_bool	0.883s	test	rt_compare_le_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x222a0
    ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.436s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x1cdc0
    ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.317s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x1cc70
    ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.130s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x1cee0
  rt_compare_lt_bool	0.409s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22230
    ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.361s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x1d020
    ↖ test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.049s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x1cb00
  ↖ rt_list_getitem_i64 ← test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.045s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x223f0
rt_call_method_ic_ptrs	1.210s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x28b50
  ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	1.199s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x1d020
  ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.011s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x1c910
rt_getattr_ic	1.023s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x28080
  ↖ test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.205s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x1d3f0
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.175s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x1cdc0
  ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.164s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x1d020
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.157s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x1cc70
  ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.156s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x1c910
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.112s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x1cee0
  ↖ test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.030s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x1cb00
  ↖ test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.026s	test	test.<module>.0:0.find.84:0	[Unknown]	0x1d6a0
rt_list_getitem_i64_not	0.716s	test	rt_list_getitem_i64_not(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x22880
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.377s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x1cc70
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.245s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x1cdc0
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.094s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x1cee0
rt_binary_mod	0.611s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x253c0
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.324s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x1cc70
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.220s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x1cdc0
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.067s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x1cee0
std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find	0.590s	test	std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find(std::basic_string_view<char, std::char_traits<char>> const&)	hashtable.h	0x1e44d0
  py::PyString::intern	0.433s	test	py::PyString::intern(char const*)	PyString.cpp	0xd7ed0
    ↖ py::PyModule::find_symbol_cstr ← rt_load_global ← test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.224s	test	py::PyModule::find_symbol_cstr(char const*) const	PyModule.cpp	0xb7810
    ↖ py::PyStringIterator::next_raw ← rt_iter_next ← test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.209s	test	py::PyStringIterator::next_raw(void)	PyString.cpp	0xe8970
  ↖ py::PyString::intern ← py::PyString::__add__ ← py::PySequenceWrapper::concat ← py::PyObject::add ← rt_binary_add ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.157s	test	py::PyString::intern(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0xd8520
__gmpz_fits_slong_p	0.574s	libgmp.so.10	__gmpz_fits_slong_p	[Unknown]	0x198d0
func@0x1aee0	0.485s	libgc.so.1	func@0x1aee0	[Unknown]	0x1aee0