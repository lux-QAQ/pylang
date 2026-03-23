[2026-03-22 21:15:14.638] [info] [timer] precompile_runtime: 40917.56ms
[2026-03-22 21:15:23.049] [info] [timer] SimpleDriver::create: 49329.47ms
[2026-03-22 21:15:23.053] [info] [timer] stage_codegen: 3.68ms
; ModuleID = 'test'
source_filename = "test"

@.str = private unnamed_addr constant [5 x i8] c"test\00", align 1
@.str.1 = private unnamed_addr constant [12 x i8] c"UPPER_BOUND\00", align 1
@.str.2 = private unnamed_addr constant [7 x i8] c"PREFIX\00", align 1
@.pystr_obj.children = internal global ptr null
@.pystr_obj.terminal = internal global ptr null
@.str.3 = private unnamed_addr constant [9 x i8] c"__init__\00", align 1
@.str.4 = private unnamed_addr constant [14 x i8] c"__classcell__\00", align 1
@.str.5 = private unnamed_addr constant [5 x i8] c"Node\00", align 1
@.pystr_obj.limit = internal global ptr null
@.pystr_obj.prime = internal global ptr null
@.str.6 = private unnamed_addr constant [6 x i8] c"print\00", align 1
@.str.7 = private unnamed_addr constant [30 x i8] c"Sieve initialized with limit:\00", align 1
@.str.8 = private unnamed_addr constant [6 x i8] c"range\00", align 1
@0 = private unnamed_addr constant [7 x i8] c"append\00", align 1
@.str.9 = private unnamed_addr constant [20 x i8] c"Total primes found:\00", align 1
@.str.10 = private unnamed_addr constant [4 x i8] c"len\00", align 1
@.str.11 = private unnamed_addr constant [8 x i8] c"to_list\00", align 1
@.str.12 = private unnamed_addr constant [13 x i8] c"omit_squares\00", align 1
@.str.13 = private unnamed_addr constant [6 x i8] c"step1\00", align 1
@.str.14 = private unnamed_addr constant [6 x i8] c"step2\00", align 1
@.str.15 = private unnamed_addr constant [6 x i8] c"step3\00", align 1
@1 = private unnamed_addr constant [6 x i8] c"step1\00", align 1
@2 = private unnamed_addr constant [6 x i8] c"step2\00", align 1
@3 = private unnamed_addr constant [6 x i8] c"step3\00", align 1
@.str.16 = private unnamed_addr constant [7 x i8] c"loop_y\00", align 1
@.str.17 = private unnamed_addr constant [23 x i8] c"Processing loop_x, x =\00", align 1
@4 = private unnamed_addr constant [7 x i8] c"loop_y\00", align 1
@.str.18 = private unnamed_addr constant [7 x i8] c"loop_x\00", align 1
@.str.19 = private unnamed_addr constant [30 x i8] c"Starting sieve calculation...\00", align 1
@5 = private unnamed_addr constant [7 x i8] c"loop_x\00", align 1
@.str.20 = private unnamed_addr constant [42 x i8] c"Sieve loops finished, omitting squares...\00", align 1
@6 = private unnamed_addr constant [13 x i8] c"omit_squares\00", align 1
@.str.21 = private unnamed_addr constant [5 x i8] c"calc\00", align 1
@.str.22 = private unnamed_addr constant [6 x i8] c"Sieve\00", align 1
@.str.23 = private unnamed_addr constant [39 x i8] c"Generating trie for primes, list size:\00", align 1
@.str.24 = private unnamed_addr constant [4 x i8] c"str\00", align 1
@.str.25 = private unnamed_addr constant [26 x i8] c"Trie generation complete.\00", align 1
@.str.26 = private unnamed_addr constant [14 x i8] c"generate_trie\00", align 1
@7 = private unnamed_addr constant [5 x i8] c"calc\00", align 1
@.str.27 = private unnamed_addr constant [22 x i8] c"Searching for prefix:\00", align 1
@8 = private unnamed_addr constant [8 x i8] c"to_list\00", align 1
@.str.28 = private unnamed_addr constant [27 x i8] c"Base primes check for 100:\00", align 1
@.str.29 = private unnamed_addr constant [29 x i8] c"Navigating to prefix node...\00", align 1
@.str.30 = private unnamed_addr constant [21 x i8] c"Link check for char:\00", align 1
@9 = private unnamed_addr constant [4 x i8] c"get\00", align 1
@.str.31 = private unnamed_addr constant [27 x i8] c"Prefix node break at char:\00", align 1
@.str.32 = private unnamed_addr constant [50 x i8] c"Prefix node found. Terminal state of prefix node:\00", align 1
@.str.33 = private unnamed_addr constant [46 x i8] c"Starting queue traversal. Initial queue size:\00", align 1
@10 = private unnamed_addr constant [4 x i8] c"pop\00", align 1
@.str.34 = private unnamed_addr constant [11 x i8] c"Queue pop:\00", align 1
@.str.35 = private unnamed_addr constant [10 x i8] c"Terminal:\00", align 1
@.str.36 = private unnamed_addr constant [4 x i8] c"int\00", align 1
@11 = private unnamed_addr constant [7 x i8] c"append\00", align 1
@12 = private unnamed_addr constant [5 x i8] c"keys\00", align 1
@.str.37 = private unnamed_addr constant [24 x i8] c"Found children chars at\00", align 1
@.str.38 = private unnamed_addr constant [2 x i8] c":\00", align 1
@.str.39 = private unnamed_addr constant [5 x i8] c"list\00", align 1
@13 = private unnamed_addr constant [6 x i8] c"items\00", align 1
@.str.40 = private unnamed_addr constant [18 x i8] c"Pushing to queue:\00", align 1
@14 = private unnamed_addr constant [7 x i8] c"insert\00", align 1
@15 = private unnamed_addr constant [5 x i8] c"sort\00", align 1
@.str.41 = private unnamed_addr constant [31 x i8] c"Find completed. Results count:\00", align 1
@.str.42 = private unnamed_addr constant [8 x i8] c"Values:\00", align 1
@.str.43 = private unnamed_addr constant [5 x i8] c"find\00", align 1
@.str.44 = private unnamed_addr constant [20 x i8] c"Running verify()...\00", align 1
@.str.45 = private unnamed_addr constant [22 x i8] c"Verify results match:\00", align 1
@.str.46 = private unnamed_addr constant [10 x i8] c"Expected:\00", align 1
@.str.47 = private unnamed_addr constant [8 x i8] c"Actual:\00", align 1
@.str.48 = private unnamed_addr constant [7 x i8] c"verify\00", align 1
@.str.49 = private unnamed_addr constant [41 x i8] c"Starting main calculation (UPPER_BOUND =\00", align 1
@.str.50 = private unnamed_addr constant [2 x i8] c")\00", align 1
@.str.51 = private unnamed_addr constant [8 x i8] c"results\00", align 1
@.str.52 = private unnamed_addr constant [15 x i8] c"Final Results:\00", align 1
@.str.53 = private unnamed_addr constant [6 x i8] c"prime\00", align 1
@.str.54 = private unnamed_addr constant [6 x i8] c"limit\00", align 1
@.str.55 = private unnamed_addr constant [9 x i8] c"terminal\00", align 1
@.str.56 = private unnamed_addr constant [9 x i8] c"children\00", align 1

