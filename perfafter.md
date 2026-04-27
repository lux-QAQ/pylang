Top Memory-Consuming Functions
    Function	Memory Consumption	Allocation/Deallocation Delta	Allocations	Module
    GC_unix_get_mem	2.0 GB 	2.0 GB 	2,044	test
    py::PyList::__mul__	733.2 MB 	0.0 B 	28	test
    _dwarf_get_alloc	33.9 MB 	9.1 MB 	518,990	test
    elf_load_nolibelf_section	31.5 MB 	31.5 MB 	21	test
    dwarf_rnglists_get_rle_head	26.3 MB 	0.0 B 	423,844	test
    [Others]	68.8 MB 	42.0 MB 	346,894	N/A*


Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
rt_list_getitem_i64	2.609s	test	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42090
py::py_true	1.633s	test	py::py_true(void)	PyBool.cpp	0x99dd0
rt_binary_mul	1.284s	test	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44490
py::RtValue::flatten	1.276s	test	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x475c0
py::py_false	0.832s	test	py::py_false(void)	PyBool.cpp	0x99fc0
rt_call_method_ic_ptrs	0.832s	test	rt_call_method_ic_ptrs(py::cache::MethodCache*, py::PyObject*, char const*, py::PyObject**, int, py::PyObject*)	rt_method_cache.cpp	0x46d40
rt_getattr_ic	0.830s	test	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x466a0
rt_binary_mod	0.631s	test	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x44720
rt_is_true_fast	0.555s	test	rt_is_true_fast(py::PyObject*)	rt_fused.cpp	0x42000
std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find	0.543s	test	std::_Hashtable<std::basic_string_view<char, std::char_traits<char>>, std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char>> const, py::PyString*>>, std::__detail::_Select1st, std::equal_to<std::basic_string_view<char, std::char_traits<char>>>, std::hash<std::basic_string_view<char, std::char_traits<char>>>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<(bool)1, (bool)0, (bool)1>>::find(std::basic_string_view<char, std::char_traits<char>> const&)	hashtable.h	0x221f40
__memset_avx2_unaligned_erms	0.522s	libc.so.6	__memset_avx2_unaligned_erms	memset-vec-unaligned-erms.S	0x189480
__memmove_avx_unaligned_erms	0.506s	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
GC_mark_from	0.482s	test	GC_mark_from	[Unknown]	0x3bec00
py::types::dict	0.471s	test	py::types::dict(void)	builtin.cpp	0x82150
__gmpz_fits_slong_p	0.464s	test	__gmpz_fits_slong_p	[Unknown]	0x313200
std::vector<py::RtValue, std::allocator<py::RtValue>>::_M_range_insert<__gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>>	0.438s	test	std::vector<py::RtValue, std::allocator<py::RtValue>>::_M_range_insert<__gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>>(__gnu_cxx::__normal_iterator<py::RtValue*, std::vector<py::RtValue, std::allocator<py::RtValue>>>, __gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>, __gnu_cxx::__normal_iterator<py::RtValue const*, std::vector<py::RtValue, py::GCTracingAllocator<py::RtValue>>>, std::forward_iterator_tag)	vector.tcc	0x1c1a70
test.<module>.0:0.Sieve.8:0.step3.43:4	0.437s	test	test.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d770
___pthread_mutex_trylock	0.425s	libc.so.6	___pthread_mutex_trylock	pthread_mutex_trylock.c	0xa0ef0
___pthread_mutex_unlock	0.422s	libc.so.6	___pthread_mutex_unlock	pthread_mutex_unlock.c	0xa1a70
rt_list_getitem_i64_truthy	0.350s	test	rt_list_getitem_i64_truthy(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x42280
py::py_none	0.331s	test	py::py_none(void)	PyNone.cpp	0xd3d20
___pthread_mutex_lock	0.327s	libc.so.6	___pthread_mutex_lock	pthread_mutex_lock.c	0x9fff0


下面是test6性能测试:
注释之前:
Top Hotspots
    Function	Module	CPU Time	% of CPU Time
    __memset_avx2_unaligned_erms	libc.so.6	0.266s	12.3%
    rt_list_getitem_i64	test6	0.187s	8.6%
    ___pthread_mutex_unlock	libc.so.6	0.084s	3.9%
    GC_build_fl	test6	0.079s	3.6%
    GC_mark_from	test6	0.076s	3.5%
    [Others]	N/A*	1.468s	68.0%


Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
__memset_avx2_unaligned_erms	266.016ms	libc.so.6	__memset_avx2_unaligned_erms	memset-vec-unaligned-erms.S	0x189480
  GC_build_fl	178.877ms	test6	GC_build_fl	[Unknown]	0x3c3040
    ↖ GC_new_hblk ← GC_allocobj ← GC_generic_malloc_inner ← GC_register_finalizer_inner	142.225ms	test6	GC_new_hblk	[Unknown]	0x3c3c60
      ↖ rt_init ← main ← __libc_start_main_impl ← _start	119.475ms	test6	rt_init(void)	rt_lifecycle.cpp	0x42eb0
      ↖ py::PyString::create_raw ← py::PyType::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.generate_trie.71:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	22.750ms	test6	py::PyString::create_raw(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0xec620
    ↖ GC_generic_malloc_many ← GC_malloc_kind ← py::Arena::allocate<py::PyDict> ← py::PyDict::create ← rt_build_dict ← test6.<module>.0:0.Node.3:0.__init__.4:4 ← py::PyNativeFunction::call_fast_ptrs ← py::PyObject::init_fast_ptrs ← py::PyType::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.generate_trie.71:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	36.652ms	test6	GC_generic_malloc_many	[Unknown]	0x3c6df0
  ↖ GC_generic_malloc_inner_ignore_off_page ← GC_grow_table ← GC_register_finalizer_inner ← rt_init ← main ← __libc_start_main_impl ← _start	55.700ms	test6	GC_generic_malloc_inner_ignore_off_page	[Unknown]	0x3c5bc0
  ↖ GC_generic_malloc ← GC_malloc_kind_global	31.438ms	test6	GC_generic_malloc	[Unknown]	0x3c5c60
rt_list_getitem_i64	186.563ms	test6	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x41a20
  ↖ test6.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	133.267ms	test6	test6.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3d5b0
  ↖ test6.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	30.454ms	test6	test6.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d730
  ↖ test6.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	22.843ms	test6	test6.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d870
___pthread_mutex_unlock	83.880ms	libc.so.6	___pthread_mutex_unlock	pthread_mutex_unlock.c	0xa1a70
GC_build_fl	78.620ms	test6	GC_build_fl	[Unknown]	0x3c3040
  ↖ GC_generic_malloc_many ← GC_malloc_kind	78.620ms	test6	GC_generic_malloc_many	[Unknown]	0x3c6df0
GC_mark_from	76.447ms	test6	GC_mark_from	[Unknown]	0x3be580
  GC_mark_local	76.447ms	test6	GC_mark_local	[Unknown]	0x3bf160
GC_register_finalizer_inner	76.097ms	test6	GC_register_finalizer_inner	[Unknown]	0x3c14d0
  ↖ rt_init ← main ← __libc_start_main_impl ← _start	76.097ms	test6	rt_init(void)	rt_lifecycle.cpp	0x42eb0
rt_binary_mul	68.522ms	test6	rt_binary_mul(py::PyObject*, py::PyObject*)	rt_op.cpp	0x43e20
py::RtValue::flatten	68.516ms	test6	py::RtValue::flatten(py::PyObject*)	RtValue.cpp	0x46f50
___pthread_mutex_trylock	56.994ms	libc.so.6	___pthread_mutex_trylock	pthread_mutex_trylock.c	0xa0ef0
__memmove_avx_unaligned_erms	53.308ms	libc.so.6	__memmove_avx_unaligned_erms	memmove-vec-unaligned-erms.S	0x188a80
py::py_false	49.562ms	test6	py::py_false(void)	PyBool.cpp	0x99950
rt_getattr_ic	41.981ms	test6	rt_getattr_ic(py::cache::AttrCache*, py::PyObject*, py::PyObject*)	rt_attr_cache.cpp	0x46030
GC_grow_table	38.055ms	test6	GC_grow_table	[Unknown]	0x3c2e80
rt_binary_mod	38.042ms	test6	rt_binary_mod(py::PyObject*, py::PyObject*)	rt_op.cpp	0x440b0
GC_finalize	36.626ms	test6	GC_finalize	[Unknown]	0x3c1e30
__gmpz_fits_slong_p	34.261ms	test6	__gmpz_fits_slong_p	[Unknown]	0x312b90

注释之后:


Function / Call Stack	CPU Time	Module	Function (Full)	Source File	Start Address
GC_mark_from	198.465ms	test6	GC_mark_from	[Unknown]	0x3be580
  GC_mark_local	198.465ms	test6	GC_mark_local	[Unknown]	0x3bf160
    ↖ GC_help_marker ← GC_mark_thread ← start_thread ← __clone3	187.549ms	test6	GC_help_marker	[Unknown]	0x3bf100
    ↖ GC_mark_some ← GC_stopped_mark ← GC_try_to_collect_inner ← GC_grow_table ← GC_register_finalizer_inner ← rt_init ← main ← __libc_start_main_impl ← _start	10.916ms	test6	GC_mark_some	[Unknown]	0x3bd850
__memset_avx2_unaligned_erms	193.793ms	libc.so.6	__memset_avx2_unaligned_erms	memset-vec-unaligned-erms.S	0x189480
  GC_build_fl	132.805ms	test6	GC_build_fl	[Unknown]	0x3c3040
    ↖ GC_new_hblk ← GC_allocobj ← GC_generic_malloc_inner ← GC_register_finalizer_inner	110.251ms	test6	GC_new_hblk	[Unknown]	0x3c3c60
      ↖ rt_init ← main ← __libc_start_main_impl ← _start	76.055ms	test6	rt_init(void)	rt_lifecycle.cpp	0x42eb0
      ↖ py::PyString::create_raw ← py::PyType::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.generate_trie.71:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	34.197ms	test6	py::PyString::create_raw(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&&)	PyString.cpp	0xec620
    ↖ GC_generic_malloc_many ← GC_malloc_kind ← py::Arena::allocate<py::PyDict> ← py::PyDict::create ← rt_build_dict ← test6.<module>.0:0.Node.3:0.__init__.4:4 ← py::PyNativeFunction::call_fast_ptrs ← py::PyObject::init_fast_ptrs ← py::PyType::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.generate_trie.71:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	22.554ms	test6	GC_generic_malloc_many	[Unknown]	0x3c6df0
  ↖ GC_generic_malloc_inner_ignore_off_page ← GC_grow_table ← GC_register_finalizer_inner ← rt_init ← main ← __libc_start_main_impl ← _start	31.979ms	test6	GC_generic_malloc_inner_ignore_off_page	[Unknown]	0x3c5bc0
  ↖ GC_generic_malloc ← GC_malloc_kind_global	21.409ms	test6	GC_generic_malloc	[Unknown]	0x3c5c60
  ↖ GC_free ← std::vector<std::pair<py::RtValue, py::RtValue>, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>>::_M_realloc_insert<std::piecewise_construct_t const&, std::tuple<py::RtValue const&>, std::tuple<py::RtValue const&>> ← tsl::detail_ordered_hash::ordered_hash<std::pair<py::RtValue, py::RtValue>, tsl::ordered_map<py::RtValue, py::RtValue, py::RtValueHash, py::RtValueEq, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>, std::vector<std::pair<py::RtValue, py::RtValue>, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>>, unsigned int>::KeySelect, tsl::ordered_map<py::RtValue, py::RtValue, py::RtValueHash, py::RtValueEq, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>, std::vector<std::pair<py::RtValue, py::RtValue>, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>>, unsigned int>::ValueSelect, py::RtValueHash, py::RtValueEq, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>, std::vector<std::pair<py::RtValue, py::RtValue>, py::GCTracingAllocator<std::pair<py::RtValue, py::RtValue>>>, unsigned int>::insert_impl<py::RtValue, std::piecewise_construct_t const&, std::tuple<py::RtValue const&>, std::tuple<py::RtValue const&>> ...	7.600ms	test6	GC_free	[Unknown]	0x3c60f0
rt_list_getitem_i64	159.265ms	test6	rt_list_getitem_i64(py::PyObject*, py::PyObject*)	rt_fused.cpp	0x41a20
  ↖ test6.<module>.0:0.Sieve.8:0.step1.33:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	68.382ms	test6	test6.<module>.0:0.Sieve.8:0.step1.33:4	[Unknown]	0x3d5b0
  ↖ test6.<module>.0:0.Sieve.8:0.step2.38:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	52.855ms	test6	test6.<module>.0:0.Sieve.8:0.step2.38:4	[Unknown]	0x3d730
  ↖ test6.<module>.0:0.Sieve.8:0.step3.43:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_y.48:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.loop_x.56:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.Sieve.8:0.calc.64:4 ← rt_call_method_ic_ptrs ← test6.<module>.0:0.find.86:0 ← py::PyNativeFunction::call_fast_ptrs ← rt_call_raw_ptrs ← PyInit_test6 ← main ← __libc_start_main_impl ← _start	38.028ms	test6	test6.<module>.0:0.Sieve.8:0.step3.43:4	[Unknown]	0x3d870
___pthread_mutex_unlock	89.801ms	libc.so.6	___pthread_mutex_unlock	pthread_mutex_unlock.c	0xa1a70


Top Memory-Consuming Functions
    Function	Memory Consumption	Allocation/Deallocation Delta	Allocations	Module
    GC_unix_get_mem	1.9 GB 	1.9 GB 	851	test6
    py::PyList::__mul__	40.0 MB 	0.0 B 	2	test6
    elf_load_nolibelf_section	31.5 MB 	31.5 MB 	21	test6
    _dwarf_get_alloc	22.8 MB 	7.8 MB 	363,126	test6
    _dwarf_get_abbrev_for_code	11.2 MB 	10.8 MB 	106,742	test6
    [Others]	33.9 MB 	23.6 MB 	79,463	N/A*
