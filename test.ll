[2026-03-18 21:04:38.304] [info] [timer] precompile_runtime: 36909.03ms
[2026-03-18 21:04:46.135] [info] [timer] SimpleDriver::create: 44739.95ms
[2026-03-18 21:04:46.141] [info] [timer] stage_codegen: 6.24ms
; ModuleID = 'test'
source_filename = "test"

@.str = private unnamed_addr constant [5 x i8] c"test\00", align 1
@.str.1 = private unnamed_addr constant [12 x i8] c"UPPER_BOUND\00", align 1
@.str.2 = private unnamed_addr constant [7 x i8] c"PREFIX\00", align 1
@.str.3 = private unnamed_addr constant [9 x i8] c"children\00", align 1
@.str.4 = private unnamed_addr constant [9 x i8] c"terminal\00", align 1
@.str.5 = private unnamed_addr constant [9 x i8] c"__init__\00", align 1
@.str.6 = private unnamed_addr constant [14 x i8] c"__classcell__\00", align 1
@.str.7 = private unnamed_addr constant [5 x i8] c"Node\00", align 1
@.str.8 = private unnamed_addr constant [6 x i8] c"limit\00", align 1
@.str.9 = private unnamed_addr constant [6 x i8] c"prime\00", align 1
@.str.10 = private unnamed_addr constant [6 x i8] c"print\00", align 1
@.str.11 = private unnamed_addr constant [30 x i8] c"Sieve initialized with limit:\00", align 1
@.str.12 = private unnamed_addr constant [6 x i8] c"range\00", align 1
@.str.13 = private unnamed_addr constant [7 x i8] c"append\00", align 1
@.str.14 = private unnamed_addr constant [20 x i8] c"Total primes found:\00", align 1
@.str.15 = private unnamed_addr constant [4 x i8] c"len\00", align 1
@.str.16 = private unnamed_addr constant [8 x i8] c"to_list\00", align 1
@.str.17 = private unnamed_addr constant [13 x i8] c"omit_squares\00", align 1
@.str.18 = private unnamed_addr constant [6 x i8] c"step1\00", align 1
@.str.19 = private unnamed_addr constant [6 x i8] c"step2\00", align 1
@.str.20 = private unnamed_addr constant [6 x i8] c"step3\00", align 1
@.str.21 = private unnamed_addr constant [7 x i8] c"loop_y\00", align 1
@.str.22 = private unnamed_addr constant [23 x i8] c"Processing loop_x, x =\00", align 1
@.str.23 = private unnamed_addr constant [7 x i8] c"loop_x\00", align 1
@.str.24 = private unnamed_addr constant [30 x i8] c"Starting sieve calculation...\00", align 1
@.str.25 = private unnamed_addr constant [42 x i8] c"Sieve loops finished, omitting squares...\00", align 1
@.str.26 = private unnamed_addr constant [5 x i8] c"calc\00", align 1
@.str.27 = private unnamed_addr constant [6 x i8] c"Sieve\00", align 1
@.str.28 = private unnamed_addr constant [39 x i8] c"Generating trie for primes, list size:\00", align 1
@.str.29 = private unnamed_addr constant [4 x i8] c"str\00", align 1
@.str.30 = private unnamed_addr constant [26 x i8] c"Trie generation complete.\00", align 1
@.str.31 = private unnamed_addr constant [14 x i8] c"generate_trie\00", align 1
@.str.32 = private unnamed_addr constant [22 x i8] c"Searching for prefix:\00", align 1
@.str.33 = private unnamed_addr constant [27 x i8] c"Base primes check for 100:\00", align 1
@.str.34 = private unnamed_addr constant [29 x i8] c"Navigating to prefix node...\00", align 1
@.str.35 = private unnamed_addr constant [21 x i8] c"Link check for char:\00", align 1
@.str.36 = private unnamed_addr constant [4 x i8] c"get\00", align 1
@.str.37 = private unnamed_addr constant [27 x i8] c"Prefix node break at char:\00", align 1
@.str.38 = private unnamed_addr constant [50 x i8] c"Prefix node found. Terminal state of prefix node:\00", align 1
@.str.39 = private unnamed_addr constant [46 x i8] c"Starting queue traversal. Initial queue size:\00", align 1
@.str.40 = private unnamed_addr constant [4 x i8] c"pop\00", align 1
@.str.41 = private unnamed_addr constant [11 x i8] c"Queue pop:\00", align 1
@.str.42 = private unnamed_addr constant [10 x i8] c"Terminal:\00", align 1
@.str.43 = private unnamed_addr constant [4 x i8] c"int\00", align 1
@.str.44 = private unnamed_addr constant [5 x i8] c"keys\00", align 1
@.str.45 = private unnamed_addr constant [24 x i8] c"Found children chars at\00", align 1
@.str.46 = private unnamed_addr constant [2 x i8] c":\00", align 1
@.str.47 = private unnamed_addr constant [5 x i8] c"list\00", align 1
@.str.48 = private unnamed_addr constant [6 x i8] c"items\00", align 1
@.str.49 = private unnamed_addr constant [18 x i8] c"Pushing to queue:\00", align 1
@.str.50 = private unnamed_addr constant [7 x i8] c"insert\00", align 1
@.str.51 = private unnamed_addr constant [5 x i8] c"sort\00", align 1
@.str.52 = private unnamed_addr constant [31 x i8] c"Find completed. Results count:\00", align 1
@.str.53 = private unnamed_addr constant [8 x i8] c"Values:\00", align 1
@.str.54 = private unnamed_addr constant [5 x i8] c"find\00", align 1
@.str.55 = private unnamed_addr constant [20 x i8] c"Running verify()...\00", align 1
@.str.56 = private unnamed_addr constant [22 x i8] c"Verify results match:\00", align 1
@.str.57 = private unnamed_addr constant [10 x i8] c"Expected:\00", align 1
@.str.58 = private unnamed_addr constant [8 x i8] c"Actual:\00", align 1
@.str.59 = private unnamed_addr constant [7 x i8] c"verify\00", align 1
@.str.60 = private unnamed_addr constant [41 x i8] c"Starting main calculation (UPPER_BOUND =\00", align 1
@.str.61 = private unnamed_addr constant [2 x i8] c")\00", align 1
@.str.62 = private unnamed_addr constant [8 x i8] c"results\00", align 1
@.str.63 = private unnamed_addr constant [15 x i8] c"Final Results:\00", align 1