; Function Attrs: uwtable(sync)
define void @PyInit_test() #0 {
entry:
  %raw_args2 = alloca [2 x ptr], align 8
  %raw_args1 = alloca [2 x ptr], align 8
  %raw_args = alloca [3 x ptr], align 8
  %0 = call ptr @_Z13rt_add_modulePKc(ptr @.str)
  %1 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.53, i64 5)
  store ptr %1, ptr @.pystr_obj.prime, align 8
  %2 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.54, i64 5)
  store ptr %2, ptr @.pystr_obj.limit, align 8
  %3 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.55, i64 8)
  store ptr %3, ptr @.pystr_obj.terminal, align 8
  %4 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.56, i64 8)
  store ptr %4, ptr @.pystr_obj.children, align 8
  %pystr_init_anchor = alloca ptr, align 8
  %5 = call ptr @_Z19rt_integer_from_i64l(i64 5000000)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.1, ptr %5)
  %6 = call ptr @_Z19rt_integer_from_i64l(i64 32338)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.2, ptr %6)
  %7 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.5, ptr @"test.<module>.0:0.Node.3:0.__body__", ptr %0, ptr null, ptr null, ptr null)
  %8 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %9 = call ptr @_Z18rt_build_class_aotPN2py8PyObjectEPKcS1_S1_(ptr %7, ptr @.str.5, ptr %8, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.5, ptr %9)
  %10 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.22, ptr @"test.<module>.0:0.Sieve.8:0.__body__", ptr %0, ptr null, ptr null, ptr null)
  %11 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 0, ptr null)
  %12 = call ptr @_Z18rt_build_class_aotPN2py8PyObjectEPKcS1_S1_(ptr %10, ptr @.str.22, ptr %11, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.22, ptr %12)
  %13 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.26, ptr @"test.<module>.0:0.generate_trie.71:0", ptr %0, ptr null, ptr null, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.26, ptr %13)
  %14 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.43, ptr @"test.<module>.0:0.find.86:0", ptr %0, ptr null, ptr null, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.43, ptr %14)
  %15 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.48, ptr @"test.<module>.0:0.verify.132:0", ptr %0, ptr null, ptr null, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.48, ptr %15)
  %16 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.48)
  %17 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %16, ptr null, i32 0, ptr null)
  %18 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.6)
  %19 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.49, i64 40)
  %20 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.1)
  %21 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.50, i64 1)
  %22 = getelementptr [3 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %19, ptr %22, align 8
  %23 = getelementptr [3 x ptr], ptr %raw_args, i32 0, i32 1
  store ptr %20, ptr %23, align 8
  %24 = getelementptr [3 x ptr], ptr %raw_args, i32 0, i32 2
  store ptr %21, ptr %24, align 8
  %25 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %18, ptr %raw_args, i32 3, ptr null)
  %26 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.43)
  %27 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.1)
  %28 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.2)
  %29 = getelementptr [2 x ptr], ptr %raw_args1, i32 0, i32 0
  store ptr %27, ptr %29, align 8
  %30 = getelementptr [2 x ptr], ptr %raw_args1, i32 0, i32 1
  store ptr %28, ptr %30, align 8
  %31 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %26, ptr %raw_args1, i32 2, ptr null)
  call void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr %0, ptr @.str.51, ptr %31)
  %32 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.6)
  %33 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.52, i64 14)
  %34 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %0, ptr @.str.51)
  %35 = getelementptr [2 x ptr], ptr %raw_args2, i32 0, i32 0
  store ptr %33, ptr %35, align 8
  %36 = getelementptr [2 x ptr], ptr %raw_args2, i32 0, i32 1
  store ptr %34, ptr %36, align 8
  %37 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %32, ptr %raw_args2, i32 2, ptr null)
  ret void
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_add_modulePKc(ptr noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z19rt_integer_from_i64l(i64 noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z15rt_store_globalPN2py8PyObjectEPKcS1_(ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Node.3:0.__body__"(ptr %module, ptr %closure, ptr %args_ptr, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %__class__.cell = alloca ptr, align 8
  store ptr null, ptr %__class__.cell, align 8
  %ns = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_ptr, i32 0)
  %0 = call ptr @_Z14rt_create_cellPN2py8PyObjectE(ptr null)
  store ptr %0, ptr %__class__.cell, align 8
  %1 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.3, ptr @"test.<module>.0:0.Node.3:0.__init__.4:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.3, ptr %1)
  %__class__.cell.load = load ptr, ptr %__class__.cell, align 8
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.4, ptr %__class__.cell.load)
  %2 = call ptr @_Z7rt_nonev()
  ret ptr %2
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr noundef, i32 noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_create_cellPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Node.3:0.__init__.4:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z13rt_build_dictiPPN2py8PyObjectES2_(i32 0, ptr null, ptr null)
  %self1 = load ptr, ptr %self, align 8
  %2 = load ptr, ptr @.pystr_obj.children, align 8
  call void @_Z15rt_setattr_fastPN2py8PyObjectES1_S1_(ptr %self1, ptr %2, ptr %1)
  %3 = call ptr @_Z8rt_falsev()
  %self2 = load ptr, ptr %self, align 8
  %4 = load ptr, ptr @.pystr_obj.terminal, align 8
  call void @_Z15rt_setattr_fastPN2py8PyObjectES1_S1_(ptr %self2, ptr %4, ptr %3)
  %5 = call ptr @_Z7rt_nonev()
  ret ptr %5
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_build_dictiPPN2py8PyObjectES2_(i32 noundef, ptr nocapture noundef readonly, ptr nocapture noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare void @_Z15rt_setattr_fastPN2py8PyObjectES1_S1_(ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z8rt_falsev() #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z7rt_nonev() #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr noundef readonly, ptr noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 noundef, ptr nocapture noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z18rt_build_class_aotPN2py8PyObjectEPKcS1_S1_(ptr noundef, ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.__body__"(ptr %module, ptr %closure, ptr %args_ptr, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %__class__.cell = alloca ptr, align 8
  store ptr null, ptr %__class__.cell, align 8
  %ns = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_ptr, i32 0)
  %0 = call ptr @_Z14rt_create_cellPN2py8PyObjectE(ptr null)
  store ptr %0, ptr %__class__.cell, align 8
  %1 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.3, ptr @"test.<module>.0:0.Sieve.8:0.__init__.9:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.3, ptr %1)
  %2 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.11, ptr @"test.<module>.0:0.Sieve.8:0.to_list.14:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.11, ptr %2)
  %3 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.12, ptr @"test.<module>.0:0.Sieve.8:0.omit_squares.22:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.12, ptr %3)
  %4 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.13, ptr @"test.<module>.0:0.Sieve.8:0.step1.33:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.13, ptr %4)
  %5 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.14, ptr @"test.<module>.0:0.Sieve.8:0.step2.38:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.14, ptr %5)
  %6 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.15, ptr @"test.<module>.0:0.Sieve.8:0.step3.43:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.15, ptr %6)
  %7 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.16, ptr @"test.<module>.0:0.Sieve.8:0.loop_y.48:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.16, ptr %7)
  %8 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.18, ptr @"test.<module>.0:0.Sieve.8:0.loop_x.56:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.18, ptr %8)
  %9 = call ptr @_Z16rt_make_functionPKcPvPN2py8PyObjectES4_S4_S4_(ptr @.str.21, ptr @"test.<module>.0:0.Sieve.8:0.calc.64:4", ptr %module, ptr null, ptr null, ptr null)
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.21, ptr %9)
  %__class__.cell.load = load ptr, ptr %__class__.cell, align 8
  call void @_Z19rt_dict_setitem_strPN2py8PyObjectEPKcS1_(ptr %ns, ptr @.str.4, ptr %__class__.cell.load)
  %10 = call ptr @_Z7rt_nonev()
  ret ptr %10
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.__init__.9:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args = alloca [2 x ptr], align 8
  %list_elems = alloca [1 x ptr], align 8
  %limit = alloca ptr, align 8
  store ptr null, ptr %limit, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %limit.has_pos_arg = icmp slt i32 1, %argc
  %1 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 1)
  store ptr %1, ptr %limit, align 8
  %limit1 = load ptr, ptr %limit, align 8
  %self2 = load ptr, ptr %self, align 8
  %2 = load ptr, ptr @.pystr_obj.limit, align 8
  call void @_Z15rt_setattr_fastPN2py8PyObjectES1_S1_(ptr %self2, ptr %2, ptr %limit1)
  %3 = call ptr @_Z8rt_falsev()
  %4 = getelementptr [1 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %3, ptr %4, align 8
  %5 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 1, ptr %list_elems)
  %limit3 = load ptr, ptr %limit, align 8
  %6 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %7 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %limit3, ptr %6)
  %8 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %5, ptr %7)
  %self4 = load ptr, ptr %self, align 8
  %9 = load ptr, ptr @.pystr_obj.prime, align 8
  call void @_Z15rt_setattr_fastPN2py8PyObjectES1_S1_(ptr %self4, ptr %9, ptr %8)
  %10 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %11 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.7, i64 29)
  %limit5 = load ptr, ptr %limit, align 8
  %12 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %11, ptr %12, align 8
  %13 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 1
  store ptr %limit5, ptr %13, align 8
  %14 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %10, ptr %raw_args, i32 2, ptr null)
  %15 = call ptr @_Z7rt_nonev()
  ret ptr %15
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 noundef, ptr nocapture noundef readonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z19rt_string_from_cstrPKcl(ptr noundef readonly, i64 noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr noundef, ptr nocapture noundef readonly, i32 noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.to_list.14:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args9 = alloca [2 x ptr], align 8
  %raw_args8 = alloca [1 x ptr], align 8
  %raw_args6 = alloca [1 x ptr], align 8
  %p = alloca ptr, align 8
  store ptr null, ptr %p, align 8
  %for_has_value = alloca i1, align 1
  store i1 false, ptr %for_has_value, align 1
  %raw_args = alloca [2 x ptr], align 8
  %result = alloca ptr, align 8
  store ptr null, ptr %result, align 8
  %list_elems = alloca [2 x ptr], align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %2 = call ptr @_Z19rt_integer_from_i64l(i64 3)
  %3 = getelementptr [2 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %1, ptr %3, align 8
  %4 = getelementptr [2 x ptr], ptr %list_elems, i32 0, i32 1
  store ptr %2, ptr %4, align 8
  %5 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 2, ptr %list_elems)
  store ptr %5, ptr %result, align 8
  %6 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.8)
  %7 = call ptr @_Z19rt_integer_from_i64l(i64 5)
  %self1 = load ptr, ptr %self, align 8
  %8 = load ptr, ptr @.pystr_obj.limit, align 8
  %9 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self1, ptr %8)
  %10 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %11 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %9, ptr %10)
  %12 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %7, ptr %12, align 8
  %13 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 1
  store ptr %11, ptr %13, align 8
  %14 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %6, ptr %raw_args, i32 2, ptr null)
  %15 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %14)
  br label %for.cond

