Top Hotspots
    Function	Module	CPU Time	% of CPU Time
    py::RtValue::flatten	test	6.181s	12.4%
    __memmove_avx_unaligned_erms	libc.so.6	4.059s	8.1%
    rt_list_getitem_i64	test	3.281s	6.6%
    rt_call_method_ic_ptrs	test	3.022s	6.0%
    rt_binary_mul	test	1.919s	3.8%
    [Others]	N/A*	31.555s	63.1%


Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
py::RtValue::flatten	6.181s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x56c40
  rt_binary_mul	2.026s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e4b0
    ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.679s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
    ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.604s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
    ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.486s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
    ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.238s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3ead0
    ↖ test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.011s	test	test.<module>.0:0.Sieve.8:0.loop_x.56:4	[Unknown]	0x3ec60
    ↖ test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.008s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x3e350
  rt_is_true_fast	1.497s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x48230
    ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.458s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
    ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.452s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
    ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.403s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
    ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.172s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
    ↖ test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.011s	test	test.<module>.0:0.find.84:0	[Unknown]	0x3f220
  rt_compare_le	0.653s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e10
    ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.247s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
    ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.228s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
    ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.178s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
  rt_binary_mod	0.365s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e950
    ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.206s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
    ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.136s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
    ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.023s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
  rt_compare_lt_bool	0.326s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x48030
    ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.306s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3ead0
    ↖ test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.019s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x3e350
  ↖ rt_compare_gt ← test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.272s	test	rt_compare_gt(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e40
  ↖ rt_binary_sub ← test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.260s	test	rt_binary_sub(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e320
  rt_list_getitem_i64	0.206s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
  ↖ rt_inplace_add ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.205s	test	rt_inplace_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4f780
  rt_binary_add	0.199s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e190
  rt_compare_eq	0.121s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41d80
  rt_unary_not	0.039s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4f740
  ↖ rt_setitem_fast ← test.<module>.0:0.Sieve.8:0.omit_squares.22:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.012s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x4aa00
__memmove_avx_unaligned_erms	4.059s	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
  ↖ rt_list_insert_0_tuple2 ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	3.908s	test	rt_list_insert_0_tuple2(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x49290
  ↖ rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.132s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
  ↖ py::PyObject::init_fast_ptrs ← py::PyType::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.011s	test	py::PyObject::init_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x195b10
  ↖ _dwarf_memcpy_noswap_bytes ← _dwarf_extract_address_from_debug_addr ← _dwarf_look_in_local_and_tied_by_index ← build_array_of_rle ← _dwarf_fill_in_rle_head ← dwarf_rnglists_get_rle_head ← _ZNK8cpptrace6detail8libdwarf10die_object4wrapIJP17Dwarf_Attribute_styPP21Dwarf_Rnglists_Head_sPyS9_PP13Dwarf_Error_sEJRS5_RtRyS8_S9_S9_ETnNSt9enable_ifIXsr3std7is_sameIDTcvvclclsr3stdE7declvalIFiDpT_EEEspclsr3stdE7forwardIT0_Eclsr3stdE7declvalISK_EEELDnEEEvEE5valueEiE4typeELi0EEEiPSJ_DpOSK_ ← cpptrace::detail::libdwarf::die_object::dwarf5_ranges<cpptrace::detail::libdwarf::die_object::pc_in_die(cpptrace::detail::libdwarf::die_object const&, int, unsigned long long) constconst::{lambda(unsigned long longunsigned long long)#1}> ← cpptrace::detail::libdwarf::die_object::dwarf_ranges<cpptrace::detail::libdwarf::die_object::pc_in_die(cpptrace::detail::libdwarf::die_object const&, int, unsigned long long) constconst::{lambda(unsigned long longunsigned long long)#1}> ← cpptrace::detail::libdwarf::die_object::pc_in_die ...	0.008s	test	_dwarf_memcpy_noswap_bytes	dwarf_memcpy_swap.c	0x54bff0
rt_list_getitem_i64	3.281s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
  ↖ test.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	1.708s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
  ↖ test.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.676s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
  ↖ test.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.420s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
  ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.334s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
  ↖ test.<module>.0:0.generate_trie.70:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.144s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3eed0
rt_call_method_ic_ptrs	3.022s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
  ↖ test.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	2.932s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3ead0
  ↖ test.<module>.0:0.Sieve.8:0.to_list.14:4 ← rt_call_method_ic_ptrs ← test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.058s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
  ↖ test.<module>.0:0.find.84:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test.<module>.0:0.run_stress_test.141:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test ← main ← __libc_start_main_impl ← _start	0.031s	test	test.<module>.0:0.find.84:0	[Unknown]	0x3f220


Function Stack	CPU Time: Total	CPU Time: Self	Module	Function (Full)	Source File	Start Address
main	98.6%	0s	test	main	[Unknown]	0x40cd0
  PyInit_test	96.8%	0s	test	PyInit_test	[Unknown]	0x3d9d0
    rt_call_raw_ptrs	96.8%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x467a0
      py::PyNativeFunction::call_fast_ptrs	96.8%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x141560
        test.<module>.0:0.run_stress_test.141:0	96.8%	0s	test	test.<module>.0:0.run_stress_test.141:0	[Unknown]	0x3fe90
          rt_call_raw_ptrs	96.8%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x467a0
            py::PyNativeFunction::call_fast_ptrs	96.8%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x141560
              test.<module>.0:0.find.84:0	96.8%	0s	test	test.<module>.0:0.find.84:0	[Unknown]	0x3f220
                rt_call_method_ic_ptrs	55.8%	0.031s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
                  test.<module>.0:0.Sieve.8:0.calc.64:4	50.0%	0s	test	test.<module>.0:0.Sieve.8:0.calc.64:4	[Unknown]	0x3edd0
                    rt_call_method_ic_ptrs	50.0%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
                      test.<module>.0:0.Sieve.8:0.loop_x.56:4	49.3%	0s	test	test.<module>.0:0.Sieve.8:0.loop_x.56:4	[Unknown]	0x3ec60
                        rt_call_method_ic_ptrs	49.1%	0s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
                          test.<module>.0:0.Sieve.8:0.loop_y.48:4	49.1%	0.139s	test	test.<module>.0:0.Sieve.8:0.loop_y.48:4	[Unknown]	0x3ead0
                            rt_call_method_ic_ptrs	44.4%	2.932s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
                              test.<module>.0:0.Sieve.8:0.step1.33:4	15.3%	0.236s	test	test.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3e4f0
                                rt_list_getitem_i64	3.5%	1.708s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
                                rt_is_true_fast	2.6%	0.287s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x48230
                                rt_binary_mul	2.3%	0.539s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e4b0
                                rt_binary_mod	1.4%	0.470s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e950
                                rt_getattr_ic	1.0%	0.298s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                                rt_compare_le	1.0%	0.023s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e10
                                rt_binary_add	0.7%	0.280s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e190
                                py::RtValue::from_int_or_box	0.7%	0.338s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x56d50
                                py::RtValue::compare_le	0.4%	0.136s	test	py::RtValue::compare_le(py::RtValue, py::RtValue)	RtValue.cpp	0x58e30
                                rt_setitem_fast	0.3%	0.066s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x4aa00
                                py::RtValue::compare_eq	0.2%	0.074s	test	py::RtValue::compare_eq(py::RtValue, py::RtValue)	RtValue.cpp	0x585d0
                                rt_unary_not	0.2%	0.031s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4f740
                                rt_value_array_get	0.2%	0.096s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x45fe0
                                rt_compare_eq	0.2%	0.035s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41d80
                                rt_none	0.1%	0.062s	test	rt_none(void)	rt_singleton.cpp	0x51180
                                rt_integer_from_i64	0.1%	0.032s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43740
                              test.<module>.0:0.Sieve.8:0.step2.38:4	10.8%	0.152s	test	test.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3e720
                                rt_binary_mul	2.6%	0.635s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e4b0
                                  py::RtValue::flatten	1.4%	0.679s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x56c40
                                rt_is_true_fast	1.9%	0.164s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x48230
                                  py::RtValue::flatten	1.5%	0.403s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x56c40
                                    py::types::integer	0.3%	0.155s	test	py::types::integer(void)	builtin.cpp	0xdab60
                                    py::types::bool_	0.3%	0.151s	test	py::types::bool_(void)	builtin.cpp	0xd85b0
                                    py::PyBool::value	0.1%	0.062s	test	py::PyBool::value(void) const	gmpxx.h	0x105410
                                rt_list_getitem_i64	1.4%	0.676s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
                                rt_binary_mod	1.0%	0.343s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e950
                                rt_compare_le	0.8%	0.031s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e10
                                rt_binary_add	0.8%	0.267s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e190
                                rt_getattr_ic	0.6%	0.225s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                                py::RtValue::from_int_or_box	0.5%	0.247s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x56d50
                                py::RtValue::compare_le	0.3%	0.089s	test	py::RtValue::compare_le(py::RtValue, py::RtValue)	RtValue.cpp	0x58e30
                                rt_compare_eq	0.2%	0.024s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41d80
                                rt_value_array_get	0.2%	0.085s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x45fe0
                                py::RtValue::compare_eq	0.1%	0.027s	test	py::RtValue::compare_eq(py::RtValue, py::RtValue)	RtValue.cpp	0x585d0
                                rt_none	0.1%	0.047s	test	rt_none(void)	rt_singleton.cpp	0x51180
                                rt_unary_not	0.1%	0.012s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4f740
                                rt_integer_from_i64	0.1%	0.027s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43740
                                rt_setitem_fast	0.0%	0s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x4aa00
                              test.<module>.0:0.Sieve.8:0.step3.43:4	10.6%	0.185s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3e8e0
                                rt_is_true_fast	2.6%	0.310s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x48230
                                  py::RtValue::flatten	2.0%	0.452s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x56c40
                                rt_binary_mul	1.9%	0.461s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e4b0
                                rt_list_getitem_i64	0.9%	0.420s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
                                rt_binary_sub	0.8%	0.151s	test	rt_binary_sub(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e320
                                rt_compare_gt	0.7%	0.074s	test	rt_compare_gt(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e40
                                rt_getattr_ic	0.7%	0.225s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                                rt_compare_le	0.6%	0.008s	test	rt_compare_le(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41e10
                                py::RtValue::compare_gt	0.5%	0.198s	test	py::RtValue::compare_gt(py::RtValue, py::RtValue)	RtValue.cpp	0x59070
                                py::RtValue::from_int_or_box	0.4%	0.202s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x56d50
                                py::RtValue::compare_le	0.2%	0.073s	test	py::RtValue::compare_le(py::RtValue, py::RtValue)	RtValue.cpp	0x58e30
                                rt_binary_mod	0.2%	0.073s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e950
                                rt_integer_from_i64	0.2%	0.090s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43740
                                rt_value_array_get	0.2%	0.082s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x45fe0
                                rt_unary_not	0.1%	0.012s	test	rt_unary_not(py::PyObject*)	rt_op.cpp	0x4f740
                                rt_setitem_fast	0.1%	0.008s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x4aa00
                                py::RtValue::compare_eq	0.1%	0.035s	test	py::RtValue::compare_eq(py::RtValue, py::RtValue)	RtValue.cpp	0x585d0
                                py::py_none	0.1%	0.027s	test	py::py_none(void)	PyNone.cpp	0x186d50
                                rt_none	0.1%	0.027s	test	rt_none(void)	rt_singleton.cpp	0x51180
                                rt_compare_eq	0.0%	0.012s	test	rt_compare_eq(py::PyObject*, py::PyObject*)	rt_cmp.cpp	0x41d80
                              py::types::list	0.4%	0.221s	test	py::types::list(void)	builtin.cpp	0xde600
                              py::types::dict	0.4%	0.213s	test	py::types::dict(void)	builtin.cpp	0xdc480
                              py::RtValue::box	0.4%	0.190s	test	py::RtValue::box(void) const	RtValue.cpp	0x56b60
                              __memmove_avx_unaligned_erms	0.3%	0.132s	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
                              py::PyType::global_version	0.2%	0.089s	test	py::PyType::global_version(void)	atomic_base.h	0x20ba30
                              func@0x39130	0.1%	0.066s	test	func@0x39130	[Unknown]	0x39130
                              py::types::native_function	0.1%	0.043s	test	py::types::native_function(void)	builtin.cpp	0xe2d30
                            rt_compare_lt_bool	1.5%	0.144s	test	rt_compare_lt_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x48030
                            rt_inplace_add	1.0%	0.093s	test	rt_inplace_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4f780
                            rt_binary_mul	1.0%	0.260s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e4b0
                            rt_getattr_ic	0.7%	0.299s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                            py::RtValue::from_int_or_box	0.2%	0.105s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x56d50
                            rt_integer_from_i64	0.1%	0.031s	test	rt_integer_from_i64(long)	rt_create.cpp	0x43740
                        rt_call_raw_ptrs	0.2%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x467a0
                        rt_binary_mul	0.0%	0s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e4b0
                        rt_binary_mod	0.0%	0.011s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e950
                      test.<module>.0:0.Sieve.8:0.omit_squares.22:4	0.7%	0s	test	test.<module>.0:0.Sieve.8:0.omit_squares.22:4	[Unknown]	0x3e350
                  test.<module>.0:0.Sieve.8:0.to_list.14:4	5.7%	0.151s	test	test.<module>.0:0.Sieve.8:0.to_list.14:4	[Unknown]	0x3e0e0
                    rt_iter_next	2.6%	0.167s	test	rt_iter_next(py::PyObject*, bool*)	rt_subscr.cpp	0x512b0
                      py::PyRangeIterator::next_fast	1.8%	0.222s	test	py::PyRangeIterator::next_fast(void)	PyRange.cpp	0x1c1340
                      py::RtValue::box	0.3%	0.159s	test	py::RtValue::box(void) const	RtValue.cpp	0x56b60
                      py::RtValue::from_int_or_box	0.1%	0.042s	test	py::RtValue::from_int_or_box(long)	RtValue.cpp	0x56d50
                      py::types::range_iterator	0.0%	0.023s	test	py::types::range_iterator(void)	builtin.cpp	0xe0bb0
                    rt_list_getitem_i64	1.1%	0.334s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
                    rt_getattr_ic	0.7%	0.233s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                    rt_is_true_fast	0.7%	0.117s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x48230
                    rt_call_method_ic_ptrs	0.2%	0.058s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x55780
                  py::PyList::sort	0.0%	0s	test	py::PyList::sort(py::PyTuple*, py::PyDict*)	PyList.cpp	0x15cd40
                rt_call_raw_ptrs	30.7%	0s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x467a0
                  py::PyNativeFunction::call_fast_ptrs	27.7%	0s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x141560
                    test.<module>.0:0.generate_trie.70:0	27.7%	0.066s	test	test.<module>.0:0.generate_trie.70:0	[Unknown]	0x3eed0
                      rt_call_raw_ptrs	11.0%	0.054s	test	rt_call_raw_ptrs(py::PyObject*, py::PyObject**, int, py::PyObject*)	rt_func.cpp	0x467a0
                        py::PyType::call_fast_ptrs	10.6%	0.547s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x192ab0
                          py::PyObject::init_fast_ptrs	4.7%	0.078s	test	py::PyObject::init_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x195b10
                            py::PyNativeFunction::call_fast_ptrs	4.6%	0.042s	test	py::PyNativeFunction::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyFunction.cpp	0x141560
                              test.<module>.0:0.Node.3:0.__init__.4:4	4.5%	0.031s	test	test.<module>.0:0.Node.3:0.__init__.4:4	[Unknown]	0x3dce0
                                rt_build_dict	3.3%	0.059s	test	rt_build_dict(int, py::PyObject**, py::PyObject**)	rt_create.cpp	0x44420
                                  py::PyDict::create	3.1%	0.012s	test	py::PyDict::create(void)	PyDict.cpp	0x126df0
                                  _ZNR2py8PyResultIPNS_6PyDictEE6unwrapEv	0.1%	0.039s	test	_ZNR2py8PyResultIPNS_6PyDictEE6unwrapEv	Value.hpp	0x26c870
                                rt_setattr_ic	1.1%	0.339s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x54b00
                                py::py_none	0.0%	0.019s	test	py::py_none(void)	PyNone.cpp	0x186d50
                                rt_value_array_get	0.0%	0.012s	test	rt_value_array_get(py::PyObject**, int)	rt_func.cpp	0x45fe0
                            py::PyType::global_version	0.0%	0.008s	test	py::PyType::global_version(void)	atomic_base.h	0x20ba30
                          py::PyString::create_raw	2.6%	0.012s	test	py::PyString::create_raw(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0x1d4950
                          py::PyType::heap_object_allocation	1.4%	0.031s	test	py::PyType::heap_object_allocation(py::PyType*)	PyType.cpp	0x24aaf0
                          std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve	0.5%	0.039s	test	std::vector<py::PyObject*, py::GCTracingAllocator<py::PyObject*>>::reserve(unsigned long)	vector.tcc	0x3fd0b0
                          _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.2%	0.083s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x2647d0
                          py::PyResult<py::PyObject*>::PyResult<py::PyString*>	0.0%	0.012s	test	py::PyResult<py::PyObject*>::PyResult<py::PyString*>(py::PyResult<py::PyString*> const&)	Value.hpp	0x27a760
                          py::types::str	0.0%	0.012s	test	py::types::str(void)	builtin.cpp	0xd9ed0
                          std::_Function_handler<py::PyResult<py::PyObject*> (py::PyType*), py::PyResult<py::PyObject*> (py::PyType*)*>::_M_invoke	0.0%	0.012s	test	std::_Function_handler<py::PyResult<py::PyObject*> (py::PyType*), py::PyResult<py::PyObject*> (py::PyType*)*>::_M_invoke(std::_Any_data const&, py::PyType*&&)	invoke.h	0x49ebc0
                          py::types::type	0.0%	0.008s	test	py::types::type(void)	builtin.cpp	0xd7d50
                        _ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	0.3%	0.136s	test	_ZNR2py8PyResultIPNS_8PyObjectEE6unwrapEv	Value.hpp	0x2647d0
                        py::RtValue::box	0.0%	0.016s	test	py::RtValue::box(void) const	RtValue.cpp	0x56b60
                      rt_load_global	4.1%	0.330s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x40d70
                      rt_list_getitem_i64	4.0%	0.144s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x483a0
                      rt_setitem_fast	3.1%	0.031s	test	rt_setitem_fast(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x4aa00
                      rt_iter_next	2.4%	0.194s	test	rt_iter_next(py::PyObject*, bool*)	rt_subscr.cpp	0x512b0
                      rt_compare_not_in_bool	1.6%	0.726s	test	rt_compare_not_in_bool(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x49bd0
                      rt_getattr_ic	0.8%	0.341s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                      rt_get_iter	0.3%	0s	test	rt_get_iter(py::PyObject*)	rt_subscr.cpp	0x511d0
                      rt_setattr_ic	0.2%	0.074s	test	rt_setattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x54b00
                  py::PyType::call_fast_ptrs	3.0%	0.028s	test	py::PyType::call_fast_ptrs(py::PyObject**, unsigned long, py::PyDict*)	PyObject.cpp	0x192ab0
                rt_list_insert_0_tuple2	8.5%	0.020s	test	rt_list_insert_0_tuple2(py::PyObject*, py::PyObject*, py::PyObject*)	rt_fused.cpp	0x49290
                rt_binary_add	1.0%	0s	test	rt_binary_add(py::PyObject*, py::PyObject*)	rt_op.cpp	0x4e190
                rt_dict_items_iter_for_loop	0.3%	0.055s	test	rt_dict_items_iter_for_loop(py::PyObject*)	rt_subscr.cpp	0x53720
                rt_is_true_fast	0.2%	0s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x48230
                rt_getattr_ic	0.1%	0.074s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x53ab0
                rt_load_global	0.1%	0.012s	test	rt_load_global(py::PyObject*, char const*)	rt_attr.cpp	0x40d70
                rt_unpack_sequence	0.1%	0s	test	rt_unpack_sequence(py::PyObject*, int, py::PyObject**)	rt_subscr.cpp	0x51830
                rt_iter_next_unpack2	0.0%	0.011s	test	rt_iter_next_unpack2(py::PyObject*, py::PyObject**, py::PyObject**)	rt_subscr.cpp	0x51920
                py::py_none	0.0%	0.012s	test	py::py_none(void)	PyNone.cpp	0x186d50