; Function Attrs: uwtable(sync)
define void @PyInit_test() #0 {
entry:
  %tuple_elems3 = alloca [2 x ptr], align 8
  %tuple_elems1 = alloca [2 x ptr], align 8
  %tuple_elems = alloca [3 x ptr], align 8
  %0 = call ptr @_Z13rt_add_modulePKc(ptr @.str)
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 5000000)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.1, ptr %1)
  %2 = call ptr @_Z19rt_integer_from_i64l(i64 32338)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.2, ptr %2)
  %3 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.7, ptr @"test.<module>.0:0.Node.3:0.__body__", ptr %0, ptr null, ptr null, ptr null)
  %4 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %5 = call ptr @_Z18rt_build_class_aotPN2py8PyObjectEPKcS1_S1_(ptr %3, ptr @.str.7, ptr %4, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.7, ptr %5)
  %6 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.27, ptr @"test.<module>.0:0.Sieve.8:0.__body__", ptr %0, ptr null, ptr null, ptr null)
  %7 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %8 = call ptr @_Z18rt_build_class_aotPN2py8PyObjectEPKcS1_S1_(ptr %6, ptr @.str.27, ptr %7, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.27, ptr %8)
  %9 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.31, ptr @"test.<module>.0:0.generate_trie.71:0", ptr %0, ptr null, ptr null, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.31, ptr %9)
  %10 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.54, ptr @"test.<module>.0:0.find.86:0", ptr %0, ptr null, ptr null, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.54, ptr %10)
  %11 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.59, ptr @"test.<module>.0:0.verify.132:0", ptr %0, ptr null, ptr null, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.59, ptr %11)
  %12 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.59)
  %13 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %14 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %12, ptr %13, ptr null)
  %15 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.10)
  %16 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.60, i64 40)
  %17 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.1)
  %18 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.61, i64 1)
  %19 = getelementptr [3 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %16, ptr %19, align 8
  %20 = getelementptr [3 x ptr], ptr %tuple_elems, i32 0, i32 1
  store ptr %17, ptr %20, align 8
  %21 = getelementptr [3 x ptr], ptr %tuple_elems, i32 0, i32 2
  store ptr %18, ptr %21, align 8
  %arr_ptr = getelementptr [3 x ptr], ptr %tuple_elems, i32 0, i32 0
  %22 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 3, ptr %arr_ptr)
  %23 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %15, ptr %22, ptr null)
  %24 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.54)
  %25 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.1)
  %26 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.2)
  %27 = getelementptr [2 x ptr], ptr %tuple_elems1, i32 0, i32 0
  store ptr %25, ptr %27, align 8
  %28 = getelementptr [2 x ptr], ptr %tuple_elems1, i32 0, i32 1
  store ptr %26, ptr %28, align 8
  %arr_ptr2 = getelementptr [2 x ptr], ptr %tuple_elems1, i32 0, i32 0
  %29 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr2)
  %30 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %24, ptr %29, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.62, ptr %30)
  %31 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.10)
  %32 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.63, i64 14)
  %33 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.62)
  %34 = getelementptr [2 x ptr], ptr %tuple_elems3, i32 0, i32 0
  store ptr %32, ptr %34, align 8
  %35 = getelementptr [2 x ptr], ptr %tuple_elems3, i32 0, i32 1
  store ptr %33, ptr %35, align 8
  %arr_ptr4 = getelementptr [2 x ptr], ptr %tuple_elems3, i32 0, i32 0
  %36 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr4)
  %37 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %31, ptr %36, ptr null)
  ret void
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_add_modulePKc(ptr noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z19rt_integer_from_i64l(i64 noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Node.3:0.__body__"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %__class__.cell = alloca ptr, align 8
  store ptr null, ptr %__class__.cell, align 8
  %ns = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  %0 = call ptr @_Z14rt_create_cellPN2py8PyObjectE(ptr null)
  store ptr %0, ptr %__class__.cell, align 8
  %1 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.5, ptr @"test.<module>.0:0.Node.3:0.__init__.4:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.5, ptr %1)
  %__class__.cell.load = load ptr, ptr %__class__.cell, align 8
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.6, ptr %__class__.cell.load)
  %2 = call ptr @_Z7rt_nonev()
  ret ptr %2
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr noundef nonnull, i32 noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_create_cellPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Node.3:0.__init__.4:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z13rt_build_dictiPPN2py8PyObjectES2_(i32 0, ptr null, ptr null)
  %self1 = load ptr, ptr %self, align 8
  call void @_Z10rt_setattrPN2py8PyObjectEPKcS1_(ptr %self1, ptr @.str.3, ptr %1)
  %2 = call ptr @_Z8rt_falsev()
  %self2 = load ptr, ptr %self, align 8
  call void @_Z10rt_setattrPN2py8PyObjectEPKcS1_(ptr %self2, ptr @.str.4, ptr %2)
  %3 = call ptr @_Z7rt_nonev()
  ret ptr %3
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_build_dictiPPN2py8PyObjectES2_(i32 noundef, ptr nocapture noundef readonly, ptr nocapture noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare void @_Z10rt_setattrPN2py8PyObjectEPKcS1_(ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z8rt_falsev() #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z7rt_nonev() #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr noundef readonly, ptr noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr noundef, ptr noundef readonly, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 noundef, ptr nocapture noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z18rt_build_class_aotPN2py8PyObjectEPKcS1_S1_(ptr noundef, ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.__body__"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %__class__.cell = alloca ptr, align 8
  store ptr null, ptr %__class__.cell, align 8
  %ns = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  %0 = call ptr @_Z14rt_create_cellPN2py8PyObjectE(ptr null)
  store ptr %0, ptr %__class__.cell, align 8
  %1 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.5, ptr @"test.<module>.0:0.Sieve.8:0.__init__.9:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.5, ptr %1)
  %2 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.16, ptr @"test.<module>.0:0.Sieve.8:0.to_list.14:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.16, ptr %2)
  %3 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.17, ptr @"test.<module>.0:0.Sieve.8:0.omit_squares.22:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.17, ptr %3)
  %4 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.18, ptr @"test.<module>.0:0.Sieve.8:0.step1.33:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.18, ptr %4)
  %5 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.19, ptr @"test.<module>.0:0.Sieve.8:0.step2.38:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.19, ptr %5)
  %6 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.20, ptr @"test.<module>.0:0.Sieve.8:0.step3.43:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.20, ptr %6)
  %7 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.21, ptr @"test.<module>.0:0.Sieve.8:0.loop_y.48:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.21, ptr %7)
  %8 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.23, ptr @"test.<module>.0:0.Sieve.8:0.loop_x.56:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.23, ptr %8)
  %9 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.26, ptr @"test.<module>.0:0.Sieve.8:0.calc.64:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.26, ptr %9)
  %__class__.cell.load = load ptr, ptr %__class__.cell, align 8
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.6, ptr %__class__.cell.load)
  %10 = call ptr @_Z7rt_nonev()
  ret ptr %10
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.__init__.9:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems = alloca [2 x ptr], align 8
  %list_elems = alloca [1 x ptr], align 8
  %limit = alloca ptr, align 8
  store ptr null, ptr %limit, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 1)
  store ptr %1, ptr %limit, align 8
  %limit1 = load ptr, ptr %limit, align 8
  %self2 = load ptr, ptr %self, align 8
  call void @_Z10rt_setattrPN2py8PyObjectEPKcS1_(ptr %self2, ptr @.str.8, ptr %limit1)
  %2 = call ptr @_Z8rt_falsev()
  %3 = getelementptr [1 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %2, ptr %3, align 8
  %4 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 1, ptr %list_elems)
  %limit3 = load ptr, ptr %limit, align 8
  %5 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %6 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %limit3, ptr %5)
  %7 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %4, ptr %6)
  %self4 = load ptr, ptr %self, align 8
  call void @_Z10rt_setattrPN2py8PyObjectEPKcS1_(ptr %self4, ptr @.str.9, ptr %7)
  %8 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %9 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.11, i64 29)
  %limit5 = load ptr, ptr %limit, align 8
  %10 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %9, ptr %10, align 8
  %11 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 1
  store ptr %limit5, ptr %11, align 8
  %arr_ptr = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  %12 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr)
  %13 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %8, ptr %12, ptr null)
  %14 = call ptr @_Z7rt_nonev()
  ret ptr %14
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 noundef, ptr nocapture noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z19rt_string_from_cstrPKcl(ptr noundef readonly, i64 noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr noundef nonnull, ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.to_list.14:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems11 = alloca [2 x ptr], align 8
  %tuple_elems9 = alloca [1 x ptr], align 8
  %tuple_elems6 = alloca [1 x ptr], align 8
  %p = alloca ptr, align 8
  store ptr null, ptr %p, align 8
  %for_has_value = alloca i1, align 1
  store i1 false, ptr %for_has_value, align 1
  %tuple_elems = alloca [2 x ptr], align 8
  %result = alloca ptr, align 8
  store ptr null, ptr %result, align 8
  %list_elems = alloca [2 x ptr], align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %2 = call ptr @_Z19rt_integer_from_i64l(i64 3)
  %3 = getelementptr [2 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %1, ptr %3, align 8
  %4 = getelementptr [2 x ptr], ptr %list_elems, i32 0, i32 1
  store ptr %2, ptr %4, align 8
  %5 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 2, ptr %list_elems)
  store ptr %5, ptr %result, align 8
  %6 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.12)
  %7 = call ptr @_Z19rt_integer_from_i64l(i64 5)
  %self1 = load ptr, ptr %self, align 8
  %8 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self1, ptr @.str.8)
  %9 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %10 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %8, ptr %9)
  %11 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %7, ptr %11, align 8
  %12 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 1
  store ptr %10, ptr %12, align 8
  %arr_ptr = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  %13 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr)
  %14 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %6, ptr %13, ptr null)
  %15 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %14)
  br label %for.cond

for.cond:                                         ; preds = %if.merge, %entry
  %16 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %15, ptr %for_has_value)
  %17 = load i1, ptr %for_has_value, align 1
  br i1 %17, label %for.body, label %for.merge

for.body:                                         ; preds = %for.cond
  store ptr %16, ptr %p, align 8
  %self2 = load ptr, ptr %self, align 8
  %18 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self2, ptr @.str.9)
  %p3 = load ptr, ptr %p, align 8
  %19 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %18, ptr %p3)
  %20 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %19)
  br i1 %20, label %if.then, label %if.merge

for.merge:                                        ; preds = %for.cond
  %21 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %22 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.14, i64 19)
  %23 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.15)
  %result8 = load ptr, ptr %result, align 8
  %24 = getelementptr [1 x ptr], ptr %tuple_elems9, i32 0, i32 0
  store ptr %result8, ptr %24, align 8
  %arr_ptr10 = getelementptr [1 x ptr], ptr %tuple_elems9, i32 0, i32 0
  %25 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr10)
  %26 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %23, ptr %25, ptr null)
  %27 = getelementptr [2 x ptr], ptr %tuple_elems11, i32 0, i32 0
  store ptr %22, ptr %27, align 8
  %28 = getelementptr [2 x ptr], ptr %tuple_elems11, i32 0, i32 1
  store ptr %26, ptr %28, align 8
  %arr_ptr12 = getelementptr [2 x ptr], ptr %tuple_elems11, i32 0, i32 0
  %29 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr12)
  %30 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %21, ptr %29, ptr null)
  %result13 = load ptr, ptr %result, align 8
  ret ptr %result13