for.cond:                                         ; preds = %if.merge, %entry
  %16 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %15, ptr %for_has_value)
  %17 = load i1, ptr %for_has_value, align 1
  br i1 %17, label %for.body, label %for.merge

for.body:                                         ; preds = %for.cond
  store ptr %16, ptr %p, align 8
  %self2 = load ptr, ptr %self, align 8
  %18 = load ptr, ptr @.pystr_obj.prime, align 8
  %19 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self2, ptr %18)
  %p3 = load ptr, ptr %p, align 8
  %20 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %19, ptr %p3)
  %21 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %20)
  br i1 %21, label %if.then, label %if.merge

for.merge:                                        ; preds = %for.cond
  %22 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %23 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.9, i64 19)
  %24 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %result7 = load ptr, ptr %result, align 8
  %25 = getelementptr [1 x ptr], ptr %raw_args8, i32 0, i32 0
  store ptr %result7, ptr %25, align 8
  %26 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %24, ptr %raw_args8, i32 1, ptr null)
  %27 = getelementptr [2 x ptr], ptr %raw_args9, i32 0, i32 0
  store ptr %23, ptr %27, align 8
  %28 = getelementptr [2 x ptr], ptr %raw_args9, i32 0, i32 1
  store ptr %26, ptr %28, align 8
  %29 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %22, ptr %raw_args9, i32 2, ptr null)
  %result10 = load ptr, ptr %result, align 8
  ret ptr %result10

if.then:                                          ; preds = %for.body
  %result4 = load ptr, ptr %result, align 8
  %p5 = load ptr, ptr %p, align 8
  %30 = getelementptr [1 x ptr], ptr %raw_args6, i32 0, i32 0
  store ptr %p5, ptr %30, align 8
  %31 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %result4, ptr @0, ptr %raw_args6, i32 1, ptr null)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %for.body
  br label %for.cond
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr noundef, ptr nocapture noundef writeonly) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef zeroext i1 @_Z10rt_is_truePN2py8PyObjectE(ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr noundef, ptr noundef, ptr nocapture noundef readonly, i32 noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.omit_squares.22:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %i = alloca ptr, align 8
  store ptr null, ptr %i, align 8
  %r = alloca ptr, align 8
  store ptr null, ptr %r, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 5)
  store ptr %1, ptr %r, align 8
  br label %while.cond

while.cond:                                       ; preds = %if.merge, %entry
  %r1 = load ptr, ptr %r, align 8
  %r2 = load ptr, ptr %r, align 8
  %2 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %r1, ptr %r2)
  %self3 = load ptr, ptr %self, align 8
  %3 = load ptr, ptr @.pystr_obj.limit, align 8
  %4 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self3, ptr %3)
  %5 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %2, ptr %4)
  %6 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %5)
  br i1 %6, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %self4 = load ptr, ptr %self, align 8
  %7 = load ptr, ptr @.pystr_obj.prime, align 8
  %8 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self4, ptr %7)
  %r5 = load ptr, ptr %r, align 8
  %9 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %8, ptr %r5)
  %10 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %9)
  br i1 %10, label %if.then, label %if.merge

while.merge:                                      ; preds = %while.cond
  %self19 = load ptr, ptr %self, align 8
  ret ptr %self19

if.then:                                          ; preds = %while.body
  %r6 = load ptr, ptr %r, align 8
  %r7 = load ptr, ptr %r, align 8
  %11 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %r6, ptr %r7)
  store ptr %11, ptr %i, align 8
  br label %while.cond8

if.merge:                                         ; preds = %while.merge10, %while.body
  %12 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %r18 = load ptr, ptr %r, align 8
  %13 = call ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr %r18, ptr %12)
  store ptr %13, ptr %r, align 8
  br label %while.cond

while.cond8:                                      ; preds = %while.body9, %if.then
  %i11 = load ptr, ptr %i, align 8
  %self12 = load ptr, ptr %self, align 8
  %14 = load ptr, ptr @.pystr_obj.limit, align 8
  %15 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self12, ptr %14)
  %16 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %i11, ptr %15)
  %17 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %16)
  br i1 %17, label %while.body9, label %while.merge10

while.body9:                                      ; preds = %while.cond8
  %18 = call ptr @_Z8rt_falsev()
  %self13 = load ptr, ptr %self, align 8
  %19 = load ptr, ptr @.pystr_obj.prime, align 8
  %20 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self13, ptr %19)
  %i14 = load ptr, ptr %i, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %20, ptr %i14, ptr %18)
  %i15 = load ptr, ptr %i, align 8
  %r16 = load ptr, ptr %r, align 8
  %r17 = load ptr, ptr %r, align 8
  %21 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %r16, ptr %r17)
  %22 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %i15, ptr %21)
  store ptr %22, ptr %i, align 8
  br label %while.cond8

while.merge10:                                    ; preds = %while.cond8
  br label %if.merge
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.step1.33:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
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
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %x.has_pos_arg = icmp slt i32 1, %argc
  %1 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 1)
  store ptr %1, ptr %x, align 8
  %y.has_pos_arg = icmp slt i32 2, %argc
  %2 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 2)
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
  %8 = load ptr, ptr @.pystr_obj.limit, align 8
  %9 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self6, ptr %8)
  %10 = call ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr %n5, ptr %9)
  store ptr %10, ptr %.bool_result, align 8
  %11 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %10)
  br i1 %11, label %boolop.next, label %boolop.merge

boolop.merge:                                     ; preds = %boolop.merge8, %entry
  %boolop.result12 = load ptr, ptr %.bool_result, align 8
  %12 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %boolop.result12)
  br i1 %12, label %if.then, label %if.merge

boolop.next:                                      ; preds = %entry
  %n9 = load ptr, ptr %n, align 8
  %13 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %14 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n9, ptr %13)
  %15 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %16 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %14, ptr %15)
  store ptr %16, ptr %.bool_result7, align 8
  %17 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %16)
  br i1 %17, label %boolop.merge8, label %boolop.next10

boolop.merge8:                                    ; preds = %boolop.next10, %boolop.next
  %boolop.result = load ptr, ptr %.bool_result7, align 8
  store ptr %boolop.result, ptr %.bool_result, align 8
  br label %boolop.merge

boolop.next10:                                    ; preds = %boolop.next
  %n11 = load ptr, ptr %n, align 8
  %18 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %19 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n11, ptr %18)
  %20 = call ptr @_Z19rt_integer_from_i64l(i64 5)
  %21 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %19, ptr %20)
  store ptr %21, ptr %.bool_result7, align 8
  br label %boolop.merge8

