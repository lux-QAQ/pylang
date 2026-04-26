Top Hotspots
    Function	Module	CPU Time	% of CPU Time
    rt_list_getitem_i64	test	2.714s	10.2%
    py::py_true	test	1.554s	5.8%
    rt_binary_mul	test	1.399s	5.3%
    py::RtValue::flatten	test	1.333s	5.0%
    rt_getattr_ic	test	1.078s	4.1%
    [Others]	N/A*	18.514s	69.6%


Function Stack	CPU Time: Total	CPU Time: Self	Module	Function (Full)	Source File	Start Address
main	98.5%	0s	test	main	[Unknown]	0x3f1e0
  rt_call_raw_ptrs	96.3%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x41570
    py::PyNativeFunction::call_fast_ptrs	96.3%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0xb3720
      test.<module>.0:0.run_stress_test.141:0	96.3%	0s	test	test.<module>.0:0.run_stress_test.141:0	[Unknown]	0x3e810
        rt_call_raw_ptrs	96.3%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x41570
          py::PyNativeFunction::call_fast_ptrs	96.3%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0xb3720
            test.<module>.0:0.find.84:0	96.2%	0s	test	test.<module>.0:0.find.84:0	[Unknown]	0x3df50
              rt_call_method_ic_ptrs	66.1%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x46d40
                test.<module>.0:0.Sieve.8:0.calc.64:4	59.8%	0s	test	test.<module>.0:0.Sieve.8:0.calc.64:4	[Unknown]	0x3dbc0
                  rt_call_method_ic_ptrs	59.8%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x46d40
                    test.<module>.0:0.Sieve.8:0.loop_x.56:4	59.2%	0s	test	test.<module>.0:0.Sieve.8:0.loop_x.56:4	[Unknown]	0x3da40
                      rt_call_method_ic_ptrs	59.1%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x46d40
                        test.<module>.0:0.Sieve.8:0.loop_y.48:4	59.0%	0.220s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3d8d0
                          rt_call_method_ic_ptrs	53.9%	0.790s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x46d40
                            test.<module>.0:0.Sieve.8:0.step1.33:4	19.6%	0.271s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3d4b0
                              rt_list_getitem_i64	5.7%	1.505s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
                              rt_compare_le	2.9%	0.060s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f900
                                py::RtValue::flatten	2.7%	0.335s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
                                  py::py_true	0.6%	0.163s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                                  __gmpz_fits_slong_p	0.3%	0.080s	test	__gmpz_fits_slong_p	[Unknown]	0x313200
                                  __gmpz_get_si	0.3%	0.080s	test	__gmpz_get_si	[Unknown]	0x313290
                                  py::PyInteger::as_big_int	0.1%	0.036s	test	py::PyInteger::as_big_int(void) const	PyInteger.cpp	0xb88f0
                                  py::py_false	0.1%	0.024s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              rt_is_true_fast	2.5%	0.231s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
                                py::RtValue::flatten	1.6%	0.092s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
                                  py::py_true	1.0%	0.263s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                                  py::py_false	0.3%	0.080s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              rt_binary_mul	1.7%	0.465s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44490
                              rt_binary_mod	1.5%	0.386s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44720
                              rt_getattr_ic	0.9%	0.216s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
                              rt_compare_eq	0.5%	0.143s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f7b0
                              rt_value_array_get	0.5%	0.140s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x41310
                              rt_binary_add	0.5%	0.136s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x442f0
                              py::py_true	0.4%	0.116s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                              py::RtValue::from_int_or_box	0.4%	0.104s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x476a0
                              py::py_false	0.4%	0.096s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              rt_integer_from_i64	0.2%	0.056s	test	rt_integer_from_i64(long)	rt_create.cpp	0x402e0
                              rt_setitem_fast	0.2%	0.032s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x432e0
                              rt_unary_not	0.2%	0s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x44dd0
                            test.<module>.0:0.Sieve.8:0.step2.38:4	13.3%	0.407s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d630
                              rt_list_getitem_i64	2.8%	0.750s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
                              rt_is_true_fast	2.4%	0.140s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
                                py::RtValue::flatten	1.8%	0.143s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
                                  py::py_true	1.0%	0.279s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                                  py::py_false	0.2%	0.064s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              rt_compare_le	2.0%	0.060s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f900
                                py::RtValue::flatten	1.8%	0.191s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
                              rt_binary_mul	1.7%	0.447s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44490
                              rt_binary_mod	0.7%	0.199s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44720
                              rt_getattr_ic	0.5%	0.124s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
                              rt_value_array_get	0.4%	0.104s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x41310
                              rt_integer_from_i64	0.2%	0.056s	test	rt_integer_from_i64(long)	rt_create.cpp	0x402e0
                              py::py_true	0.2%	0.056s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                              rt_unary_not	0.2%	0.020s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x44dd0
                              rt_compare_eq	0.2%	0.044s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f7b0
                              py::RtValue::from_int_or_box	0.1%	0.040s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x476a0
                              py::py_false	0.1%	0.032s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              rt_setitem_fast	0.1%	0.028s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x432e0
                              rt_binary_add	0.1%	0.020s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x442f0
                            test.<module>.0:0.Sieve.8:0.step3.43:4	11.8%	0.311s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d770
                              rt_is_true_fast	2.6%	0.247s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
                                py::RtValue::flatten	1.7%	0.112s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
                                  py::py_true	0.8%	0.219s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                                  py::py_false	0.4%	0.119s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              rt_binary_mul	1.6%	0.415s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44490
                              rt_list_getitem_i64	1.5%	0.403s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
                              rt_compare_le	1.3%	0.120s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f900
                              rt_compare_gt	0.7%	0.188s	test	rt_compare_gt(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f970
                              rt_getattr_ic	0.5%	0.112s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
                              rt_binary_mod	0.4%	0.120s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44720
                              rt_binary_sub	0.4%	0.116s	test	rt_binary_sub(py::PyObject*, py::PyObject*)	rt_op.cpp	0x443c0
                              py::RtValue::from_int_or_box	0.4%	0.107s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x476a0
                              rt_value_array_get	0.3%	0.084s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x41310
                              py::py_false	0.3%	0.076s	test	py::py_false(void)	PyBool.cpp	0x99fc0
                              py::py_true	0.2%	0.064s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                              rt_integer_from_i64	0.1%	0.028s	test	rt_integer_from_i64(long)	rt_create.cpp	0x402e0
                              rt_setitem_fast	0.1%	0.012s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x432e0
                              rt_compare_eq	0.1%	0.016s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f7b0
                              rt_unary_not	0.0%	0s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x44dd0
                            py::py_none	1.5%	0.402s	test	py::py_none(void)	PyNone.cpp	0xd3d20
                            py::types::dict	1.1%	0.299s	test	py::types::dict(void)	builtin.cpp	0x82150
                            py::types::native_function	1.1%	0.295s	test	py::types::native_function(void)	builtin.cpp	0x867a0
                            __memmove_avx_unaligned_erms	1.0%	0.275s	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
                            py::types::list	0.8%	0.207s	test	py::types::list(void)	builtin.cpp	0x837d0
                            func@0x38790	0.3%	0.092s	test	func@0x38790	[Unknown]	0x38790
                            py::PyType::global_version	0.3%	0.088s	test	py::PyType::global_version(void)	atomic_base.h	0x1011b0
                            rt_none	0.1%	0.016s	test	rt_none(void)	rt_singleton.cpp	0x45870
                          rt_compare_lt_bool	2.4%	0.068s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x41e50
                          rt_getattr_ic	1.0%	0.272s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
                          rt_inplace_add	0.7%	0.192s	test	rt_inplace_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44e70
                          rt_binary_mul	0.2%	0.060s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44490
                        py::types::list	0.0%	0.008s	test	py::types::list(void)	builtin.cpp	0x837d0
                      rt_call_raw_ptrs	0.1%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x41570
                    test.<module>.0:0.Sieve.8:0.omit_squares.22:4	0.7%	0.008s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x3d340
                test.<module>.0:0.Sieve.8:0.to_list.14:4	6.2%	0.080s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3d150
                py::PyList::sort	0.1%	0s	test	py::PyList::sort(py::PyTuple*, py::PyDict*)	PyList.cpp	0xbf4b0
                __strcmp_avx2	0.0%	0.012s	libc.so.6	__strcmp_avx2	strcmp-avx2.S	0x18b010
              rt_call_raw_ptrs	27.5%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x41570
                py::PyNativeFunction::call_fast_ptrs	23.5%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0xb3720
                  test.<module>.0:0.generate_trie.70:0	23.3%	0.064s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3dca0
                    rt_call_raw_ptrs	8.3%	0.008s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x41570
                      py::PyType::call_fast_ptrs	8.3%	0.068s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0xd83f0
                        py::PyObject::init_fast_ptrs	3.6%	0s	test	py::PyObject::init_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0xd8e60
                          py::PyNativeFunction::call_fast_ptrs	3.6%	0.024s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0xb3720
                            test.<module>.0:0.Node.3:0.__init__.4:4	3.5%	0.020s	test	test.<module>.0:0.Node.3:0.__init__.4:4	[Unknown]	0x3ce10
                              rt_build_dict	3.1%	0.008s	test	rt_build_dict(int, py::PyObject**, py::PyObject**)	rt_create.cpp	0x407f0
                                py::PyDict::create	3.0%	0.012s	test	py::PyDict::create(void)	PyDict.cpp	0xa87f0
                              rt_setattr_ic	0.3%	0.056s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x469c0
                              rt_false	0.0%	0.012s	test	rt_false(void)	rt_singleton.cpp	0x45890
                        py::PyString::create_raw	2.9%	0s	test	py::PyString::create_raw(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0xecc90
                          GC_register_finalizer_inner	1.7%	0.203s	test	GC_register_finalizer_inner	[Unknown]	0x3c1b50
                          GC_malloc_kind	0.5%	0s	test	GC_malloc_kind	[Unknown]	0x3cbae0
                          _pylang_debug_log_alloc	0.4%	0.012s	test	_pylang_debug_log_alloc(char const*, std::atomic<unsigned long>&)	compat.hpp	0x1185d0
                          ___pthread_mutex_unlock	0.2%	0.048s	libc.so.6	___pthread_mutex_unlock	pthread_mutex_unlock.c	0xa1a70
                          py::PyString::PyString	0.1%	0.012s	test	py::PyString::PyString(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0xec5a0
                        py::PyType::heap_object_allocation	0.9%	0.008s	test	py::PyType::heap_object_allocation(py::PyType*)	PyType.cpp	0x113580
                        std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve	0.5%	0.020s	test	std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve(unsigned long)	vector.tcc	0x20afb0
                        py::types::integer	0.1%	0.016s	test	py::types::integer(void)	builtin.cpp	0x81070
                        func@0x384b0	0.0%	0.012s	test	func@0x384b0	[Unknown]	0x384b0
                        py::PyType::issubclass	0.0%	0.012s	test	py::PyType::issubclass(py::PyType const*) const	PyType.cpp	0x10edc0
                        py::types::type	0.0%	0.012s	test	py::types::type(void)	builtin.cpp	0x7f000
                    rt_load_global	5.0%	0.088s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x3f260
                      py::PyModule::find_symbol_cstr	2.9%	0.112s	test	py::PyModule::find_symbol_cstr(char const*) const	PyModule.cpp	0xd2400
                        py::PyString::intern	2.4%	0.068s	test	py::PyString::intern(char const*)	PyString.cpp	0xec060
                          std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find	1.0%	0.259s	test	std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find(std::basic_string_view<char, std::char_traits<char>> const&)	hashtable.h	0x221f40
                          ___pthread_mutex_unlock	0.6%	0.148s	libc.so.6	___pthread_mutex_unlock	pthread_mutex_unlock.c	0xa1a70
                          ___pthread_mutex_lock	0.5%	0.143s	libc.so.6	___pthread_mutex_lock	pthread_mutex_lock.c	0x9fff0
                          __strlen_avx2	0.0%	0.008s	libc.so.6	__strlen_avx2	strlen-avx2.S	0x18b7c0
                        py::PyObject::from<py::RtValue>	0.1%	0s	test	py::PyObject::from<py::RtValue>(py::RtValue const&)	PyObject.cpp	0xdb1d0
                      py::ModuleRegistry::find	1.6%	0.052s	test	py::ModuleRegistry::find(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>> const&) const	ModuleRegistry.cpp	0x97940
                      py::RtValue::box	0.1%	0.024s	test	py::RtValue::box(void) const	RtValue.cpp	0x474f0
                      py::ModuleRegistry::instance	0.1%	0.016s	test	py::ModuleRegistry::instance(void)	ModuleRegistry.cpp	0x97650
                    rt_setitem_fast	3.2%	0.048s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x432e0
                    rt_list_getitem_i64	2.1%	0.056s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
                    rt_iter_next	2.0%	0.008s	test	rt_iter_next(py::PyObject*, bool*)	rt_subscr.cpp	0x45910
                    rt_compare_not_in_bool	1.7%	0.378s	test	rt_compare_not_in_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42db0
                    rt_getattr_ic	0.7%	0.140s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
                    py::py_true	0.1%	0.020s	test	py::py_true(void)	PyBool.cpp	0x99dd0
                    rt_get_iter	0.1%	0s	test	rt_get_iter(py::PyObject*)	rt_subscr.cpp	0x458c0
                    rt_setattr_ic	0.0%	0.008s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x469c0
                  [Unknown stack frame(s)]	0.2%	0s	[Unknown]	[Unknown stack frame(s)]	[Unknown]	0
                py::PyType::call_fast_ptrs	4.0%	0s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0xd83f0
              rt_list_insert_0_tuple2	1.2%	0s	test	rt_list_insert_0_tuple2(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42a50
              rt_binary_add	0.7%	0.012s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x442f0
              rt_dict_items_iter_for_loop	0.3%	0.028s	test	rt_dict_items_iter_for_loop(py::PyObject*)	rt_subscr.cpp	0x465b0
              rt_load_global	0.2%	0s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x3f260
              rt_getattr_ic	0.1%	0.036s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
              rt_iter_next_unpack2	0.1%	0s	test	rt_iter_next_unpack2(py::PyObject*, py::PyObject**, py::PyObject**)	rt_subscr.cpp	0x45aa0
            [Unknown stack frame(s)]	0.1%	0s	[Unknown]	[Unknown stack frame(s)]	[Unknown]	0
  rt_init	2.2%	0s	test	rt_init(void)	rt_lifecycle.cpp	0x43520

Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
rt_list_getitem_i64	2.714s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	1.505s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3d4b0
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.750s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d630
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.403s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d770
  ↖ test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.056s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3dca0
py::py_true	1.554s	test	py::py_true(void)	PyBool.cpp	0x99dd0
  py::RtValue::flatten	1.255s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
    rt_is_true_fast	0.761s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
    rt_compare_le	0.355s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f900
    ↖ rt_compare_lt_bool ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.116s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x41e50
    ↖ rt_unary_not ← test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.012s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x44dd0
    ↖ rt_list_getitem_i64 ← test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.012s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.116s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3d4b0
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.064s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d770
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.056s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d630
  ↖ rt_list_getitem_i64_truthy ← test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.044s	test	rt_list_getitem_i64_truthy(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42280
  ↖ test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.020s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3dca0
rt_binary_mul	1.399s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44490
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.465s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3d4b0
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.447s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d630
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.415s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d770
  ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.060s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3d8d0
  ↖ test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.012s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x3d340
py::RtValue::flatten	1.333s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
  rt_compare_le	0.623s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x3f900
  rt_is_true_fast	0.347s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
  rt_compare_lt_bool	0.287s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x41e50
  rt_unary_not	0.048s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x44dd0
  ↖ rt_list_getitem_i64 ← test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.028s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
rt_getattr_ic	1.078s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
rt_call_method_ic_ptrs	0.806s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x46d40
py::py_false	0.734s	test	py::py_false(void)	PyBool.cpp	0x99fc0
rt_binary_mod	0.705s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44720
__memset_avx2_unaligned_erms	0.697s	libc.so.6	__memset_avx2_unaligned_erms	memset-vec-unaligned-erms.S	0x189480
rt_is_true_fast	0.618s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
___pthread_mutex_unlock	0.523s	libc.so.6	___pthread_mutex_unlock	pthread_mutex_unlock.c	0xa1a70
rt_list_getitem_i64_truthy	0.486s	test	rt_list_getitem_i64_truthy(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42280
std::vector<py::RtValue, std::allocator<py::RtValue>>::_M_range_insert<__gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>>	0.482s	test	std::vector<py::RtValue, std::allocator<py::RtValue>>::_M_range_insert<__gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>>(__gnu_cxx::__normal_iterator<py::RtValue*, std::vector<py::RtValue, std::allocator<py::RtValue>>>, __gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>, __gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>, std::forward_iterator_tag)	vector.tcc	0x1c1a70
  ↖ py::PyList::__mul__ ← py::PySequenceWrapper::repeat ← py::PyObject::multiply ← rt_binary_mul ← test.<module>.0:0.Sieve.8:0.__init__.9:4 ← py::PyNativeFunction::call_fast_ptrs ← py::PyObject::init_fast_ptrs ← py::PyType::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← main ← __libc_start_main_impl ← _start	0.482s	test	py::PyList::__mul__(unsigned long) const	PyList.cpp	0xbede0
std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find	0.474s	test	std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find(std::basic_string_view<char, std::char_traits<char>> const&)	hashtable.h	0x221f40
py::py_none	0.414s	test	py::py_none(void)	PyNone.cpp	0xd3d20
test.<module>.0:0.Sieve.8:0.step2.38:4	0.407s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d630
rt_compare_not_in_bool	0.378s	test	rt_compare_not_in_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42db0
GC_mark_from	0.376s	test	GC_mark_from	[Unknown]	0x3bec00
___pthread_mutex_lock	0.375s	libc.so.6	___pthread_mutex_lock	pthread_mutex_lock.c	0x9fff0
___pthread_mutex_trylock	0.367s	libc.so.6	___pthread_mutex_trylock	pthread_mutex_trylock.c	0xa0ef0
py::types::dict	0.353s	test	py::types::dict(void)	builtin.cpp	0x82150