if.then:                                          ; preds = %for.body
  %result4 = load ptr, ptr %result, align 8
  %31 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %result4, ptr @.str.13)
  %p5 = load ptr, ptr %p, align 8
  %32 = getelementptr [1 x ptr], ptr %tuple_elems6, i32 0, i32 0
  store ptr %p5, ptr %32, align 8
  %arr_ptr7 = getelementptr [1 x ptr], ptr %tuple_elems6, i32 0, i32 0
  %33 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr7)
  %34 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %31, ptr %33, ptr null)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %for.body
  br label %for.cond
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr noundef nonnull) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr noundef nonnull, ptr nocapture noundef writeonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef zeroext i1 @_Z10rt_is_truePN2py8PyObjectE(ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.omit_squares.22:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %i = alloca ptr, align 8
  store ptr null, ptr %i, align 8
  %r = alloca ptr, align 8
  store ptr null, ptr %r, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 5)
  store ptr %1, ptr %r, align 8
  br label %while.cond

while.cond:                                       ; preds = %if.merge, %entry
  %r1 = load ptr, ptr %r, align 8
  %r2 = load ptr, ptr %r, align 8
  %2 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %r1, ptr %r2)
  %self3 = load ptr, ptr %self, align 8
  %3 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self3, ptr @.str.8)
  %4 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %2, ptr %3)
  %5 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %4)
  br i1 %5, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %self4 = load ptr, ptr %self, align 8
  %6 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self4, ptr @.str.9)
  %r5 = load ptr, ptr %r, align 8
  %7 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %6, ptr %r5)
  %8 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %7)
  br i1 %8, label %if.then, label %if.merge

while.merge:                                      ; preds = %while.cond
  %self19 = load ptr, ptr %self, align 8
  ret ptr %self19

if.then:                                          ; preds = %while.body
  %r6 = load ptr, ptr %r, align 8
  %r7 = load ptr, ptr %r, align 8
  %9 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %r6, ptr %r7)
  store ptr %9, ptr %i, align 8
  br label %while.cond8

if.merge:                                         ; preds = %while.merge10, %while.body
  %10 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %r18 = load ptr, ptr %r, align 8
  %11 = call ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr %r18, ptr %10)
  store ptr %11, ptr %r, align 8
  br label %while.cond

while.cond8:                                      ; preds = %while.body9, %if.then
  %i11 = load ptr, ptr %i, align 8
  %self12 = load ptr, ptr %self, align 8
  %12 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self12, ptr @.str.8)
  %13 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %i11, ptr %12)
  %14 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %13)
  br i1 %14, label %while.body9, label %while.merge10

while.body9:                                      ; preds = %while.cond8
  %15 = call ptr @_Z8rt_falsev()
  %self13 = load ptr, ptr %self, align 8
  %16 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self13, ptr @.str.9)
  %i14 = load ptr, ptr %i, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %16, ptr %i14, ptr %15)
  %i15 = load ptr, ptr %i, align 8
  %r16 = load ptr, ptr %r, align 8
  %r17 = load ptr, ptr %r, align 8
  %17 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %r16, ptr %r17)
  %18 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %i15, ptr %17)
  store ptr %18, ptr %i, align 8
  br label %while.cond8

while.merge10:                                    ; preds = %while.cond8
  br label %if.merge
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr noundef nonnull, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.step1.33:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %.bool_result7 = alloca ptr, align 8
  store ptr null, ptr %.bool_result7, align 8
  %.bool_result = alloca ptr, align 8
  store ptr null, ptr %.bool_result, align 8
  %n = alloca ptr, align 8
  store ptr null, ptr %n, align 8
  %y = alloca ptr, align 8
  store ptr null, ptr %y, align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 1)
  store ptr %1, ptr %x, align 8
  %2 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 2)
  store ptr %2, ptr %y, align 8
  %3 = call ptr @_Z19rt_integer_from_i64l(i64 4)
  %x1 = load ptr, ptr %x, align 8
  %4 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %3, ptr %x1)
  %x2 = load ptr, ptr %x, align 8
  %5 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %4, ptr %x2)
  %y3 = load ptr, ptr %y, align 8
  %y4 = load ptr, ptr %y, align 8
  %6 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %y3, ptr %y4)
  %7 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %5, ptr %6)
  store ptr %7, ptr %n, align 8
  %n5 = load ptr, ptr %n, align 8
  %self6 = load ptr, ptr %self, align 8
  %8 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self6, ptr @.str.8)
  %9 = call ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr %n5, ptr %8)
  store ptr %9, ptr %.bool_result, align 8
  %10 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %9)
  br i1 %10, label %boolop.next, label %boolop.merge

boolop.merge:                                     ; preds = %boolop.merge8, %entry
  %boolop.result12 = load ptr, ptr %.bool_result, align 8
  %11 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %boolop.result12)
  br i1 %11, label %if.then, label %if.merge

boolop.next:                                      ; preds = %entry
  %n9 = load ptr, ptr %n, align 8
  %12 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %13 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n9, ptr %12)
  %14 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %15 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %13, ptr %14)
  store ptr %15, ptr %.bool_result7, align 8
  %16 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %15)
  br i1 %16, label %boolop.merge8, label %boolop.next10

boolop.merge8:                                    ; preds = %boolop.next10, %boolop.next
  %boolop.result = load ptr, ptr %.bool_result7, align 8
  store ptr %boolop.result, ptr %.bool_result, align 8
  br label %boolop.merge

boolop.next10:                                    ; preds = %boolop.next
  %n11 = load ptr, ptr %n, align 8
  %17 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %18 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n11, ptr %17)
  %19 = call ptr @_Z19rt_integer_from_i64l(i64 5)
  %20 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %18, ptr %19)
  store ptr %20, ptr %.bool_result7, align 8
  br label %boolop.merge8

if.then:                                          ; preds = %boolop.merge
  %self13 = load ptr, ptr %self, align 8
  %21 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self13, ptr @.str.9)
  %n14 = load ptr, ptr %n, align 8
  %22 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %21, ptr %n14)
  %23 = call ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr %22)
  %self15 = load ptr, ptr %self, align 8
  %24 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self15, ptr @.str.9)
  %n16 = load ptr, ptr %n, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %24, ptr %n16, ptr %23)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %boolop.merge
  %25 = call ptr @_Z7rt_nonev()
  ret ptr %25
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.step2.38:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %.bool_result = alloca ptr, align 8
  store ptr null, ptr %.bool_result, align 8
  %n = alloca ptr, align 8
  store ptr null, ptr %n, align 8
  %y = alloca ptr, align 8
  store ptr null, ptr %y, align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 1)
  store ptr %1, ptr %x, align 8
  %2 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 2)
  store ptr %2, ptr %y, align 8
  %3 = call ptr @_Z19rt_integer_from_i64l(i64 3)
  %x1 = load ptr, ptr %x, align 8
  %4 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %3, ptr %x1)
  %x2 = load ptr, ptr %x, align 8
  %5 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %4, ptr %x2)
  %y3 = load ptr, ptr %y, align 8
  %y4 = load ptr, ptr %y, align 8
  %6 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %y3, ptr %y4)
  %7 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %5, ptr %6)
  store ptr %7, ptr %n, align 8
  %n5 = load ptr, ptr %n, align 8
  %self6 = load ptr, ptr %self, align 8
  %8 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self6, ptr @.str.8)
  %9 = call ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr %n5, ptr %8)
  store ptr %9, ptr %.bool_result, align 8
  %10 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %9)
  br i1 %10, label %boolop.next, label %boolop.merge

boolop.merge:                                     ; preds = %boolop.next, %entry
  %boolop.result = load ptr, ptr %.bool_result, align 8
  %11 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %boolop.result)
  br i1 %11, label %if.then, label %if.merge

boolop.next:                                      ; preds = %entry
  %n7 = load ptr, ptr %n, align 8
  %12 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %13 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n7, ptr %12)
  %14 = call ptr @_Z19rt_integer_from_i64l(i64 7)
  %15 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %13, ptr %14)
  store ptr %15, ptr %.bool_result, align 8
  br label %boolop.merge