if.then:                                          ; preds = %boolop.merge
  %self13 = load ptr, ptr %self, align 8
  %22 = load ptr, ptr @.pystr_obj.prime, align 8
  %23 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self13, ptr %22)
  %n14 = load ptr, ptr %n, align 8
  %24 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %23, ptr %n14)
  %25 = call ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr %24)
  %self15 = load ptr, ptr %self, align 8
  %26 = load ptr, ptr @.pystr_obj.prime, align 8
  %27 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self15, ptr %26)
  %n16 = load ptr, ptr %n, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %27, ptr %n16, ptr %25)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %boolop.merge
  %28 = call ptr @_Z7rt_nonev()
  ret ptr %28
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.step2.38:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
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
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %x.has_pos_arg = icmp slt i32 1, %argc
  %1 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 1)
  store ptr %1, ptr %x, align 8
  %y.has_pos_arg = icmp slt i32 2, %argc
  %2 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 2)
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
  %8 = load ptr, ptr @.pystr_obj.limit, align 8
  %9 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self6, ptr %8)
  %10 = call ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr %n5, ptr %9)
  store ptr %10, ptr %.bool_result, align 8
  %11 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %10)
  br i1 %11, label %boolop.next, label %boolop.merge

boolop.merge:                                     ; preds = %boolop.next, %entry
  %boolop.result = load ptr, ptr %.bool_result, align 8
  %12 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %boolop.result)
  br i1 %12, label %if.then, label %if.merge

boolop.next:                                      ; preds = %entry
  %n7 = load ptr, ptr %n, align 8
  %13 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %14 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n7, ptr %13)
  %15 = call ptr @_Z19rt_integer_from_i64l(i64 7)
  %16 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %14, ptr %15)
  store ptr %16, ptr %.bool_result, align 8
  br label %boolop.merge

if.then:                                          ; preds = %boolop.merge
  %self8 = load ptr, ptr %self, align 8
  %17 = load ptr, ptr @.pystr_obj.prime, align 8
  %18 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self8, ptr %17)
  %n9 = load ptr, ptr %n, align 8
  %19 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %18, ptr %n9)
  %20 = call ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr %19)
  %self10 = load ptr, ptr %self, align 8
  %21 = load ptr, ptr @.pystr_obj.prime, align 8
  %22 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self10, ptr %21)
  %n11 = load ptr, ptr %n, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %22, ptr %n11, ptr %20)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %boolop.merge
  %23 = call ptr @_Z7rt_nonev()
  ret ptr %23
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.step3.43:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
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
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %x.has_pos_arg = icmp slt i32 1, %argc
  %1 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 1)
  store ptr %1, ptr %x, align 8
  %y.has_pos_arg = icmp slt i32 2, %argc
  %2 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 2)
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
  %11 = load ptr, ptr @.pystr_obj.limit, align 8
  %12 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self8, ptr %11)
  %13 = call ptr @_Z13rt_compare_lePN2py8PyObjectES1_(ptr %n7, ptr %12)
  store ptr %13, ptr %.bool_result, align 8
  %14 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %13)
  br i1 %14, label %boolop.next9, label %boolop.merge

boolop.next9:                                     ; preds = %boolop.next
  %n10 = load ptr, ptr %n, align 8
  %15 = call ptr @_Z19rt_integer_from_i64l(i64 12)
  %16 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %n10, ptr %15)
  %17 = call ptr @_Z19rt_integer_from_i64l(i64 11)
  %18 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %16, ptr %17)
  store ptr %18, ptr %.bool_result, align 8
  br label %boolop.merge

if.then:                                          ; preds = %boolop.merge
  %self11 = load ptr, ptr %self, align 8
  %19 = load ptr, ptr @.pystr_obj.prime, align 8
  %20 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self11, ptr %19)
  %n12 = load ptr, ptr %n, align 8
  %21 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %20, ptr %n12)
  %22 = call ptr @_Z12rt_unary_notPN2py8PyObjectE(ptr %21)
  %self13 = load ptr, ptr %self, align 8
  %23 = load ptr, ptr @.pystr_obj.prime, align 8
  %24 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self13, ptr %23)
  %n14 = load ptr, ptr %n, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %24, ptr %n14, ptr %22)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %boolop.merge
  %25 = call ptr @_Z7rt_nonev()
  ret ptr %25
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_binary_subPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_gtPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.loop_y.48:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args14 = alloca [2 x ptr], align 8
  %raw_args10 = alloca [2 x ptr], align 8
  %raw_args = alloca [2 x ptr], align 8
  %y = alloca ptr, align 8
  store ptr null, ptr %y, align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %x.has_pos_arg = icmp slt i32 1, %argc
  %1 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 1)
  store ptr %1, ptr %x, align 8
  %2 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  store ptr %2, ptr %y, align 8
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %y1 = load ptr, ptr %y, align 8
  %y2 = load ptr, ptr %y, align 8
  %3 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %y1, ptr %y2)
  %self3 = load ptr, ptr %self, align 8
  %4 = load ptr, ptr @.pystr_obj.limit, align 8
  %5 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self3, ptr %4)
  %6 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %3, ptr %5)
  %7 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %6)
  br i1 %7, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %self4 = load ptr, ptr %self, align 8
  %x5 = load ptr, ptr %x, align 8
  %y6 = load ptr, ptr %y, align 8
  %8 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %x5, ptr %8, align 8
  %9 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 1
  store ptr %y6, ptr %9, align 8
  %10 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %self4, ptr @1, ptr %raw_args, i32 2, ptr null)
  %self7 = load ptr, ptr %self, align 8
  %x8 = load ptr, ptr %x, align 8
  %y9 = load ptr, ptr %y, align 8
  %11 = getelementptr [2 x ptr], ptr %raw_args10, i32 0, i32 0
  store ptr %x8, ptr %11, align 8
  %12 = getelementptr [2 x ptr], ptr %raw_args10, i32 0, i32 1
  store ptr %y9, ptr %12, align 8
  %13 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %self7, ptr @2, ptr %raw_args10, i32 2, ptr null)
  %self11 = load ptr, ptr %self, align 8
  %x12 = load ptr, ptr %x, align 8
  %y13 = load ptr, ptr %y, align 8
  %14 = getelementptr [2 x ptr], ptr %raw_args14, i32 0, i32 0
  store ptr %x12, ptr %14, align 8
  %15 = getelementptr [2 x ptr], ptr %raw_args14, i32 0, i32 1
  store ptr %y13, ptr %15, align 8
  %16 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %self11, ptr @3, ptr %raw_args14, i32 2, ptr null)
  %17 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %y15 = load ptr, ptr %y, align 8
  %18 = call ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr %y15, ptr %17)
  store ptr %18, ptr %y, align 8
  br label %while.cond

while.merge:                                      ; preds = %while.cond
  %19 = call ptr @_Z7rt_nonev()
  ret ptr %19
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.loop_x.56:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args8 = alloca [1 x ptr], align 8
  %raw_args = alloca [2 x ptr], align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  store ptr %1, ptr %x, align 8
  br label %while.cond

while.cond:                                       ; preds = %if.merge, %entry
  %x1 = load ptr, ptr %x, align 8
  %x2 = load ptr, ptr %x, align 8
  %2 = call ptr @_Z13rt_binary_mulPN2py8PyObjectES1_(ptr %x1, ptr %x2)
  %self3 = load ptr, ptr %self, align 8
  %3 = load ptr, ptr @.pystr_obj.limit, align 8
  %4 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %self3, ptr %3)
  %5 = call ptr @_Z13rt_compare_ltPN2py8PyObjectES1_(ptr %2, ptr %4)
  %6 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %5)
  br i1 %6, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %x4 = load ptr, ptr %x, align 8
  %7 = call ptr @_Z19rt_integer_from_i64l(i64 500)
  %8 = call ptr @_Z13rt_binary_modPN2py8PyObjectES1_(ptr %x4, ptr %7)
  %9 = call ptr @_Z19rt_integer_from_i64l(i64 0)
  %10 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %8, ptr %9)
  %11 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %10)
  br i1 %11, label %if.then, label %if.merge

while.merge:                                      ; preds = %while.cond
  %12 = call ptr @_Z7rt_nonev()
  ret ptr %12