if.then:                                          ; preds = %boolop.merge
  %self8 = load ptr, ptr %self, align 8
  %16 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self8, ptr @.str.9)
  %n9 = load ptr, ptr %n, align 8
  %17 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %16, ptr %n9)
  %18 = call ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr %17)
  %self10 = load ptr, ptr %self, align 8
  %19 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self10, ptr @.str.9)
  %n11 = load ptr, ptr %n, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %19, ptr %n11, ptr %18)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %boolop.merge
  %20 = call ptr @_Z7rt_nonev()
  ret ptr %20
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.step3.43:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %.bool_result = alloca ptr, align 8
  store ptr null, ptr %.bool_result, align 8
  %n = alloca ptr, align 8
  store ptr null, ptr %n, align 8
  %y = alloca ptr, align 8
  store ptr null, ptr %y, align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 1)
  store ptr %1, ptr %x, align 8
  %2 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 2)
  store ptr %2, ptr %y, align 8
  %3 = call ptr @_Z19rt_integer_from_i64l(i64 3)
  %x1 = load ptr, ptr %x, align 8
  %4 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %3, ptr %x1)
  %x2 = load ptr, ptr %x, align 8
  %5 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %4, ptr %x2)
  %y3 = load ptr, ptr %y, align 8
  %y4 = load ptr, ptr %y, align 8
  %6 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %y3, ptr %y4)
  %7 = call ptr @_Z13rt_binary_subPN2py8PyObjectES1_(ptr %5, ptr %6)
  store ptr %7, ptr %n, align 8
  %x5 = load ptr, ptr %x, align 8
  %y6 = load ptr, ptr %y, align 8
  %8 = call ptr @_Z13rt_compare_gtPN2py8PyObjectES1_(ptr %x5, ptr %y6)
  store ptr %8, ptr %.bool_result, align 8
  %9 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %8)
  br i1 %9, label %boolop.next, label %boolop.merge

boolop.merge:                                     ; preds = %boolop.next9, %boolop.next, %entry
  %boolop.result = load ptr, ptr %.bool_result, align 8
  %10 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %boolop.result)
  br i1 %10, label %if.then, label %if.merge

boolop.next:                                      ; preds = %entry
  %n7 = load ptr, ptr %n, align 8
  %self8 = load ptr, ptr %self, align 8
  %11 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self8, ptr @.str.8)
  %12 = call ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr %n7, ptr %11)
  store ptr %12, ptr %.bool_result, align 8
  %13 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %12)
  br i1 %13, label %boolop.next9, label %boolop.merge

boolop.next9:                                     ; preds = %boolop.next
  %n10 = load ptr, ptr %n, align 8
  %14 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %15 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n10, ptr %14)
  %16 = call ptr @_Z19rt_integer_from_i64l(i64 11)
  %17 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %15, ptr %16)
  store ptr %17, ptr %.bool_result, align 8
  br label %boolop.merge

if.then:                                          ; preds = %boolop.merge
  %self11 = load ptr, ptr %self, align 8
  %18 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self11, ptr @.str.9)
  %n12 = load ptr, ptr %n, align 8
  %19 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %18, ptr %n12)
  %20 = call ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr %19)
  %self13 = load ptr, ptr %self, align 8
  %21 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self13, ptr @.str.9)
  %n14 = load ptr, ptr %n, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %21, ptr %n14, ptr %20)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %boolop.merge
  %22 = call ptr @_Z7rt_nonev()
  ret ptr %22
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_subPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_gtPN2py8PyObjectES1_(ptr noundef nonnull, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.loop_y.48:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems15 = alloca [2 x ptr], align 8
  %tuple_elems10 = alloca [2 x ptr], align 8
  %tuple_elems = alloca [2 x ptr], align 8
  %y = alloca ptr, align 8
  store ptr null, ptr %y, align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 1)
  store ptr %1, ptr %x, align 8
  %2 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  store ptr %2, ptr %y, align 8
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %y1 = load ptr, ptr %y, align 8
  %y2 = load ptr, ptr %y, align 8
  %3 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %y1, ptr %y2)
  %self3 = load ptr, ptr %self, align 8
  %4 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self3, ptr @.str.8)
  %5 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %3, ptr %4)
  %6 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %5)
  br i1 %6, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %self4 = load ptr, ptr %self, align 8
  %7 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self4, ptr @.str.18)
  %x5 = load ptr, ptr %x, align 8
  %y6 = load ptr, ptr %y, align 8
  %8 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %x5, ptr %8, align 8
  %9 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 1
  store ptr %y6, ptr %9, align 8
  %arr_ptr = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  %10 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr)
  %11 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %7, ptr %10, ptr null)
  %self7 = load ptr, ptr %self, align 8
  %12 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self7, ptr @.str.19)
  %x8 = load ptr, ptr %x, align 8
  %y9 = load ptr, ptr %y, align 8
  %13 = getelementptr [2 x ptr], ptr %tuple_elems10, i32 0, i32 0
  store ptr %x8, ptr %13, align 8
  %14 = getelementptr [2 x ptr], ptr %tuple_elems10, i32 0, i32 1
  store ptr %y9, ptr %14, align 8
  %arr_ptr11 = getelementptr [2 x ptr], ptr %tuple_elems10, i32 0, i32 0
  %15 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr11)
  %16 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %12, ptr %15, ptr null)
  %self12 = load ptr, ptr %self, align 8
  %17 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self12, ptr @.str.20)
  %x13 = load ptr, ptr %x, align 8
  %y14 = load ptr, ptr %y, align 8
  %18 = getelementptr [2 x ptr], ptr %tuple_elems15, i32 0, i32 0
  store ptr %x13, ptr %18, align 8
  %19 = getelementptr [2 x ptr], ptr %tuple_elems15, i32 0, i32 1
  store ptr %y14, ptr %19, align 8
  %arr_ptr16 = getelementptr [2 x ptr], ptr %tuple_elems15, i32 0, i32 0
  %20 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr16)
  %21 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %17, ptr %20, ptr null)
  %22 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %y17 = load ptr, ptr %y, align 8
  %23 = call ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr %y17, ptr %22)
  store ptr %23, ptr %y, align 8
  br label %while.cond

while.merge:                                      ; preds = %while.cond
  %24 = call ptr @_Z7rt_nonev()
  ret ptr %24
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.loop_x.56:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems8 = alloca [1 x ptr], align 8
  %tuple_elems = alloca [2 x ptr], align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  store ptr %1, ptr %x, align 8
  br label %while.cond

while.cond:                                       ; preds = %if.merge, %entry
  %x1 = load ptr, ptr %x, align 8
  %x2 = load ptr, ptr %x, align 8
  %2 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %x1, ptr %x2)
  %self3 = load ptr, ptr %self, align 8
  %3 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self3, ptr @.str.8)
  %4 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %2, ptr %3)
  %5 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %4)
  br i1 %5, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %x4 = load ptr, ptr %x, align 8
  %6 = call ptr @_Z19rt_integer_from_i64l(i64 500)
  %7 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %x4, ptr %6)
  %8 = call ptr @_Z19rt_integer_from_i64l(i64 0)
  %9 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %7, ptr %8)
  %10 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %9)
  br i1 %10, label %if.then, label %if.merge

while.merge:                                      ; preds = %while.cond
  %11 = call ptr @_Z7rt_nonev()
  ret ptr %11

if.then:                                          ; preds = %while.body
  %12 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %13 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.22, i64 22)
  %x5 = load ptr, ptr %x, align 8
  %14 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %13, ptr %14, align 8
  %15 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 1
  store ptr %x5, ptr %15, align 8
  %arr_ptr = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  %16 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr)
  %17 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %12, ptr %16, ptr null)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %while.body
  %self6 = load ptr, ptr %self, align 8
  %18 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self6, ptr @.str.21)
  %x7 = load ptr, ptr %x, align 8
  %19 = getelementptr [1 x ptr], ptr %tuple_elems8, i32 0, i32 0
  store ptr %x7, ptr %19, align 8
  %arr_ptr9 = getelementptr [1 x ptr], ptr %tuple_elems8, i32 0, i32 0
  %20 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr9)
  %21 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %18, ptr %20, ptr null)
  %22 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %x10 = load ptr, ptr %x, align 8
  %23 = call ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr %x10, ptr %22)
  store ptr %23, ptr %x, align 8
  br label %while.cond
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.calc.64:4"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems2 = alloca [1 x ptr], align 8
  %tuple_elems = alloca [1 x ptr], align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %2 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.24, i64 29)
  %3 = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %2, ptr %3, align 8
  %arr_ptr = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  %4 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr)
  %5 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %1, ptr %4, ptr null)
  %self1 = load ptr, ptr %self, align 8
  %6 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self1, ptr @.str.23)
  %7 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %8 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %6, ptr %7, ptr null)
  %9 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %10 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.25, i64 41)
  %11 = getelementptr [1 x ptr], ptr %tuple_elems2, i32 0, i32 0
  store ptr %10, ptr %11, align 8
  %arr_ptr3 = getelementptr [1 x ptr], ptr %tuple_elems2, i32 0, i32 0
  %12 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr3)
  %13 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %9, ptr %12, ptr null)
  %self4 = load ptr, ptr %self, align 8
  %14 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %self4, ptr @.str.17)
  %15 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %16 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %14, ptr %15, ptr null)
  ret ptr %16
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.generate_trie.71:0"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems21 = alloca [1 x ptr], align 8
  %ch = alloca ptr, align 8
  store ptr null, ptr %ch, align 8
  %for_has_value13 = alloca i1, align 1
  store i1 false, ptr %for_has_value13, align 1
  %head = alloca ptr, align 8
  store ptr null, ptr %head, align 8
  %s_el = alloca ptr, align 8
  store ptr null, ptr %s_el, align 8
  %tuple_elems6 = alloca [1 x ptr], align 8
  %el = alloca ptr, align 8
  store ptr null, ptr %el, align 8
  %for_has_value = alloca i1, align 1
  store i1 false, ptr %for_has_value, align 1
  %root = alloca ptr, align 8
  store ptr null, ptr %root, align 8
  %tuple_elems2 = alloca [2 x ptr], align 8
  %tuple_elems = alloca [1 x ptr], align 8
  %l = alloca ptr, align 8
  store ptr null, ptr %l, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %l, align 8
  %1 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %2 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.28, i64 38)
  %3 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.15)
  %l1 = load ptr, ptr %l, align 8
  %4 = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %l1, ptr %4, align 8
  %arr_ptr = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  %5 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr)
  %6 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %3, ptr %5, ptr null)
  %7 = getelementptr [2 x ptr], ptr %tuple_elems2, i32 0, i32 0
  store ptr %2, ptr %7, align 8
  %8 = getelementptr [2 x ptr], ptr %tuple_elems2, i32 0, i32 1
  store ptr %6, ptr %8, align 8
  %arr_ptr3 = getelementptr [2 x ptr], ptr %tuple_elems2, i32 0, i32 0
  %9 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr3)
  %10 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %1, ptr %9, ptr null)
  %11 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.7)
  %12 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %13 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %11, ptr %12, ptr null)
  store ptr %13, ptr %root, align 8
  %l4 = load ptr, ptr %l, align 8
  %14 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %l4)
  br label %for.cond

for.cond:                                         ; preds = %for.merge12, %entry
  %15 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %14, ptr %for_has_value)
  %16 = load i1, ptr %for_has_value, align 1
  br i1 %16, label %for.body, label %for.merge

for.body:                                         ; preds = %for.cond
  store ptr %15, ptr %el, align 8
  %17 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.29)
  %el5 = load ptr, ptr %el, align 8
  %18 = getelementptr [1 x ptr], ptr %tuple_elems6, i32 0, i32 0
  store ptr %el5, ptr %18, align 8
  %arr_ptr7 = getelementptr [1 x ptr], ptr %tuple_elems6, i32 0, i32 0
  %19 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr7)
  %20 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %17, ptr %19, ptr null)
  store ptr %20, ptr %s_el, align 8
  %root8 = load ptr, ptr %root, align 8
  store ptr %root8, ptr %head, align 8
  %s_el9 = load ptr, ptr %s_el, align 8
  %21 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %s_el9)
  br label %for.cond10

for.merge:                                        ; preds = %for.cond
  %22 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %23 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.30, i64 25)
  %24 = getelementptr [1 x ptr], ptr %tuple_elems21, i32 0, i32 0
  store ptr %23, ptr %24, align 8
  %arr_ptr22 = getelementptr [1 x ptr], ptr %tuple_elems21, i32 0, i32 0
  %25 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr22)
  %26 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %22, ptr %25, ptr null)
  %root23 = load ptr, ptr %root, align 8
  ret ptr %root23

for.cond10:                                       ; preds = %if.merge, %for.body
  %27 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %21, ptr %for_has_value13)
  %28 = load i1, ptr %for_has_value13, align 1
  br i1 %28, label %for.body11, label %for.merge12

for.body11:                                       ; preds = %for.cond10
  store ptr %27, ptr %ch, align 8
  %ch14 = load ptr, ptr %ch, align 8
  %head15 = load ptr, ptr %head, align 8
  %29 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %head15, ptr @.str.3)
  %30 = call ptr @_Z17rt_compare_not_inPN2py8PyObjectES1_(ptr %ch14, ptr %29)
  %31 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %30)
  br i1 %31, label %if.then, label %if.merge

for.merge12:                                      ; preds = %for.cond10
  %32 = call ptr @_Z7rt_truev()
  %head20 = load ptr, ptr %head, align 8
  call void @_Z10rt_setattrPN2py8PyObjectEPKcS1_(ptr %head20, ptr @.str.4, ptr %32)
  br label %for.cond

if.then:                                          ; preds = %for.body11
  %33 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.7)
  %34 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %35 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %33, ptr %34, ptr null)
  %head16 = load ptr, ptr %head, align 8
  %36 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %head16, ptr @.str.3)
  %ch17 = load ptr, ptr %ch, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %36, ptr %ch17, ptr %35)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %for.body11
  %head18 = load ptr, ptr %head, align 8
  %37 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %head18, ptr @.str.3)
  %ch19 = load ptr, ptr %ch, align 8
  %38 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %37, ptr %ch19)
  store ptr %38, ptr %head, align 8
  br label %for.cond10
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z17rt_compare_not_inPN2py8PyObjectES1_(ptr noundef, ptr noundef nonnull) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z7rt_truev() #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.find.86:0"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems99 = alloca [4 x ptr], align 8
  %tuple_elems96 = alloca [1 x ptr], align 8
  %tuple_elems92 = alloca [2 x ptr], align 8
  %tuple_elems90 = alloca [2 x ptr], align 8
  %tuple_elems85 = alloca [2 x ptr], align 8
  %new_prefix = alloca ptr, align 8
  store ptr null, ptr %new_prefix, align 8
  %v = alloca ptr, align 8
  store ptr null, ptr %v, align 8
  %unpack_arr81 = alloca [2 x ptr], align 8
  %for_has_value80 = alloca i1, align 1
  store i1 false, ptr %for_has_value80, align 1
  %tuple_elems74 = alloca [4 x ptr], align 8
  %tuple_elems72 = alloca [1 x ptr], align 8
  %tuple_elems66 = alloca [1 x ptr], align 8
  %child_chars = alloca ptr, align 8
  store ptr null, ptr %child_chars, align 8
  %tuple_elems62 = alloca [1 x ptr], align 8
  %tuple_elems60 = alloca [1 x ptr], align 8
  %tuple_elems53 = alloca [4 x ptr], align 8
  %current_prefix = alloca ptr, align 8
  store ptr null, ptr %current_prefix, align 8
  %top = alloca ptr, align 8
  store ptr null, ptr %top, align 8
  %unpack_arr50 = alloca [2 x ptr], align 8
  %tuple_elems46 = alloca [2 x ptr], align 8
  %tuple_elems44 = alloca [1 x ptr], align 8
  %result = alloca ptr, align 8
  store ptr null, ptr %result, align 8
  %queue = alloca ptr, align 8
  store ptr null, ptr %queue, align 8
  %unpack_arr = alloca [2 x ptr], align 8
  %tuple_elems41 = alloca [2 x ptr], align 8
  %list_elems = alloca [1 x ptr], align 8
  %tuple_elems39 = alloca [2 x ptr], align 8
  %tuple_elems35 = alloca [2 x ptr], align 8
  %tuple_elems32 = alloca [2 x ptr], align 8
  %tuple_elems26 = alloca [1 x ptr], align 8
  %tuple_elems22 = alloca [2 x ptr], align 8
  %ch = alloca ptr, align 8
  store ptr null, ptr %ch, align 8
  %for_has_value = alloca i1, align 1
  store i1 false, ptr %for_has_value, align 1
  %tuple_elems18 = alloca [1 x ptr], align 8
  %head = alloca ptr, align 8
  store ptr null, ptr %head, align 8
  %tuple_elems16 = alloca [1 x ptr], align 8
  %tuple_elems13 = alloca [4 x ptr], align 8
  %prime_list = alloca ptr, align 8
  store ptr null, ptr %prime_list, align 8
  %tuple_elems6 = alloca [2 x ptr], align 8
  %str_prefix = alloca ptr, align 8
  store ptr null, ptr %str_prefix, align 8
  %tuple_elems3 = alloca [1 x ptr], align 8
  %primes = alloca ptr, align 8
  store ptr null, ptr %primes, align 8
  %tuple_elems = alloca [1 x ptr], align 8
  %prefix = alloca ptr, align 8
  store ptr null, ptr %prefix, align 8
  %upper_bound = alloca ptr, align 8
  store ptr null, ptr %upper_bound, align 8
  %0 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 0)
  store ptr %0, ptr %upper_bound, align 8
  %1 = call ptr @_Z16rt_tuple_getitemPN2py8PyObjectEi(ptr %args, i32 1)
  store ptr %1, ptr %prefix, align 8
  %2 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.27)
  %upper_bound1 = load ptr, ptr %upper_bound, align 8
  %3 = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %upper_bound1, ptr %3, align 8
  %arr_ptr = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  %4 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr)
  %5 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %2, ptr %4, ptr null)
  %6 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %5, ptr @.str.26)
  %7 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %8 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %6, ptr %7, ptr null)
  store ptr %8, ptr %primes, align 8
  %9 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.29)
  %prefix2 = load ptr, ptr %prefix, align 8
  %10 = getelementptr [1 x ptr], ptr %tuple_elems3, i32 0, i32 0
  store ptr %prefix2, ptr %10, align 8
  %arr_ptr4 = getelementptr [1 x ptr], ptr %tuple_elems3, i32 0, i32 0
  %11 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr4)
  %12 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %9, ptr %11, ptr null)
  store ptr %12, ptr %str_prefix, align 8
  %13 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %14 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.32, i64 21)
  %str_prefix5 = load ptr, ptr %str_prefix, align 8
  %15 = getelementptr [2 x ptr], ptr %tuple_elems6, i32 0, i32 0
  store ptr %14, ptr %15, align 8
  %16 = getelementptr [2 x ptr], ptr %tuple_elems6, i32 0, i32 1
  store ptr %str_prefix5, ptr %16, align 8
  %arr_ptr7 = getelementptr [2 x ptr], ptr %tuple_elems6, i32 0, i32 0
  %17 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr7)
  %18 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %13, ptr %17, ptr null)
  %primes8 = load ptr, ptr %primes, align 8
  %19 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %primes8, ptr @.str.16)
  %20 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %21 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %19, ptr %20, ptr null)
  store ptr %21, ptr %prime_list, align 8
  %upper_bound9 = load ptr, ptr %upper_bound, align 8
  %22 = call ptr @_Z19rt_integer_from_i64l(i64 100)
  %23 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %upper_bound9, ptr %22)
  %24 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %23)
  br i1 %24, label %if.then, label %if.merge