if.then:                                          ; preds = %while.body
  %13 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %14 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.17, i64 22)
  %x5 = load ptr, ptr %x, align 8
  %15 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %14, ptr %15, align 8
  %16 = getelementptr [2 x ptr], ptr %raw_args, i32 0, i32 1
  store ptr %x5, ptr %16, align 8
  %17 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %13, ptr %raw_args, i32 2, ptr null)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %while.body
  %self6 = load ptr, ptr %self, align 8
  %x7 = load ptr, ptr %x, align 8
  %18 = getelementptr [1 x ptr], ptr %raw_args8, i32 0, i32 0
  store ptr %x7, ptr %18, align 8
  %19 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %self6, ptr @4, ptr %raw_args8, i32 1, ptr null)
  %20 = call ptr @_Z19rt_integer_from_i64l(i64 1)
  %x9 = load ptr, ptr %x, align 8
  %21 = call ptr @_Z14rt_inplace_addPN2py8PyObjectES1_(ptr %x9, ptr %20)
  store ptr %21, ptr %x, align 8
  br label %while.cond
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.Sieve.8:0.calc.64:4"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args2 = alloca [1 x ptr], align 8
  %raw_args = alloca [1 x ptr], align 8
  %self = alloca ptr, align 8
  store ptr null, ptr %self, align 8
  %self.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %self, align 8
  %1 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %2 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.19, i64 29)
  %3 = getelementptr [1 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %2, ptr %3, align 8
  %4 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %1, ptr %raw_args, i32 1, ptr null)
  %self1 = load ptr, ptr %self, align 8
  %5 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %self1, ptr @5, ptr null, i32 0, ptr null)
  %6 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %7 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.20, i64 41)
  %8 = getelementptr [1 x ptr], ptr %raw_args2, i32 0, i32 0
  store ptr %7, ptr %8, align 8
  %9 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %6, ptr %raw_args2, i32 1, ptr null)
  %self3 = load ptr, ptr %self, align 8
  %10 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %self3, ptr @6, ptr null, i32 0, ptr null)
  ret ptr %10
}

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.generate_trie.71:0"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args19 = alloca [1 x ptr], align 8
  %ch = alloca ptr, align 8
  store ptr null, ptr %ch, align 8
  %for_has_value11 = alloca i1, align 1
  store i1 false, ptr %for_has_value11, align 1
  %head = alloca ptr, align 8
  store ptr null, ptr %head, align 8
  %s_el = alloca ptr, align 8
  store ptr null, ptr %s_el, align 8
  %raw_args5 = alloca [1 x ptr], align 8
  %el = alloca ptr, align 8
  store ptr null, ptr %el, align 8
  %for_has_value = alloca i1, align 1
  store i1 false, ptr %for_has_value, align 1
  %root = alloca ptr, align 8
  store ptr null, ptr %root, align 8
  %raw_args2 = alloca [2 x ptr], align 8
  %raw_args = alloca [1 x ptr], align 8
  %l = alloca ptr, align 8
  store ptr null, ptr %l, align 8
  %l.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %l, align 8
  %1 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %2 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.23, i64 38)
  %3 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %l1 = load ptr, ptr %l, align 8
  %4 = getelementptr [1 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %l1, ptr %4, align 8
  %5 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %3, ptr %raw_args, i32 1, ptr null)
  %6 = getelementptr [2 x ptr], ptr %raw_args2, i32 0, i32 0
  store ptr %2, ptr %6, align 8
  %7 = getelementptr [2 x ptr], ptr %raw_args2, i32 0, i32 1
  store ptr %5, ptr %7, align 8
  %8 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %1, ptr %raw_args2, i32 2, ptr null)
  %9 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.5)
  %10 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %9, ptr null, i32 0, ptr null)
  store ptr %10, ptr %root, align 8
  %l3 = load ptr, ptr %l, align 8
  %11 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %l3)
  br label %for.cond

for.cond:                                         ; preds = %for.merge10, %entry
  %12 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %11, ptr %for_has_value)
  %13 = load i1, ptr %for_has_value, align 1
  br i1 %13, label %for.body, label %for.merge

for.body:                                         ; preds = %for.cond
  store ptr %12, ptr %el, align 8
  %14 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.24)
  %el4 = load ptr, ptr %el, align 8
  %15 = getelementptr [1 x ptr], ptr %raw_args5, i32 0, i32 0
  store ptr %el4, ptr %15, align 8
  %16 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %14, ptr %raw_args5, i32 1, ptr null)
  store ptr %16, ptr %s_el, align 8
  %root6 = load ptr, ptr %root, align 8
  store ptr %root6, ptr %head, align 8
  %s_el7 = load ptr, ptr %s_el, align 8
  %17 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %s_el7)
  br label %for.cond8

for.merge:                                        ; preds = %for.cond
  %18 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %19 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.25, i64 25)
  %20 = getelementptr [1 x ptr], ptr %raw_args19, i32 0, i32 0
  store ptr %19, ptr %20, align 8
  %21 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %18, ptr %raw_args19, i32 1, ptr null)
  %root20 = load ptr, ptr %root, align 8
  ret ptr %root20

for.cond8:                                        ; preds = %if.merge, %for.body
  %22 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %17, ptr %for_has_value11)
  %23 = load i1, ptr %for_has_value11, align 1
  br i1 %23, label %for.body9, label %for.merge10

for.body9:                                        ; preds = %for.cond8
  store ptr %22, ptr %ch, align 8
  %ch12 = load ptr, ptr %ch, align 8
  %head13 = load ptr, ptr %head, align 8
  %24 = load ptr, ptr @.pystr_obj.children, align 8
  %25 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %head13, ptr %24)
  %26 = call ptr @_Z17rt_compare_not_inPN2py8PyObjectES1_(ptr %ch12, ptr %25)
  %27 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %26)
  br i1 %27, label %if.then, label %if.merge

for.merge10:                                      ; preds = %for.cond8
  %28 = call ptr @_Z7rt_truev()
  %head18 = load ptr, ptr %head, align 8
  %29 = load ptr, ptr @.pystr_obj.terminal, align 8
  call void @_Z15rt_setattr_fastPN2py8PyObjectES1_S1_(ptr %head18, ptr %29, ptr %28)
  br label %for.cond