if.then:                                          ; preds = %entry
  %25 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %26 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.33, i64 26)
  %27 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %prime_list10 = load ptr, ptr %prime_list, align 8
  %28 = call ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr %27, ptr %prime_list10)
  %29 = call ptr @_Z19rt_integer_from_i64l(i64 23)
  %prime_list11 = load ptr, ptr %prime_list, align 8
  %30 = call ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr %29, ptr %prime_list11)
  %31 = call ptr @_Z19rt_integer_from_i64l(i64 29)
  %prime_list12 = load ptr, ptr %prime_list, align 8
  %32 = call ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr %31, ptr %prime_list12)
  %33 = getelementptr [4 x ptr], ptr %tuple_elems13, i32 0, i32 0
  store ptr %26, ptr %33, align 8
  %34 = getelementptr [4 x ptr], ptr %tuple_elems13, i32 0, i32 1
  store ptr %28, ptr %34, align 8
  %35 = getelementptr [4 x ptr], ptr %tuple_elems13, i32 0, i32 2
  store ptr %30, ptr %35, align 8
  %36 = getelementptr [4 x ptr], ptr %tuple_elems13, i32 0, i32 3
  store ptr %32, ptr %36, align 8
  %arr_ptr14 = getelementptr [4 x ptr], ptr %tuple_elems13, i32 0, i32 0
  %37 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 4, ptr %arr_ptr14)
  %38 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %25, ptr %37, ptr null)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %entry
  %39 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.31)
  %prime_list15 = load ptr, ptr %prime_list, align 8
  %40 = getelementptr [1 x ptr], ptr %tuple_elems16, i32 0, i32 0
  store ptr %prime_list15, ptr %40, align 8
  %arr_ptr17 = getelementptr [1 x ptr], ptr %tuple_elems16, i32 0, i32 0
  %41 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr17)
  %42 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %39, ptr %41, ptr null)
  store ptr %42, ptr %head, align 8
  %43 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %44 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.34, i64 28)
  %45 = getelementptr [1 x ptr], ptr %tuple_elems18, i32 0, i32 0
  store ptr %44, ptr %45, align 8
  %arr_ptr19 = getelementptr [1 x ptr], ptr %tuple_elems18, i32 0, i32 0
  %46 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr19)
  %47 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %43, ptr %46, ptr null)
  %str_prefix20 = load ptr, ptr %str_prefix, align 8
  %48 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %str_prefix20)
  br label %for.cond

for.cond:                                         ; preds = %if.merge30, %if.merge
  %49 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %48, ptr %for_has_value)
  %50 = load i1, ptr %for_has_value, align 1
  br i1 %50, label %for.body, label %for.merge

for.body:                                         ; preds = %for.cond
  store ptr %49, ptr %ch, align 8
  %51 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %52 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.35, i64 20)
  %ch21 = load ptr, ptr %ch, align 8
  %53 = getelementptr [2 x ptr], ptr %tuple_elems22, i32 0, i32 0
  store ptr %52, ptr %53, align 8
  %54 = getelementptr [2 x ptr], ptr %tuple_elems22, i32 0, i32 1
  store ptr %ch21, ptr %54, align 8
  %arr_ptr23 = getelementptr [2 x ptr], ptr %tuple_elems22, i32 0, i32 0
  %55 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr23)
  %56 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %51, ptr %55, ptr null)
  %head24 = load ptr, ptr %head, align 8
  %57 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %head24, ptr @.str.3)
  %58 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %57, ptr @.str.36)
  %ch25 = load ptr, ptr %ch, align 8
  %59 = getelementptr [1 x ptr], ptr %tuple_elems26, i32 0, i32 0
  store ptr %ch25, ptr %59, align 8
  %arr_ptr27 = getelementptr [1 x ptr], ptr %tuple_elems26, i32 0, i32 0
  %60 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr27)
  %61 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %58, ptr %60, ptr null)
  store ptr %61, ptr %head, align 8
  %head28 = load ptr, ptr %head, align 8
  %62 = call ptr @_Z7rt_nonev()
  %63 = call ptr @_Z13rt_compare_isPN2py8PyObjectES1_(ptr %head28, ptr %62)
  %64 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %63)
  br i1 %64, label %if.then29, label %if.merge30

for.merge:                                        ; preds = %for.cond
  %65 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %66 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.38, i64 49)
  %head34 = load ptr, ptr %head, align 8
  %67 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %head34, ptr @.str.4)
  %68 = getelementptr [2 x ptr], ptr %tuple_elems35, i32 0, i32 0
  store ptr %66, ptr %68, align 8
  %69 = getelementptr [2 x ptr], ptr %tuple_elems35, i32 0, i32 1
  store ptr %67, ptr %69, align 8
  %arr_ptr36 = getelementptr [2 x ptr], ptr %tuple_elems35, i32 0, i32 0
  %70 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr36)
  %71 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %65, ptr %70, ptr null)
  %head37 = load ptr, ptr %head, align 8
  %str_prefix38 = load ptr, ptr %str_prefix, align 8
  %72 = getelementptr [2 x ptr], ptr %tuple_elems39, i32 0, i32 0
  store ptr %head37, ptr %72, align 8
  %73 = getelementptr [2 x ptr], ptr %tuple_elems39, i32 0, i32 1
  store ptr %str_prefix38, ptr %73, align 8
  %arr_ptr40 = getelementptr [2 x ptr], ptr %tuple_elems39, i32 0, i32 0
  %74 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr40)
  %75 = getelementptr [1 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %74, ptr %75, align 8
  %76 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 1, ptr %list_elems)
  %77 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 0, ptr null)
  %78 = getelementptr [2 x ptr], ptr %tuple_elems41, i32 0, i32 0
  store ptr %76, ptr %78, align 8
  %79 = getelementptr [2 x ptr], ptr %tuple_elems41, i32 0, i32 1
  store ptr %77, ptr %79, align 8
  %arr_ptr42 = getelementptr [2 x ptr], ptr %tuple_elems41, i32 0, i32 0
  %80 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr42)
  call void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr %80, i32 2, ptr %unpack_arr)
  %81 = getelementptr [2 x ptr], ptr %unpack_arr, i32 0, i32 0
  %82 = load ptr, ptr %81, align 8
  store ptr %82, ptr %queue, align 8
  %83 = getelementptr [2 x ptr], ptr %unpack_arr, i32 0, i32 1
  %84 = load ptr, ptr %83, align 8
  store ptr %84, ptr %result, align 8
  %85 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %86 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.39, i64 45)
  %87 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.15)
  %queue43 = load ptr, ptr %queue, align 8
  %88 = getelementptr [1 x ptr], ptr %tuple_elems44, i32 0, i32 0
  store ptr %queue43, ptr %88, align 8
  %arr_ptr45 = getelementptr [1 x ptr], ptr %tuple_elems44, i32 0, i32 0
  %89 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr45)
  %90 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %87, ptr %89, ptr null)
  %91 = getelementptr [2 x ptr], ptr %tuple_elems46, i32 0, i32 0
  store ptr %86, ptr %91, align 8
  %92 = getelementptr [2 x ptr], ptr %tuple_elems46, i32 0, i32 1
  store ptr %90, ptr %92, align 8
  %arr_ptr47 = getelementptr [2 x ptr], ptr %tuple_elems46, i32 0, i32 0
  %93 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr47)
  %94 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %85, ptr %93, ptr null)
  br label %while.cond

if.then29:                                        ; preds = %for.body
  %95 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %96 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.37, i64 26)
  %ch31 = load ptr, ptr %ch, align 8
  %97 = getelementptr [2 x ptr], ptr %tuple_elems32, i32 0, i32 0
  store ptr %96, ptr %97, align 8
  %98 = getelementptr [2 x ptr], ptr %tuple_elems32, i32 0, i32 1
  store ptr %ch31, ptr %98, align 8
  %arr_ptr33 = getelementptr [2 x ptr], ptr %tuple_elems32, i32 0, i32 0
  %99 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr33)
  %100 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %95, ptr %99, ptr null)
  %101 = call ptr @_Z7rt_nonev()
  ret ptr %101

if.merge30:                                       ; preds = %for.body
  br label %for.cond

while.cond:                                       ; preds = %for.merge79, %for.merge
  %queue48 = load ptr, ptr %queue, align 8
  %102 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %queue48)
  br i1 %102, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %queue49 = load ptr, ptr %queue, align 8
  %103 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %queue49, ptr @.str.40)
  %104 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %105 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %103, ptr %104, ptr null)
  call void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr %105, i32 2, ptr %unpack_arr50)
  %106 = getelementptr [2 x ptr], ptr %unpack_arr50, i32 0, i32 0
  %107 = load ptr, ptr %106, align 8
  store ptr %107, ptr %top, align 8
  %108 = getelementptr [2 x ptr], ptr %unpack_arr50, i32 0, i32 1
  %109 = load ptr, ptr %108, align 8
  store ptr %109, ptr %current_prefix, align 8
  %110 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %111 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.41, i64 10)
  %current_prefix51 = load ptr, ptr %current_prefix, align 8
  %112 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.42, i64 9)
  %top52 = load ptr, ptr %top, align 8
  %113 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %top52, ptr @.str.4)
  %114 = getelementptr [4 x ptr], ptr %tuple_elems53, i32 0, i32 0
  store ptr %111, ptr %114, align 8
  %115 = getelementptr [4 x ptr], ptr %tuple_elems53, i32 0, i32 1
  store ptr %current_prefix51, ptr %115, align 8
  %116 = getelementptr [4 x ptr], ptr %tuple_elems53, i32 0, i32 2
  store ptr %112, ptr %116, align 8
  %117 = getelementptr [4 x ptr], ptr %tuple_elems53, i32 0, i32 3
  store ptr %113, ptr %117, align 8
  %arr_ptr54 = getelementptr [4 x ptr], ptr %tuple_elems53, i32 0, i32 0
  %118 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 4, ptr %arr_ptr54)
  %119 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %110, ptr %118, ptr null)
  %top55 = load ptr, ptr %top, align 8
  %120 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %top55, ptr @.str.4)
  %121 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %120)
  br i1 %121, label %if.then56, label %if.merge57

while.merge:                                      ; preds = %while.cond
  %result94 = load ptr, ptr %result, align 8
  %122 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %result94, ptr @.str.51)
  %123 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %124 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %122, ptr %123, ptr null)
  %125 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %126 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.52, i64 30)
  %127 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.15)
  %result95 = load ptr, ptr %result, align 8
  %128 = getelementptr [1 x ptr], ptr %tuple_elems96, i32 0, i32 0
  store ptr %result95, ptr %128, align 8
  %arr_ptr97 = getelementptr [1 x ptr], ptr %tuple_elems96, i32 0, i32 0
  %129 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr97)
  %130 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %127, ptr %129, ptr null)
  %131 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.53, i64 7)
  %result98 = load ptr, ptr %result, align 8
  %132 = getelementptr [4 x ptr], ptr %tuple_elems99, i32 0, i32 0
  store ptr %126, ptr %132, align 8
  %133 = getelementptr [4 x ptr], ptr %tuple_elems99, i32 0, i32 1
  store ptr %130, ptr %133, align 8
  %134 = getelementptr [4 x ptr], ptr %tuple_elems99, i32 0, i32 2
  store ptr %131, ptr %134, align 8
  %135 = getelementptr [4 x ptr], ptr %tuple_elems99, i32 0, i32 3
  store ptr %result98, ptr %135, align 8
  %arr_ptr100 = getelementptr [4 x ptr], ptr %tuple_elems99, i32 0, i32 0
  %136 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 4, ptr %arr_ptr100)
  %137 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %125, ptr %136, ptr null)
  %result101 = load ptr, ptr %result, align 8
  ret ptr %result101

if.then56:                                        ; preds = %while.body
  %result58 = load ptr, ptr %result, align 8
  %138 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %result58, ptr @.str.13)
  %139 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.43)
  %current_prefix59 = load ptr, ptr %current_prefix, align 8
  %140 = getelementptr [1 x ptr], ptr %tuple_elems60, i32 0, i32 0
  store ptr %current_prefix59, ptr %140, align 8
  %arr_ptr61 = getelementptr [1 x ptr], ptr %tuple_elems60, i32 0, i32 0
  %141 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr61)
  %142 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %139, ptr %141, ptr null)
  %143 = getelementptr [1 x ptr], ptr %tuple_elems62, i32 0, i32 0
  store ptr %142, ptr %143, align 8
  %arr_ptr63 = getelementptr [1 x ptr], ptr %tuple_elems62, i32 0, i32 0
  %144 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr63)
  %145 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %138, ptr %144, ptr null)
  br label %if.merge57

if.merge57:                                       ; preds = %if.then56, %while.body
  %top64 = load ptr, ptr %top, align 8
  %146 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %top64, ptr @.str.3)
  %147 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %146, ptr @.str.44)
  %148 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %149 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %147, ptr %148, ptr null)
  store ptr %149, ptr %child_chars, align 8
  %150 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.15)
  %child_chars65 = load ptr, ptr %child_chars, align 8
  %151 = getelementptr [1 x ptr], ptr %tuple_elems66, i32 0, i32 0
  store ptr %child_chars65, ptr %151, align 8
  %arr_ptr67 = getelementptr [1 x ptr], ptr %tuple_elems66, i32 0, i32 0
  %152 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr67)
  %153 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %150, ptr %152, ptr null)
  %154 = call ptr @_Z19rt_integer_from_i64l(i64 0)
  %155 = call ptr @_Z13rt_compare_gtPN2py8PyObjectES1_(ptr %153, ptr %154)
  %156 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %155)
  br i1 %156, label %if.then68, label %if.merge69

if.then68:                                        ; preds = %if.merge57
  %157 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %158 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.45, i64 23)
  %current_prefix70 = load ptr, ptr %current_prefix, align 8
  %159 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.46, i64 1)
  %160 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.47)
  %child_chars71 = load ptr, ptr %child_chars, align 8
  %161 = getelementptr [1 x ptr], ptr %tuple_elems72, i32 0, i32 0
  store ptr %child_chars71, ptr %161, align 8
  %arr_ptr73 = getelementptr [1 x ptr], ptr %tuple_elems72, i32 0, i32 0
  %162 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr73)
  %163 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %160, ptr %162, ptr null)
  %164 = getelementptr [4 x ptr], ptr %tuple_elems74, i32 0, i32 0
  store ptr %158, ptr %164, align 8
  %165 = getelementptr [4 x ptr], ptr %tuple_elems74, i32 0, i32 1
  store ptr %current_prefix70, ptr %165, align 8
  %166 = getelementptr [4 x ptr], ptr %tuple_elems74, i32 0, i32 2
  store ptr %159, ptr %166, align 8
  %167 = getelementptr [4 x ptr], ptr %tuple_elems74, i32 0, i32 3
  store ptr %163, ptr %167, align 8
  %arr_ptr75 = getelementptr [4 x ptr], ptr %tuple_elems74, i32 0, i32 0
  %168 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 4, ptr %arr_ptr75)
  %169 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %157, ptr %168, ptr null)
  br label %if.merge69