if.then:                                          ; preds = %for.body9
  %30 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.5)
  %31 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %30, ptr null, i32 0, ptr null)
  %head14 = load ptr, ptr %head, align 8
  %32 = load ptr, ptr @.pystr_obj.children, align 8
  %33 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %head14, ptr %32)
  %ch15 = load ptr, ptr %ch, align 8
  call void @_Z10rt_setitemPN2py8PyObjectES1_S1_(ptr %33, ptr %ch15, ptr %31)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %for.body9
  %head16 = load ptr, ptr %head, align 8
  %34 = load ptr, ptr @.pystr_obj.children, align 8
  %35 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %head16, ptr %34)
  %ch17 = load ptr, ptr %ch, align 8
  %36 = call ptr @_Z10rt_getitemPN2py8PyObjectES1_(ptr %35, ptr %ch17)
  store ptr %36, ptr %head, align 8
  br label %for.cond8
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z17rt_compare_not_inPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z7rt_truev() #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.find.86:0"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args77 = alloca [4 x ptr], align 8
  %raw_args75 = alloca [1 x ptr], align 8
  %raw_args72 = alloca [2 x ptr], align 8
  %tuple_elems70 = alloca [2 x ptr], align 8
  %raw_args66 = alloca [2 x ptr], align 8
  %new_prefix = alloca ptr, align 8
  store ptr null, ptr %new_prefix, align 8
  %v = alloca ptr, align 8
  store ptr null, ptr %v, align 8
  %unpack_arr62 = alloca [2 x ptr], align 8
  %for_has_value61 = alloca i1, align 1
  store i1 false, ptr %for_has_value61, align 1
  %raw_args56 = alloca [4 x ptr], align 8
  %raw_args55 = alloca [1 x ptr], align 8
  %raw_args50 = alloca [1 x ptr], align 8
  %child_chars = alloca ptr, align 8
  store ptr null, ptr %child_chars, align 8
  %raw_args47 = alloca [1 x ptr], align 8
  %raw_args46 = alloca [1 x ptr], align 8
  %raw_args40 = alloca [4 x ptr], align 8
  %current_prefix = alloca ptr, align 8
  store ptr null, ptr %current_prefix, align 8
  %top = alloca ptr, align 8
  store ptr null, ptr %top, align 8
  %unpack_arr37 = alloca [2 x ptr], align 8
  %raw_args34 = alloca [2 x ptr], align 8
  %raw_args33 = alloca [1 x ptr], align 8
  %result = alloca ptr, align 8
  store ptr null, ptr %result, align 8
  %queue = alloca ptr, align 8
  store ptr null, ptr %queue, align 8
  %unpack_arr = alloca [2 x ptr], align 8
  %tuple_elems30 = alloca [2 x ptr], align 8
  %list_elems = alloca [1 x ptr], align 8
  %tuple_elems = alloca [2 x ptr], align 8
  %raw_args27 = alloca [2 x ptr], align 8
  %raw_args25 = alloca [2 x ptr], align 8
  %raw_args20 = alloca [1 x ptr], align 8
  %raw_args17 = alloca [2 x ptr], align 8
  %ch = alloca ptr, align 8
  store ptr null, ptr %ch, align 8
  %for_has_value = alloca i1, align 1
  store i1 false, ptr %for_has_value, align 1
  %raw_args14 = alloca [1 x ptr], align 8
  %head = alloca ptr, align 8
  store ptr null, ptr %head, align 8
  %raw_args13 = alloca [1 x ptr], align 8
  %raw_args11 = alloca [4 x ptr], align 8
  %prime_list = alloca ptr, align 8
  store ptr null, ptr %prime_list, align 8
  %raw_args5 = alloca [2 x ptr], align 8
  %str_prefix = alloca ptr, align 8
  store ptr null, ptr %str_prefix, align 8
  %raw_args3 = alloca [1 x ptr], align 8
  %primes = alloca ptr, align 8
  store ptr null, ptr %primes, align 8
  %raw_args = alloca [1 x ptr], align 8
  %prefix = alloca ptr, align 8
  store ptr null, ptr %prefix, align 8
  %upper_bound = alloca ptr, align 8
  store ptr null, ptr %upper_bound, align 8
  %upper_bound.has_pos_arg = icmp slt i32 0, %argc
  %0 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 0)
  store ptr %0, ptr %upper_bound, align 8
  %prefix.has_pos_arg = icmp slt i32 1, %argc
  %1 = call ptr @_Z18rt_value_array_getPKSt7variantIJN2py6NumberENS0_6StringENS0_5BytesENS0_8EllipsisENS0_12NameConstantENS0_5TupleEPNS0_8PyObjectEEEi(ptr %args_array, i32 1)
  store ptr %1, ptr %prefix, align 8
  %2 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.22)
  %upper_bound1 = load ptr, ptr %upper_bound, align 8
  %3 = getelementptr [1 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %upper_bound1, ptr %3, align 8
  %4 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %2, ptr %raw_args, i32 1, ptr null)
  %5 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %4, ptr @7, ptr null, i32 0, ptr null)
  store ptr %5, ptr %primes, align 8
  %6 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.24)
  %prefix2 = load ptr, ptr %prefix, align 8
  %7 = getelementptr [1 x ptr], ptr %raw_args3, i32 0, i32 0
  store ptr %prefix2, ptr %7, align 8
  %8 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %6, ptr %raw_args3, i32 1, ptr null)
  store ptr %8, ptr %str_prefix, align 8
  %9 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %10 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.27, i64 21)
  %str_prefix4 = load ptr, ptr %str_prefix, align 8
  %11 = getelementptr [2 x ptr], ptr %raw_args5, i32 0, i32 0
  store ptr %10, ptr %11, align 8
  %12 = getelementptr [2 x ptr], ptr %raw_args5, i32 0, i32 1
  store ptr %str_prefix4, ptr %12, align 8
  %13 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %9, ptr %raw_args5, i32 2, ptr null)
  %primes6 = load ptr, ptr %primes, align 8
  %14 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %primes6, ptr @8, ptr null, i32 0, ptr null)
  store ptr %14, ptr %prime_list, align 8
  %upper_bound7 = load ptr, ptr %upper_bound, align 8
  %15 = call ptr @_Z19rt_integer_from_i64l(i64 100)
  %16 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %upper_bound7, ptr %15)
  %17 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %16)
  br i1 %17, label %if.then, label %if.merge

if.then:                                          ; preds = %entry
  %18 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %19 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.28, i64 26)
  %20 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %prime_list8 = load ptr, ptr %prime_list, align 8
  %21 = call ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr %20, ptr %prime_list8)
  %22 = call ptr @_Z19rt_integer_from_i64l(i64 23)
  %prime_list9 = load ptr, ptr %prime_list, align 8
  %23 = call ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr %22, ptr %prime_list9)
  %24 = call ptr @_Z19rt_integer_from_i64l(i64 29)
  %prime_list10 = load ptr, ptr %prime_list, align 8
  %25 = call ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr %24, ptr %prime_list10)
  %26 = getelementptr [4 x ptr], ptr %raw_args11, i32 0, i32 0
  store ptr %19, ptr %26, align 8
  %27 = getelementptr [4 x ptr], ptr %raw_args11, i32 0, i32 1
  store ptr %21, ptr %27, align 8
  %28 = getelementptr [4 x ptr], ptr %raw_args11, i32 0, i32 2
  store ptr %23, ptr %28, align 8
  %29 = getelementptr [4 x ptr], ptr %raw_args11, i32 0, i32 3
  store ptr %25, ptr %29, align 8
  %30 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %18, ptr %raw_args11, i32 4, ptr null)
  br label %if.merge

if.merge:                                         ; preds = %if.then, %entry
  %31 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.26)
  %prime_list12 = load ptr, ptr %prime_list, align 8
  %32 = getelementptr [1 x ptr], ptr %raw_args13, i32 0, i32 0
  store ptr %prime_list12, ptr %32, align 8
  %33 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %31, ptr %raw_args13, i32 1, ptr null)
  store ptr %33, ptr %head, align 8
  %34 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %35 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.29, i64 28)
  %36 = getelementptr [1 x ptr], ptr %raw_args14, i32 0, i32 0
  store ptr %35, ptr %36, align 8
  %37 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %34, ptr %raw_args14, i32 1, ptr null)
  %str_prefix15 = load ptr, ptr %str_prefix, align 8
  %38 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %str_prefix15)
  br label %for.cond

for.cond:                                         ; preds = %if.merge23, %if.merge
  %39 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %38, ptr %for_has_value)
  %40 = load i1, ptr %for_has_value, align 1
  br i1 %40, label %for.body, label %for.merge

for.body:                                         ; preds = %for.cond
  store ptr %39, ptr %ch, align 8
  %41 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %42 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.30, i64 20)
  %ch16 = load ptr, ptr %ch, align 8
  %43 = getelementptr [2 x ptr], ptr %raw_args17, i32 0, i32 0
  store ptr %42, ptr %43, align 8
  %44 = getelementptr [2 x ptr], ptr %raw_args17, i32 0, i32 1
  store ptr %ch16, ptr %44, align 8
  %45 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %41, ptr %raw_args17, i32 2, ptr null)
  %head18 = load ptr, ptr %head, align 8
  %46 = load ptr, ptr @.pystr_obj.children, align 8
  %47 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %head18, ptr %46)
  %ch19 = load ptr, ptr %ch, align 8
  %48 = getelementptr [1 x ptr], ptr %raw_args20, i32 0, i32 0
  store ptr %ch19, ptr %48, align 8
  %49 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %47, ptr @9, ptr %raw_args20, i32 1, ptr null)
  store ptr %49, ptr %head, align 8
  %head21 = load ptr, ptr %head, align 8
  %50 = call ptr @_Z7rt_nonev()
  %51 = call ptr @_Z13rt_compare_isPN2py8PyObjectES1_(ptr %head21, ptr %50)
  %52 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %51)
  br i1 %52, label %if.then22, label %if.merge23

for.merge:                                        ; preds = %for.cond
  %53 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %54 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.32, i64 49)
  %head26 = load ptr, ptr %head, align 8
  %55 = load ptr, ptr @.pystr_obj.terminal, align 8
  %56 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %head26, ptr %55)
  %57 = getelementptr [2 x ptr], ptr %raw_args27, i32 0, i32 0
  store ptr %54, ptr %57, align 8
  %58 = getelementptr [2 x ptr], ptr %raw_args27, i32 0, i32 1
  store ptr %56, ptr %58, align 8
  %59 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %53, ptr %raw_args27, i32 2, ptr null)
  %head28 = load ptr, ptr %head, align 8
  %str_prefix29 = load ptr, ptr %str_prefix, align 8
  %60 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  store ptr %head28, ptr %60, align 8
  %61 = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 1
  store ptr %str_prefix29, ptr %61, align 8
  %arr_ptr = getelementptr [2 x ptr], ptr %tuple_elems, i32 0, i32 0
  %62 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr)
  %63 = getelementptr [1 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %62, ptr %63, align 8
  %64 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 1, ptr %list_elems)
  %65 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 0, ptr null)
  %66 = getelementptr [2 x ptr], ptr %tuple_elems30, i32 0, i32 0
  store ptr %64, ptr %66, align 8
  %67 = getelementptr [2 x ptr], ptr %tuple_elems30, i32 0, i32 1
  store ptr %65, ptr %67, align 8
  %arr_ptr31 = getelementptr [2 x ptr], ptr %tuple_elems30, i32 0, i32 0
  %68 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr31)
  call void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr %68, i32 2, ptr %unpack_arr)
  %69 = getelementptr [2 x ptr], ptr %unpack_arr, i32 0, i32 0
  %70 = load ptr, ptr %69, align 8
  store ptr %70, ptr %queue, align 8
  %71 = getelementptr [2 x ptr], ptr %unpack_arr, i32 0, i32 1
  %72 = load ptr, ptr %71, align 8
  store ptr %72, ptr %result, align 8
  %73 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %74 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.33, i64 45)
  %75 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %queue32 = load ptr, ptr %queue, align 8
  %76 = getelementptr [1 x ptr], ptr %raw_args33, i32 0, i32 0
  store ptr %queue32, ptr %76, align 8
  %77 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %75, ptr %raw_args33, i32 1, ptr null)
  %78 = getelementptr [2 x ptr], ptr %raw_args34, i32 0, i32 0
  store ptr %74, ptr %78, align 8
  %79 = getelementptr [2 x ptr], ptr %raw_args34, i32 0, i32 1
  store ptr %77, ptr %79, align 8
  %80 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %73, ptr %raw_args34, i32 2, ptr null)
  br label %while.cond

if.then22:                                        ; preds = %for.body
  %81 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %82 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.31, i64 26)
  %ch24 = load ptr, ptr %ch, align 8
  %83 = getelementptr [2 x ptr], ptr %raw_args25, i32 0, i32 0
  store ptr %82, ptr %83, align 8
  %84 = getelementptr [2 x ptr], ptr %raw_args25, i32 0, i32 1
  store ptr %ch24, ptr %84, align 8
  %85 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %81, ptr %raw_args25, i32 2, ptr null)
  %86 = call ptr @_Z7rt_nonev()
  ret ptr %86

if.merge23:                                       ; preds = %for.body
  br label %for.cond

while.cond:                                       ; preds = %for.merge60, %for.merge
  %queue35 = load ptr, ptr %queue, align 8
  %87 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %queue35)
  br i1 %87, label %while.body, label %while.merge

while.body:                                       ; preds = %while.cond
  %queue36 = load ptr, ptr %queue, align 8
  %88 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %queue36, ptr @10, ptr null, i32 0, ptr null)
  call void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr %88, i32 2, ptr %unpack_arr37)
  %89 = getelementptr [2 x ptr], ptr %unpack_arr37, i32 0, i32 0
  %90 = load ptr, ptr %89, align 8
  store ptr %90, ptr %top, align 8
  %91 = getelementptr [2 x ptr], ptr %unpack_arr37, i32 0, i32 1
  %92 = load ptr, ptr %91, align 8
  store ptr %92, ptr %current_prefix, align 8
  %93 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %94 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.34, i64 10)
  %current_prefix38 = load ptr, ptr %current_prefix, align 8
  %95 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.35, i64 9)
  %top39 = load ptr, ptr %top, align 8
  %96 = load ptr, ptr @.pystr_obj.terminal, align 8
  %97 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %top39, ptr %96)
  %98 = getelementptr [4 x ptr], ptr %raw_args40, i32 0, i32 0
  store ptr %94, ptr %98, align 8
  %99 = getelementptr [4 x ptr], ptr %raw_args40, i32 0, i32 1
  store ptr %current_prefix38, ptr %99, align 8
  %100 = getelementptr [4 x ptr], ptr %raw_args40, i32 0, i32 2
  store ptr %95, ptr %100, align 8
  %101 = getelementptr [4 x ptr], ptr %raw_args40, i32 0, i32 3
  store ptr %97, ptr %101, align 8
  %102 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %93, ptr %raw_args40, i32 4, ptr null)
  %top41 = load ptr, ptr %top, align 8
  %103 = load ptr, ptr @.pystr_obj.terminal, align 8
  %104 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %top41, ptr %103)
  %105 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %104)
  br i1 %105, label %if.then42, label %if.merge43

while.merge:                                      ; preds = %while.cond
  %result73 = load ptr, ptr %result, align 8
  %106 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %result73, ptr @15, ptr null, i32 0, ptr null)
  %107 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %108 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.41, i64 30)
  %109 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %result74 = load ptr, ptr %result, align 8
  %110 = getelementptr [1 x ptr], ptr %raw_args75, i32 0, i32 0
  store ptr %result74, ptr %110, align 8
  %111 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %109, ptr %raw_args75, i32 1, ptr null)
  %112 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.42, i64 7)
  %result76 = load ptr, ptr %result, align 8
  %113 = getelementptr [4 x ptr], ptr %raw_args77, i32 0, i32 0
  store ptr %108, ptr %113, align 8
  %114 = getelementptr [4 x ptr], ptr %raw_args77, i32 0, i32 1
  store ptr %111, ptr %114, align 8
  %115 = getelementptr [4 x ptr], ptr %raw_args77, i32 0, i32 2
  store ptr %112, ptr %115, align 8
  %116 = getelementptr [4 x ptr], ptr %raw_args77, i32 0, i32 3
  store ptr %result76, ptr %116, align 8
  %117 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %107, ptr %raw_args77, i32 4, ptr null)
  %result78 = load ptr, ptr %result, align 8
  ret ptr %result78

if.then42:                                        ; preds = %while.body
  %result44 = load ptr, ptr %result, align 8
  %118 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.36)
  %current_prefix45 = load ptr, ptr %current_prefix, align 8
  %119 = getelementptr [1 x ptr], ptr %raw_args46, i32 0, i32 0
  store ptr %current_prefix45, ptr %119, align 8
  %120 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %118, ptr %raw_args46, i32 1, ptr null)
  %121 = getelementptr [1 x ptr], ptr %raw_args47, i32 0, i32 0
  store ptr %120, ptr %121, align 8
  %122 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %result44, ptr @11, ptr %raw_args47, i32 1, ptr null)
  br label %if.merge43

if.merge43:                                       ; preds = %if.then42, %while.body
  %top48 = load ptr, ptr %top, align 8
  %123 = load ptr, ptr @.pystr_obj.children, align 8
  %124 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %top48, ptr %123)
  %125 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %124, ptr @12, ptr null, i32 0, ptr null)
  store ptr %125, ptr %child_chars, align 8
  %126 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.10)
  %child_chars49 = load ptr, ptr %child_chars, align 8
  %127 = getelementptr [1 x ptr], ptr %raw_args50, i32 0, i32 0
  store ptr %child_chars49, ptr %127, align 8
  %128 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %126, ptr %raw_args50, i32 1, ptr null)
  %129 = call ptr @_Z19rt_integer_from_i64l(i64 0)
  %130 = call ptr @_Z13rt_compare_gtPN2py8PyObjectES1_(ptr %128, ptr %129)
  %131 = call i1 @_Z10rt_is_truePN2py8PyObjectE(ptr %130)
  br i1 %131, label %if.then51, label %if.merge52

if.then51:                                        ; preds = %if.merge43
  %132 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %133 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.37, i64 23)
  %current_prefix53 = load ptr, ptr %current_prefix, align 8
  %134 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.38, i64 1)
  %135 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.39)
  %child_chars54 = load ptr, ptr %child_chars, align 8
  %136 = getelementptr [1 x ptr], ptr %raw_args55, i32 0, i32 0
  store ptr %child_chars54, ptr %136, align 8
  %137 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %135, ptr %raw_args55, i32 1, ptr null)
  %138 = getelementptr [4 x ptr], ptr %raw_args56, i32 0, i32 0
  store ptr %133, ptr %138, align 8
  %139 = getelementptr [4 x ptr], ptr %raw_args56, i32 0, i32 1
  store ptr %current_prefix53, ptr %139, align 8
  %140 = getelementptr [4 x ptr], ptr %raw_args56, i32 0, i32 2
  store ptr %134, ptr %140, align 8
  %141 = getelementptr [4 x ptr], ptr %raw_args56, i32 0, i32 3
  store ptr %137, ptr %141, align 8
  %142 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %132, ptr %raw_args56, i32 4, ptr null)
  br label %if.merge52