if.merge69:                                       ; preds = %if.then68, %if.merge57
  %top76 = load ptr, ptr %top, align 8
  %170 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %top76, ptr @.str.3)
  %171 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %170, ptr @.str.48)
  %172 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %173 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %171, ptr %172, ptr null)
  %174 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %173)
  br label %for.cond77

for.cond77:                                       ; preds = %for.body78, %if.merge69
  %175 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %174, ptr %for_has_value80)
  %176 = load i1, ptr %for_has_value80, align 1
  br i1 %176, label %for.body78, label %for.merge79

for.body78:                                       ; preds = %for.cond77
  call void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr %175, i32 2, ptr %unpack_arr81)
  %177 = getelementptr [2 x ptr], ptr %unpack_arr81, i32 0, i32 0
  %178 = load ptr, ptr %177, align 8
  store ptr %178, ptr %ch, align 8
  %179 = getelementptr [2 x ptr], ptr %unpack_arr81, i32 0, i32 1
  %180 = load ptr, ptr %179, align 8
  store ptr %180, ptr %v, align 8
  %current_prefix82 = load ptr, ptr %current_prefix, align 8
  %ch83 = load ptr, ptr %ch, align 8
  %181 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %current_prefix82, ptr %ch83)
  store ptr %181, ptr %new_prefix, align 8
  %182 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %183 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.49, i64 17)
  %new_prefix84 = load ptr, ptr %new_prefix, align 8
  %184 = getelementptr [2 x ptr], ptr %tuple_elems85, i32 0, i32 0
  store ptr %183, ptr %184, align 8
  %185 = getelementptr [2 x ptr], ptr %tuple_elems85, i32 0, i32 1
  store ptr %new_prefix84, ptr %185, align 8
  %arr_ptr86 = getelementptr [2 x ptr], ptr %tuple_elems85, i32 0, i32 0
  %186 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr86)
  %187 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %182, ptr %186, ptr null)
  %queue87 = load ptr, ptr %queue, align 8
  %188 = call ptr @_Z10rt_getattrPN2py8PyObjectEPKc(ptr %queue87, ptr @.str.50)
  %189 = call ptr @_Z19rt_integer_from_i64l(i64 0)
  %v88 = load ptr, ptr %v, align 8
  %new_prefix89 = load ptr, ptr %new_prefix, align 8
  %190 = getelementptr [2 x ptr], ptr %tuple_elems90, i32 0, i32 0
  store ptr %v88, ptr %190, align 8
  %191 = getelementptr [2 x ptr], ptr %tuple_elems90, i32 0, i32 1
  store ptr %new_prefix89, ptr %191, align 8
  %arr_ptr91 = getelementptr [2 x ptr], ptr %tuple_elems90, i32 0, i32 0
  %192 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr91)
  %193 = getelementptr [2 x ptr], ptr %tuple_elems92, i32 0, i32 0
  store ptr %189, ptr %193, align 8
  %194 = getelementptr [2 x ptr], ptr %tuple_elems92, i32 0, i32 1
  store ptr %192, ptr %194, align 8
  %arr_ptr93 = getelementptr [2 x ptr], ptr %tuple_elems92, i32 0, i32 0
  %195 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr93)
  %196 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %188, ptr %195, ptr null)
  br label %for.cond77

for.merge79:                                      ; preds = %for.cond77
  br label %while.cond
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr noundef, ptr noundef nonnull) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_isPN2py8PyObjectES1_(ptr noundef readnone, ptr noundef readnone) #1

; Function Attrs: mustprogress uwtable
declare void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr noundef, i32 noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.verify.132:0"(ptr %module, ptr %args, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %tuple_elems11 = alloca [2 x ptr], align 8
  %tuple_elems8 = alloca [2 x ptr], align 8
  %tuple_elems5 = alloca [2 x ptr], align 8
  %right = alloca ptr, align 8
  store ptr null, ptr %right, align 8
  %tuple_elems1 = alloca [2 x ptr], align 8
  %left = alloca ptr, align 8
  store ptr null, ptr %left, align 8
  %list_elems = alloca [3 x ptr], align 8
  %tuple_elems = alloca [1 x ptr], align 8
  %0 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %1 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.55, i64 19)
  %2 = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %1, ptr %2, align 8
  %arr_ptr = getelementptr [1 x ptr], ptr %tuple_elems, i32 0, i32 0
  %3 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 1, ptr %arr_ptr)
  %4 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %0, ptr %3, ptr null)
  %5 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %6 = call ptr @_Z19rt_integer_from_i64l(i64 23)
  %7 = call ptr @_Z19rt_integer_from_i64l(i64 29)
  %8 = getelementptr [3 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %5, ptr %8, align 8
  %9 = getelementptr [3 x ptr], ptr %list_elems, i32 0, i32 1
  store ptr %6, ptr %9, align 8
  %10 = getelementptr [3 x ptr], ptr %list_elems, i32 0, i32 2
  store ptr %7, ptr %10, align 8
  %11 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 3, ptr %list_elems)
  store ptr %11, ptr %left, align 8
  %12 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.54)
  %13 = call ptr @_Z19rt_integer_from_i64l(i64 100)
  %14 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %15 = getelementptr [2 x ptr], ptr %tuple_elems1, i32 0, i32 0
  store ptr %13, ptr %15, align 8
  %16 = getelementptr [2 x ptr], ptr %tuple_elems1, i32 0, i32 1
  store ptr %14, ptr %16, align 8
  %arr_ptr2 = getelementptr [2 x ptr], ptr %tuple_elems1, i32 0, i32 0
  %17 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr2)
  %18 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %12, ptr %17, ptr null)
  store ptr %18, ptr %right, align 8
  %19 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %20 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.56, i64 21)
  %left3 = load ptr, ptr %left, align 8
  %right4 = load ptr, ptr %right, align 8
  %21 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %left3, ptr %right4)
  %22 = getelementptr [2 x ptr], ptr %tuple_elems5, i32 0, i32 0
  store ptr %20, ptr %22, align 8
  %23 = getelementptr [2 x ptr], ptr %tuple_elems5, i32 0, i32 1
  store ptr %21, ptr %23, align 8
  %arr_ptr6 = getelementptr [2 x ptr], ptr %tuple_elems5, i32 0, i32 0
  %24 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr6)
  %25 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %19, ptr %24, ptr null)
  %26 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %27 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.57, i64 9)
  %left7 = load ptr, ptr %left, align 8
  %28 = getelementptr [2 x ptr], ptr %tuple_elems8, i32 0, i32 0
  store ptr %27, ptr %28, align 8
  %29 = getelementptr [2 x ptr], ptr %tuple_elems8, i32 0, i32 1
  store ptr %left7, ptr %29, align 8
  %arr_ptr9 = getelementptr [2 x ptr], ptr %tuple_elems8, i32 0, i32 0
  %30 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr9)
  %31 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %26, ptr %30, ptr null)
  %32 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %33 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.58, i64 7)
  %right10 = load ptr, ptr %right, align 8
  %34 = getelementptr [2 x ptr], ptr %tuple_elems11, i32 0, i32 0
  store ptr %33, ptr %34, align 8
  %35 = getelementptr [2 x ptr], ptr %tuple_elems11, i32 0, i32 1
  store ptr %right10, ptr %35, align 8
  %arr_ptr12 = getelementptr [2 x ptr], ptr %tuple_elems11, i32 0, i32 0
  %36 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr12)
  %37 = call ptr @_Z7rt_callPN2py8PyObjectES1_S1_(ptr %32, ptr %36, ptr null)
  %38 = call ptr @_Z7rt_nonev()
  ret ptr %38
}

define i32 @main(i32 %0, ptr %1) personality ptr @__gxx_personality_v0 {
entry:
  call void @_Z7rt_initv()
  invoke void @PyInit_test()
          to label %normal unwind label %unwind

normal:                                           ; preds = %entry
  call void @_Z11rt_shutdownv()
  ret i32 0

unwind:                                           ; preds = %entry
  %2 = landingpad { ptr, i32 }
          catch ptr null
  %3 = extractvalue { ptr, i32 } %2, 0
  %4 = call ptr @_Z14rt_catch_beginPv(ptr %3)
  call void @_Z28rt_print_unhandled_exceptionPN2py8PyObjectE(ptr %4)
  call void @_Z12rt_catch_endv()
  call void @_Z11rt_shutdownv()
  ret i32 1
}

; Function Attrs: uwtable
declare void @_Z7rt_initv() #2

; Function Attrs: uwtable
declare void @_Z11rt_shutdownv() #2

; Function Attrs: mustprogress nounwind uwtable
declare noundef ptr @_Z14rt_catch_beginPv(ptr noundef) #3

; Function Attrs: mustprogress uwtable
declare void @_Z28rt_print_unhandled_exceptionPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z12rt_catch_endv() #1

attributes #0 = { uwtable(sync) }
attributes #1 = { mustprogress uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { uwtable "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