if.merge52:                                       ; preds = %if.then51, %if.merge43
  %top57 = load ptr, ptr %top, align 8
  %143 = load ptr, ptr @.pystr_obj.children, align 8
  %144 = call ptr @_Z15rt_getattr_fastPN2py8PyObjectES1_(ptr %top57, ptr %143)
  %145 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %144, ptr @13, ptr null, i32 0, ptr null)
  %146 = call ptr @_Z11rt_get_iterPN2py8PyObjectE(ptr %145)
  br label %for.cond58

for.cond58:                                       ; preds = %for.body59, %if.merge52
  %147 = call ptr @_Z12rt_iter_nextPN2py8PyObjectEPb(ptr %146, ptr %for_has_value61)
  %148 = load i1, ptr %for_has_value61, align 1
  br i1 %148, label %for.body59, label %for.merge60

for.body59:                                       ; preds = %for.cond58
  call void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr %147, i32 2, ptr %unpack_arr62)
  %149 = getelementptr [2 x ptr], ptr %unpack_arr62, i32 0, i32 0
  %150 = load ptr, ptr %149, align 8
  store ptr %150, ptr %ch, align 8
  %151 = getelementptr [2 x ptr], ptr %unpack_arr62, i32 0, i32 1
  %152 = load ptr, ptr %151, align 8
  store ptr %152, ptr %v, align 8
  %current_prefix63 = load ptr, ptr %current_prefix, align 8
  %ch64 = load ptr, ptr %ch, align 8
  %153 = call ptr @_Z13rt_binary_addPN2py8PyObjectES1_(ptr %current_prefix63, ptr %ch64)
  store ptr %153, ptr %new_prefix, align 8
  %154 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %155 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.40, i64 17)
  %new_prefix65 = load ptr, ptr %new_prefix, align 8
  %156 = getelementptr [2 x ptr], ptr %raw_args66, i32 0, i32 0
  store ptr %155, ptr %156, align 8
  %157 = getelementptr [2 x ptr], ptr %raw_args66, i32 0, i32 1
  store ptr %new_prefix65, ptr %157, align 8
  %158 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %154, ptr %raw_args66, i32 2, ptr null)
  %queue67 = load ptr, ptr %queue, align 8
  %159 = call ptr @_Z19rt_integer_from_i64l(i64 0)
  %v68 = load ptr, ptr %v, align 8
  %new_prefix69 = load ptr, ptr %new_prefix, align 8
  %160 = getelementptr [2 x ptr], ptr %tuple_elems70, i32 0, i32 0
  store ptr %v68, ptr %160, align 8
  %161 = getelementptr [2 x ptr], ptr %tuple_elems70, i32 0, i32 1
  store ptr %new_prefix69, ptr %161, align 8
  %arr_ptr71 = getelementptr [2 x ptr], ptr %tuple_elems70, i32 0, i32 0
  %162 = call ptr @_Z14rt_build_tupleiPPN2py8PyObjectE(i32 2, ptr %arr_ptr71)
  %163 = getelementptr [2 x ptr], ptr %raw_args72, i32 0, i32 0
  store ptr %159, ptr %163, align 8
  %164 = getelementptr [2 x ptr], ptr %raw_args72, i32 0, i32 1
  store ptr %162, ptr %164, align 8
  %165 = call ptr @_Z23rt_call_method_raw_ptrsPN2py8PyObjectEPKcPS1_iS1_(ptr %queue67, ptr @14, ptr %raw_args72, i32 2, ptr null)
  br label %for.cond58

for.merge60:                                      ; preds = %for.cond58
  br label %while.cond
}

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_inPN2py8PyObjectES1_(ptr noundef, ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare noundef ptr @_Z13rt_compare_isPN2py8PyObjectES1_(ptr noundef readnone, ptr noundef readnone) #1

; Function Attrs: mustprogress uwtable
declare void @_Z18rt_unpack_sequencePN2py8PyObjectEiPS1_(ptr noundef, i32 noundef, ptr noundef) #1

; Function Attrs: uwtable(sync)
define internal ptr @"test.<module>.0:0.verify.132:0"(ptr %module, ptr %closure, ptr %args_array, i32 %argc, ptr %kwargs) #0 personality ptr @__gxx_personality_v0 {
entry:
  %raw_args8 = alloca [2 x ptr], align 8
  %raw_args6 = alloca [2 x ptr], align 8
  %raw_args4 = alloca [2 x ptr], align 8
  %right = alloca ptr, align 8
  store ptr null, ptr %right, align 8
  %raw_args1 = alloca [2 x ptr], align 8
  %left = alloca ptr, align 8
  store ptr null, ptr %left, align 8
  %list_elems = alloca [3 x ptr], align 8
  %raw_args = alloca [1 x ptr], align 8
  %0 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %1 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.44, i64 19)
  %2 = getelementptr [1 x ptr], ptr %raw_args, i32 0, i32 0
  store ptr %1, ptr %2, align 8
  %3 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %0, ptr %raw_args, i32 1, ptr null)
  %4 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %5 = call ptr @_Z19rt_integer_from_i64l(i64 23)
  %6 = call ptr @_Z19rt_integer_from_i64l(i64 29)
  %7 = getelementptr [3 x ptr], ptr %list_elems, i32 0, i32 0
  store ptr %4, ptr %7, align 8
  %8 = getelementptr [3 x ptr], ptr %list_elems, i32 0, i32 1
  store ptr %5, ptr %8, align 8
  %9 = getelementptr [3 x ptr], ptr %list_elems, i32 0, i32 2
  store ptr %6, ptr %9, align 8
  %10 = call ptr @_Z13rt_build_listiPPN2py8PyObjectE(i32 3, ptr %list_elems)
  store ptr %10, ptr %left, align 8
  %11 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.43)
  %12 = call ptr @_Z19rt_integer_from_i64l(i64 100)
  %13 = call ptr @_Z19rt_integer_from_i64l(i64 2)
  %14 = getelementptr [2 x ptr], ptr %raw_args1, i32 0, i32 0
  store ptr %12, ptr %14, align 8
  %15 = getelementptr [2 x ptr], ptr %raw_args1, i32 0, i32 1
  store ptr %13, ptr %15, align 8
  %16 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %11, ptr %raw_args1, i32 2, ptr null)
  store ptr %16, ptr %right, align 8
  %17 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %18 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.45, i64 21)
  %left2 = load ptr, ptr %left, align 8
  %right3 = load ptr, ptr %right, align 8
  %19 = call ptr @_Z13rt_compare_eqPN2py8PyObjectES1_(ptr %left2, ptr %right3)
  %20 = getelementptr [2 x ptr], ptr %raw_args4, i32 0, i32 0
  store ptr %18, ptr %20, align 8
  %21 = getelementptr [2 x ptr], ptr %raw_args4, i32 0, i32 1
  store ptr %19, ptr %21, align 8
  %22 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %17, ptr %raw_args4, i32 2, ptr null)
  %23 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %24 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.46, i64 9)
  %left5 = load ptr, ptr %left, align 8
  %25 = getelementptr [2 x ptr], ptr %raw_args6, i32 0, i32 0
  store ptr %24, ptr %25, align 8
  %26 = getelementptr [2 x ptr], ptr %raw_args6, i32 0, i32 1
  store ptr %left5, ptr %26, align 8
  %27 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %23, ptr %raw_args6, i32 2, ptr null)
  %28 = call ptr @_Z14rt_load_globalPN2py8PyObjectEPKc(ptr %module, ptr @.str.6)
  %29 = call ptr @_Z19rt_string_from_cstrPKcl(ptr @.str.47, i64 7)
  %right7 = load ptr, ptr %right, align 8
  %30 = getelementptr [2 x ptr], ptr %raw_args8, i32 0, i32 0
  store ptr %29, ptr %30, align 8
  %31 = getelementptr [2 x ptr], ptr %raw_args8, i32 0, i32 1
  store ptr %right7, ptr %31, align 8
  %32 = call ptr @_Z16rt_call_raw_ptrsPN2py8PyObjectEPS1_iS1_(ptr %28, ptr %raw_args8, i32 2, ptr null)
  %33 = call ptr @_Z7rt_nonev()
  ret ptr %33
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

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
declare void @_Z11rt_shutdownv() #3

; Function Attrs: mustprogress nounwind uwtable
declare noundef ptr @_Z14rt_catch_beginPv(ptr noundef) #4

; Function Attrs: mustprogress uwtable
declare void @_Z28rt_print_unhandled_exceptionPN2py8PyObjectE(ptr noundef) #1

; Function Attrs: mustprogress uwtable
declare void @_Z12rt_catch_endv() #1

attributes #0 = { uwtable(sync) }


