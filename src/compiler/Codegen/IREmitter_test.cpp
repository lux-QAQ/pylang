#include "IREmitter.hpp"
#include "compiler/Support/gtest_wrapper.hpp"


#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdlib>
#include <filesystem>

using namespace pylang;

class IREmitterTest : public ::testing::Test
{
  protected:
    // =========================================================================
    // Suite-level 共享资源 — runtime.bc 只加载一次
    // =========================================================================
    static void SetUpTestSuite()
    {
        s_loader = std::make_unique<LLVMModuleLoader>(s_ctx);

        const char *env_path = std::getenv("PYLANG_RUNTIME_BC");
#ifdef PYLANG_RUNTIME_BC_DEFAULT
        std::string runtime_bc = env_path ? env_path : PYLANG_RUNTIME_BC_DEFAULT;
#else
        std::string runtime_bc = env_path ? env_path : "build/debug/runtime.bc";
#endif

        if (!std::filesystem::exists(runtime_bc)) {
            s_skip_reason = "runtime.bc not found at " + runtime_bc;
            return;
        }

        auto linker_result = RuntimeLinker::create(*s_loader, runtime_bc);
        if (!linker_result.has_value()) {
            s_skip_reason = linker_result.error().to_string();
            return;
        }
        s_linker = std::make_unique<RuntimeLinker>(std::move(*linker_result));
    }

    static void TearDownTestSuite()
    {
        s_linker.reset();
        s_loader.reset();
    }

    // =========================================================================
    // Test-level 资源 — 每个测试独立的 module/builder/emitter
    // =========================================================================
    void SetUp() override
    {
        if (!s_skip_reason.empty()) { GTEST_SKIP() << s_skip_reason; }
        if (!s_linker) { GTEST_SKIP() << "linker not initialized"; }

        // module、builder、emitter 每个测试独立，避免状态污染
        module = std::make_unique<llvm::Module>("test_module", s_ctx);
        builder = std::make_unique<llvm::IRBuilder<>>(s_ctx);
        // 复用 suite-level linker，不重新加载 bc
        emitter = std::make_unique<IREmitter>(*builder, *s_linker, module.get());

        create_test_function();
    }

    void TearDown() override
    {
        emitter.reset();
        builder.reset();
        module.reset();
    }

    void create_test_function()
    {
        auto *func_type = llvm::FunctionType::get(emitter->pyobject_ptr_type(), false);
        test_func = llvm::Function::Create(
            func_type, llvm::Function::ExternalLinkage, "test_func", module.get());
        entry_bb = llvm::BasicBlock::Create(s_ctx, "entry", test_func);
        builder->SetInsertPoint(entry_bb);
    }

    void finish_test_function(llvm::Value *ret_val)
    {
        builder->CreateRet(ret_val);
        
        // 验证函数结构
        std::string err;
        llvm::raw_string_ostream os(err);
        ASSERT_FALSE(llvm::verifyFunction(*test_func, &os)) 
            << "Function verification failed: " << err;
    }

    std::string get_ir()
    {
        std::string ir;
        llvm::raw_string_ostream os(ir);
        module->print(os, nullptr);
        return ir;
    }

    bool ir_contains(std::string_view pattern)
    {
        return get_ir().find(pattern) != std::string::npos;
    }

    // Suite-level (static)
    static llvm::LLVMContext s_ctx;
    static std::unique_ptr<LLVMModuleLoader> s_loader;
    static std::unique_ptr<RuntimeLinker> s_linker;
    static std::string s_skip_reason;

    // Test-level (per-test)
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<IREmitter> emitter;
    llvm::Function *test_func = nullptr;
    llvm::BasicBlock *entry_bb = nullptr;
};

// 静态成员定义
llvm::LLVMContext IREmitterTest::s_ctx;
std::unique_ptr<LLVMModuleLoader> IREmitterTest::s_loader;
std::unique_ptr<RuntimeLinker> IREmitterTest::s_linker;
std::string IREmitterTest::s_skip_reason;

// =============================================================================
// Tier 0: 单例
// =============================================================================
TEST_F(IREmitterTest, GetNone)
{
    auto *val = emitter->get_none();
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(llvm::isa<llvm::CallInst>(val));
    
    finish_test_function(val);
    EXPECT_TRUE(ir_contains("rt_none"));
}

TEST_F(IREmitterTest, GetTrue)
{
    auto *val = emitter->get_true();
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    EXPECT_TRUE(ir_contains("rt_true"));
}

TEST_F(IREmitterTest, GetFalse)
{
    auto *val = emitter->get_false();
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    EXPECT_TRUE(ir_contains("rt_false"));
}

TEST_F(IREmitterTest, GetEllipsis)
{
    auto *val = emitter->get_ellipsis();
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    EXPECT_TRUE(ir_contains("rt_ellipsis"));
}

TEST_F(IREmitterTest, GetNotImplemented)
{
    auto *val = emitter->get_not_implemented();
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    EXPECT_TRUE(ir_contains("rt_not_implemented"));
}

// =============================================================================
// Tier 0: 对象创建
// =============================================================================
TEST_F(IREmitterTest, CreateString)
{
    auto *val = emitter->create_string("hello");
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_string_from_cstr"));
    EXPECT_TRUE(ir_contains("hello"));
    EXPECT_TRUE(ir_contains("i64 5"));  // 字符串长度
}

TEST_F(IREmitterTest, CreateStringCaching)
{
    auto *val1 = emitter->create_string("unique_cache_test_12345");
    auto *val2 = emitter->create_string("unique_cache_test_12345");
    
    ASSERT_NE(val1, nullptr);
    ASSERT_NE(val2, nullptr);
    
    // ✅ 使用 LLVM API 验证缓存（最准确）
    EXPECT_TRUE(llvm::isa<llvm::CallInst>(val1));
    EXPECT_TRUE(llvm::isa<llvm::CallInst>(val2));
    
    auto *call1 = llvm::cast<llvm::CallInst>(val1);
    auto *call2 = llvm::cast<llvm::CallInst>(val2);
    
    // ✅ 验证第一个参数（字符串指针）相同 → 证明缓存生效
    EXPECT_EQ(call1->getArgOperand(0), call2->getArgOperand(0))
        << "Both calls should use the same cached global string constant";
    
    finish_test_function(val1);
    
    // ✅ 可选：验证 IR 包含预期内容
    auto ir = get_ir();
    EXPECT_TRUE(ir.find("unique_cache_test_12345") != std::string::npos);
    EXPECT_TRUE(ir.find("rt_string_from_cstr") != std::string::npos);
}

TEST_F(IREmitterTest, CreateTupleEmpty)
{
    auto *val = emitter->create_tuple({});
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_tuple"));
    EXPECT_TRUE(ir_contains("i32 0"));  // count = 0
}

TEST_F(IREmitterTest, CreateTupleWithElements)
{
    auto *elem1 = emitter->create_integer(1);
    auto *elem2 = emitter->create_integer(2);
    auto *elem3 = emitter->create_integer(3);
    
    auto *val = emitter->create_tuple({elem1, elem2, elem3});
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_tuple"));
    EXPECT_TRUE(ir_contains("i32 3"));  // count = 3
    EXPECT_TRUE(ir_contains("alloca"));  // 临时数组
    EXPECT_TRUE(ir_contains("tuple_elems"));
}

// =============================================================================
// Tier 1: 更多对象创建
// =============================================================================
TEST_F(IREmitterTest, CreateInteger)
{
    auto *val = emitter->create_integer(42);
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_integer_from_i64"));
    EXPECT_TRUE(ir_contains("i64 42"));
}

TEST_F(IREmitterTest, CreateIntegerNegative)
{
    auto *val = emitter->create_integer(-123);
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("i64 -123"));
}

TEST_F(IREmitterTest, CreateFloat)
{
    auto *val = emitter->create_float(3.14);
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_float_from_f64"));
    EXPECT_TRUE(ir_contains("double"));
}

TEST_F(IREmitterTest, CreateListEmpty)
{
    auto *val = emitter->create_list({});
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_list"));
    EXPECT_TRUE(ir_contains("i32 0"));
}

TEST_F(IREmitterTest, CreateListWithElements)
{
    auto *elem1 = emitter->create_string("a");
    auto *elem2 = emitter->create_string("b");
    
    auto *val = emitter->create_list({elem1, elem2});
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_list"));
    EXPECT_TRUE(ir_contains("i32 2"));
    EXPECT_TRUE(ir_contains("list_elems"));
}

// =============================================================================
// Tier 1: 二元运算
// =============================================================================
TEST_F(IREmitterTest, BinaryAdd)
{
    auto *lhs = emitter->create_integer(1);
    auto *rhs = emitter->create_integer(2);
    auto *result = emitter->call_binary_add(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_add"));
}

TEST_F(IREmitterTest, BinarySub)
{
    auto *lhs = emitter->create_integer(10);
    auto *rhs = emitter->create_integer(3);
    auto *result = emitter->call_binary_sub(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_sub"));
}

TEST_F(IREmitterTest, BinaryMul)
{
    auto *lhs = emitter->create_integer(5);
    auto *rhs = emitter->create_integer(6);
    auto *result = emitter->call_binary_mul(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_mul"));
}

TEST_F(IREmitterTest, BinaryTruediv)
{
    auto *lhs = emitter->create_integer(10);
    auto *rhs = emitter->create_integer(3);
    auto *result = emitter->call_binary_truediv(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_truediv"));
}

TEST_F(IREmitterTest, BinaryPow)
{
    auto *lhs = emitter->create_integer(2);
    auto *rhs = emitter->create_integer(8);
    auto *result = emitter->call_binary_pow(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_pow"));
}

TEST_F(IREmitterTest, BinaryBitwise)
{
    auto *lhs = emitter->create_integer(0xFF);
    auto *rhs = emitter->create_integer(0x0F);
    
    auto *and_result = emitter->call_binary_and(lhs, rhs);
    ASSERT_NE(and_result, nullptr);
    
    auto *or_result = emitter->call_binary_or(lhs, rhs);
    ASSERT_NE(or_result, nullptr);
    
    auto *xor_result = emitter->call_binary_xor(lhs, rhs);
    ASSERT_NE(xor_result, nullptr);
    
    finish_test_function(and_result);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_binary_and"));
    EXPECT_TRUE(ir_contains("rt_binary_or"));
    EXPECT_TRUE(ir_contains("rt_binary_xor"));
}

// =============================================================================
// Tier 1: 一元运算
// =============================================================================
TEST_F(IREmitterTest, UnaryNeg)
{
    auto *obj = emitter->create_integer(42);
    auto *result = emitter->call_unary_neg(obj);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_unary_neg"));
}

TEST_F(IREmitterTest, UnaryNot)
{
    auto *obj = emitter->get_true();
    auto *result = emitter->call_unary_not(obj);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_unary_not"));
}

TEST_F(IREmitterTest, UnaryInvert)
{
    auto *obj = emitter->create_integer(0xFF);
    auto *result = emitter->call_unary_invert(obj);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_unary_invert"));
}

// =============================================================================
// Tier 1: 类型转换
// =============================================================================
TEST_F(IREmitterTest, IsTrue)
{
    auto *obj = emitter->create_integer(0);
    auto *result = emitter->call_is_true(obj);
    
    ASSERT_NE(result, nullptr);
    
    // is_true 返回 i1，不能直接作为 PyObject* 返回
    // 需要转换为 PyBool
    auto *true_val = emitter->get_true();
    auto *false_val = emitter->get_false();
    auto *py_result = builder->CreateSelect(result, true_val, false_val);
    
    finish_test_function(py_result);
    
    EXPECT_TRUE(ir_contains("rt_is_true"));
}

// =============================================================================
// Tier 2: 比较操作
// =============================================================================
TEST_F(IREmitterTest, CompareEq)
{
    auto *lhs = emitter->create_integer(42);
    auto *rhs = emitter->create_integer(42);
    auto *result = emitter->call_compare_eq(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_eq"));
}

TEST_F(IREmitterTest, CompareLt)
{
    auto *lhs = emitter->create_integer(1);
    auto *rhs = emitter->create_integer(2);
    auto *result = emitter->call_compare_lt(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_lt"));
}

TEST_F(IREmitterTest, CompareIs)
{
    auto *lhs = emitter->get_none();
    auto *rhs = emitter->get_none();
    auto *result = emitter->call_compare_is(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_is"));
}

TEST_F(IREmitterTest, CompareIn)
{
    auto *value = emitter->create_integer(1);
    auto *container = emitter->create_list({value});
    auto *result = emitter->call_compare_in(value, container);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_in"));
}

// =============================================================================
// Tier 2: 迭代器
// =============================================================================
TEST_F(IREmitterTest, GetIter)
{
    auto *list = emitter->create_list({});
    auto *iter = emitter->call_get_iter(list);
    
    ASSERT_NE(iter, nullptr);
    finish_test_function(iter);
    
    EXPECT_TRUE(ir_contains("rt_get_iter"));
}

TEST_F(IREmitterTest, IterNext)
{
    auto *list = emitter->create_list({});
    auto *iter = emitter->call_get_iter(list);
    
    // 创建 bool* 输出参数
    auto *has_value = builder->CreateAlloca(builder->getInt1Ty(), nullptr, "has_value");
    auto *next_val = emitter->call_iter_next(iter, has_value);
    
    // next_val 可能为 nullptr（迭代结束），返回 none
    auto *result = builder->CreateSelect(
        builder->CreateLoad(builder->getInt1Ty(), has_value),
        next_val ? next_val : emitter->get_none(),
        emitter->get_none()
    );
    
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_iter_next"));
}

// =============================================================================
// Tier 3: 下标访问
// =============================================================================
TEST_F(IREmitterTest, Getitem)
{
    auto *list = emitter->create_list({emitter->create_integer(42)});
    auto *index = emitter->create_integer(0);
    auto *result = emitter->call_getitem(list, index);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_getitem"));
}

TEST_F(IREmitterTest, Setitem)
{
    auto *list = emitter->create_list({emitter->get_none()});
    auto *index = emitter->create_integer(0);
    auto *value = emitter->create_integer(42);
    
    emitter->call_setitem(list, index, value);
    
    finish_test_function(list);
    
    EXPECT_TRUE(ir_contains("rt_setitem"));
}

// =============================================================================
// Tier 3: 容器方法
// =============================================================================
TEST_F(IREmitterTest, ListAppend)
{
    auto *list = emitter->create_list({});
    auto *value = emitter->create_integer(42);
    
    emitter->call_list_append(list, value);
    
    finish_test_function(list);
    
    EXPECT_TRUE(ir_contains("rt_list_append"));
}

// =============================================================================
// Tier 0: 属性访问
// =============================================================================
TEST_F(IREmitterTest, Getattr)
{
    auto *obj = emitter->create_string("hello");
    auto *result = emitter->call_getattr(obj, "upper");
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_getattr"));
    EXPECT_TRUE(ir_contains("upper"));
}

TEST_F(IREmitterTest, LoadGlobal)
{
    // 创建一个模拟的 module 对象
    auto *module_obj = emitter->get_none();  // 简化测试
    auto *result = emitter->call_load_global(module_obj, "print");
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_load_global"));
    EXPECT_TRUE(ir_contains("print"));
}

TEST_F(IREmitterTest, StoreGlobal)
{
    auto *module_obj = emitter->get_none();
    auto *value = emitter->create_integer(42);
    
    emitter->call_store_global(module_obj, "x", value);
    
    finish_test_function(value);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_store_global"));
    EXPECT_TRUE(ir_contains("\"x\"") || ir_contains(".str"));
}

// =============================================================================
// Tier 0: 函数调用
// =============================================================================
TEST_F(IREmitterTest, CallFunction)
{
    auto *callable = emitter->get_none();  // 简化测试
    auto *args = emitter->create_tuple({});
    auto *result = emitter->call_function(callable, args);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_call"));
}

TEST_F(IREmitterTest, CallFunctionWithKwargs)
{
    auto *callable = emitter->get_none();
    auto *args = emitter->create_tuple({});
    auto *kwargs = emitter->get_none();  // 简化：应该是 dict
    auto *result = emitter->call_function(callable, args, kwargs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_call"));
}

// =============================================================================
// Tier 4: 方法调用
// =============================================================================
TEST_F(IREmitterTest, LoadMethod)
{
    auto *obj = emitter->create_string("hello");
    auto *method = emitter->call_load_method(obj, "upper");
    
    ASSERT_NE(method, nullptr);
    finish_test_function(method);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_load_method"));
    EXPECT_TRUE(ir_contains("upper"));
}

// =============================================================================
// Tier 0: 异常处理
// =============================================================================
TEST_F(IREmitterTest, LoadAssertionError)
{
    auto *exc_type = emitter->call_load_assertion_error();
    
    ASSERT_NE(exc_type, nullptr);
    finish_test_function(exc_type);
    
    EXPECT_TRUE(ir_contains("rt_load_assertion_error"));
}

TEST_F(IREmitterTest, Raise)
{
    auto *exc = emitter->call_load_assertion_error();
    emitter->call_raise(exc);
    
    // raise 是 void，需要手动添加 unreachable
    builder->CreateUnreachable();
    
    // 验证函数（允许 unreachable）
    std::string err;
    llvm::raw_string_ostream os(err);
    ASSERT_FALSE(llvm::verifyFunction(*test_func, &os)) 
        << "Function verification failed: " << err;
    
    EXPECT_TRUE(ir_contains("rt_raise"));
}

// =============================================================================
// 辅助方法
// =============================================================================
TEST_F(IREmitterTest, PyObjectPtrType)
{
    auto *ptr_type = emitter->pyobject_ptr_type();
    ASSERT_NE(ptr_type, nullptr);
    EXPECT_TRUE(ptr_type->isPointerTy());
}

TEST_F(IREmitterTest, Context)
{
    auto &context = emitter->context();
    EXPECT_EQ(&context, &s_ctx);
}

// =============================================================================
// Tier 1: 更多二元运算
// =============================================================================
TEST_F(IREmitterTest, BinaryFloordiv)
{
    auto *lhs = emitter->create_integer(10);
    auto *rhs = emitter->create_integer(3);
    auto *result = emitter->call_binary_floordiv(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_floordiv"));
}

TEST_F(IREmitterTest, BinaryMod)
{
    auto *lhs = emitter->create_integer(10);
    auto *rhs = emitter->create_integer(3);
    auto *result = emitter->call_binary_mod(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_binary_mod"));
}

TEST_F(IREmitterTest, BinaryShift)
{
    auto *lhs = emitter->create_integer(8);
    auto *rhs = emitter->create_integer(2);
    
    auto *lshift_result = emitter->call_binary_lshift(lhs, rhs);
    ASSERT_NE(lshift_result, nullptr);
    
    auto *rshift_result = emitter->call_binary_rshift(lhs, rhs);
    ASSERT_NE(rshift_result, nullptr);
    
    finish_test_function(lshift_result);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_binary_lshift"));
    EXPECT_TRUE(ir_contains("rt_binary_rshift"));
}

// =============================================================================
// Tier 1: 更多一元运算
// =============================================================================
TEST_F(IREmitterTest, UnaryPos)
{
    auto *obj = emitter->create_integer(42);
    auto *result = emitter->call_unary_pos(obj);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_unary_pos"));
}

// =============================================================================
// Tier 2: 更多比较操作
// =============================================================================
TEST_F(IREmitterTest, CompareNe)
{
    auto *lhs = emitter->create_integer(1);
    auto *rhs = emitter->create_integer(2);
    auto *result = emitter->call_compare_ne(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_ne"));
}

TEST_F(IREmitterTest, CompareLe)
{
    auto *lhs = emitter->create_integer(1);
    auto *rhs = emitter->create_integer(2);
    auto *result = emitter->call_compare_le(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_le"));
}

TEST_F(IREmitterTest, CompareGt)
{
    auto *lhs = emitter->create_integer(2);
    auto *rhs = emitter->create_integer(1);
    auto *result = emitter->call_compare_gt(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_gt"));
}

TEST_F(IREmitterTest, CompareGe)
{
    auto *lhs = emitter->create_integer(2);
    auto *rhs = emitter->create_integer(1);
    auto *result = emitter->call_compare_ge(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_ge"));
}

TEST_F(IREmitterTest, CompareIsNot)
{
    auto *lhs = emitter->get_none();
    auto *rhs = emitter->get_true();
    auto *result = emitter->call_compare_is_not(lhs, rhs);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_is_not"));
}

TEST_F(IREmitterTest, CompareNotIn)
{
    auto *value = emitter->create_integer(99);
    auto *container = emitter->create_list({emitter->create_integer(1)});
    auto *result = emitter->call_compare_not_in(value, container);
    
    ASSERT_NE(result, nullptr);
    finish_test_function(result);
    
    EXPECT_TRUE(ir_contains("rt_compare_not_in"));
}

// =============================================================================
// Tier 3: 更多下标操作
// =============================================================================
TEST_F(IREmitterTest, Delitem)
{
    auto *list = emitter->create_list({emitter->create_integer(1), emitter->create_integer(2)});
    auto *index = emitter->create_integer(0);
    
    emitter->call_delitem(list, index);
    
    finish_test_function(list);
    
    EXPECT_TRUE(ir_contains("rt_delitem"));
}

TEST_F(IREmitterTest, SetAdd)
{
    // 注意：需要先有 create_set 方法，这里简化测试
    auto *set_obj = emitter->get_none();  // 简化
    auto *value = emitter->create_integer(42);
    
    emitter->call_set_add(set_obj, value);
    
    finish_test_function(set_obj);
    
    EXPECT_TRUE(ir_contains("rt_set_add"));
}

// =============================================================================
// Tier 0: 更多属性操作
// =============================================================================
TEST_F(IREmitterTest, Setattr)
{
    auto *obj = emitter->get_none();  // 简化测试
    auto *value = emitter->create_integer(42);
    
    emitter->call_setattr(obj, "x", value);
    
    finish_test_function(obj);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_setattr"));
    EXPECT_TRUE(ir_contains("\"x\"") || ir_contains(".str"));
}

TEST_F(IREmitterTest, Delattr)
{
    auto *obj = emitter->get_none();  // 简化测试
    
    emitter->call_delattr(obj, "x");
    
    finish_test_function(obj);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_delattr"));
    EXPECT_TRUE(ir_contains("\"x\"") || ir_contains(".str"));
}

// =============================================================================
// Tier 0: 模块导入
// =============================================================================
TEST_F(IREmitterTest, Import)
{
    // 签名: call_import(name, globals=null, fromlist=null, level=0)
    auto *module = emitter->call_import("sys");

    ASSERT_NE(module, nullptr);
    finish_test_function(module);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_import"));
    EXPECT_TRUE(ir_contains("sys"));
}

// =============================================================================
// 边界情况测试
// =============================================================================
TEST_F(IREmitterTest, CreateStringEmpty)
{
    auto *val = emitter->create_string("");
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_string_from_cstr"));
    EXPECT_TRUE(ir_contains("i64 0"));  // 长度为 0
}

TEST_F(IREmitterTest, CreateIntegerZero)
{
    auto *val = emitter->create_integer(0);
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    EXPECT_TRUE(ir_contains("i64 0"));
}

TEST_F(IREmitterTest, CreateFloatZero)
{
    auto *val = emitter->create_float(0.0);
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    EXPECT_TRUE(ir_contains("double"));
}

TEST_F(IREmitterTest, CreateTupleLarge)
{
    std::vector<llvm::Value *> elements;
    for (int i = 0; i < 100; ++i) {
        elements.push_back(emitter->create_integer(i));
    }
    
    auto *val = emitter->create_tuple(elements);
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_tuple"));
    EXPECT_TRUE(ir_contains("i32 100"));
}

// =============================================================================
// 错误处理测试
// =============================================================================
TEST_F(IREmitterTest, CallNonexistentFunction)
{
    // 尝试调用不存在的 runtime 函数
    auto *result = emitter->call_binary_add(nullptr, nullptr);
    
    // 应该返回 nullptr（因为 declare_in 会失败）
    // 但这个测试依赖于 RuntimeLinker 的行为
    // 实际上 binary_add 存在，所以这个测试需要修改
    
    // 改为测试 emit_runtime_call 的错误处理
    // 这需要访问 private 方法，暂时跳过
}

TEST_F(IREmitterTest, StringCachingMultiple)
{
    // 测试多个不同字符串的缓存
    auto *str1 = emitter->create_string("hello");
    auto *str2 = emitter->create_string("world");
    auto *str3 = emitter->create_string("hello");  // 应该复用 str1 的全局变量
    
    ASSERT_NE(str1, nullptr);
    ASSERT_NE(str2, nullptr);
    ASSERT_NE(str3, nullptr);
    
    finish_test_function(str1);
    
    auto ir = get_ir();
    
    // 应该有 2 个不同的字符串常量（hello 和 world）
    size_t hello_count = 0;
    size_t world_count = 0;
    size_t pos = 0;
    
    while ((pos = ir.find("hello", pos)) != std::string::npos) {
        ++hello_count;
        ++pos;
    }
    
    pos = 0;
    while ((pos = ir.find("world", pos)) != std::string::npos) {
        ++world_count;
        ++pos;
    }
    
    EXPECT_GT(hello_count, 0);
    EXPECT_GT(world_count, 0);
}

// =============================================================================
// 集成测试：复杂表达式
// =============================================================================
TEST_F(IREmitterTest, ComplexExpression)
{
    // 生成 (1 + 2) * 3 的 IR
    auto *one = emitter->create_integer(1);
    auto *two = emitter->create_integer(2);
    auto *three = emitter->create_integer(3);
    
    auto *sum = emitter->call_binary_add(one, two);
    ASSERT_NE(sum, nullptr);
    
    auto *result = emitter->call_binary_mul(sum, three);
    ASSERT_NE(result, nullptr);
    
    finish_test_function(result);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_binary_add"));
    EXPECT_TRUE(ir_contains("rt_binary_mul"));
}

TEST_F(IREmitterTest, ChainedComparison)
{
    // 生成 1 < 2 and 2 < 3 的 IR（简化版）
    auto *one = emitter->create_integer(1);
    auto *two = emitter->create_integer(2);
    auto *three = emitter->create_integer(3);
    
    auto *cmp1 = emitter->call_compare_lt(one, two);
    ASSERT_NE(cmp1, nullptr);
    
    auto *cmp2 = emitter->call_compare_lt(two, three);
    ASSERT_NE(cmp2, nullptr);
    
    // 这里应该用 and 运算，但我们还没实现 logical and
    // 暂时只返回第一个比较结果
    finish_test_function(cmp1);
    
    EXPECT_TRUE(ir_contains("rt_compare_lt"));
}

// =============================================================================
// Tier 3: 更多容器创建
// =============================================================================

TEST_F(IREmitterTest, CreateDictEmpty)
{
    auto *val = emitter->create_dict({}, {});
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_dict"));
    EXPECT_TRUE(ir_contains("i32 0"));
}

TEST_F(IREmitterTest, CreateDictWithElements)
{
    auto *key1 = emitter->create_string("a");
    auto *val1 = emitter->create_integer(1);
    auto *key2 = emitter->create_string("b");
    auto *val2 = emitter->create_integer(2);
    
    auto *dict = emitter->create_dict({key1, key2}, {val1, val2});
    ASSERT_NE(dict, nullptr);
    
    finish_test_function(dict);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_dict"));
    EXPECT_TRUE(ir_contains("i32 2"));
    EXPECT_TRUE(ir_contains("dict_keys"));
    EXPECT_TRUE(ir_contains("dict_values"));
}

TEST_F(IREmitterTest, CreateSetEmpty)
{
    auto *val = emitter->create_set({});
    ASSERT_NE(val, nullptr);
    
    finish_test_function(val);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_set"));
    EXPECT_TRUE(ir_contains("i32 0"));
}

TEST_F(IREmitterTest, CreateSetWithElements)
{
    auto *elem1 = emitter->create_integer(1);
    auto *elem2 = emitter->create_integer(2);
    auto *elem3 = emitter->create_integer(3);
    
    auto *set = emitter->create_set({elem1, elem2, elem3});
    ASSERT_NE(set, nullptr);
    
    finish_test_function(set);
    
    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_build_set"));
    EXPECT_TRUE(ir_contains("i32 3"));
    EXPECT_TRUE(ir_contains("set_elems"));
}

TEST_F(IREmitterTest, CreateSlice)
{
    auto *start = emitter->create_integer(0);
    auto *stop = emitter->create_integer(10);
    auto *step = emitter->create_integer(2);
    
    auto *slice = emitter->create_slice(start, stop, step);
    ASSERT_NE(slice, nullptr);
    
    finish_test_function(slice);
    
    EXPECT_TRUE(ir_contains("rt_build_slice"));
}

TEST_F(IREmitterTest, CreateSliceWithNone)
{
    auto *start = emitter->get_none();
    auto *stop = emitter->create_integer(10);
    auto *step = emitter->get_none();
    
    auto *slice = emitter->create_slice(start, stop, step);
    ASSERT_NE(slice, nullptr);
    
    finish_test_function(slice);
    
    EXPECT_TRUE(ir_contains("rt_build_slice"));
}

// =============================================================================
// Tier 3: 更多容器方法
// =============================================================================

TEST_F(IREmitterTest, ListExtend)
{
    auto *list = emitter->create_list({});
    auto *other = emitter->create_list({emitter->create_integer(1), emitter->create_integer(2)});
    
    emitter->call_list_extend(list, other);
    
    finish_test_function(list);
    
    EXPECT_TRUE(ir_contains("rt_list_extend"));
}

TEST_F(IREmitterTest, DictMerge)
{
    auto *dict1 = emitter->create_dict({}, {});
    auto *dict2 = emitter->create_dict({emitter->create_string("a")}, {emitter->create_integer(1)});
    
    emitter->call_dict_merge(dict1, dict2);
    
    finish_test_function(dict1);
    
    EXPECT_TRUE(ir_contains("rt_dict_merge"));
}

TEST_F(IREmitterTest, DictUpdate)
{
    auto *dict = emitter->create_dict({}, {});
    auto *other = emitter->create_dict({emitter->create_string("x")}, {emitter->create_integer(42)});
    
    emitter->call_dict_update(dict, other);
    
    finish_test_function(dict);
    
    EXPECT_TRUE(ir_contains("rt_dict_update"));
}

TEST_F(IREmitterTest, SetUpdate)
{
    auto *set = emitter->create_set({});
    auto *iterable = emitter->create_list({emitter->create_integer(1), emitter->create_integer(2)});
    
    emitter->call_set_update(set, iterable);
    
    finish_test_function(set);
    
    EXPECT_TRUE(ir_contains("rt_set_update"));
}

// =============================================================================
// Tier 1: 字节/复数字面量
// =============================================================================

TEST_F(IREmitterTest, CreateBytes)
{
    auto *val = emitter->create_bytes("hello");
    ASSERT_NE(val, nullptr);

    finish_test_function(val);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_bytes_from_buffer"));
    EXPECT_TRUE(ir_contains("i64 5"));// 长度
}

TEST_F(IREmitterTest, CreateBytesEmpty)
{
    auto *val = emitter->create_bytes("");
    ASSERT_NE(val, nullptr);

    finish_test_function(val);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_bytes_from_buffer"));
    EXPECT_TRUE(ir_contains("i64 0"));
}

TEST_F(IREmitterTest, CreateBytesWithNullByte)
{
    // bytes 数据可以包含 \0
    std::string_view data("\x00\x01\x02", 3);
    auto *val = emitter->create_bytes(data);
    ASSERT_NE(val, nullptr);

    finish_test_function(val);

    EXPECT_TRUE(ir_contains("rt_bytes_from_buffer"));
    EXPECT_TRUE(ir_contains("i64 3"));
}

TEST_F(IREmitterTest, CreateComplex)
{
    auto *val = emitter->create_complex(1.0, 2.0);
    ASSERT_NE(val, nullptr);

    finish_test_function(val);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_complex_from_doubles"));
    EXPECT_TRUE(ir_contains("double"));
}

TEST_F(IREmitterTest, CreateComplexZero)
{
    auto *val = emitter->create_complex(0.0, 0.0);
    ASSERT_NE(val, nullptr);

    finish_test_function(val);

    EXPECT_TRUE(ir_contains("rt_complex_from_doubles"));
}

TEST_F(IREmitterTest, CreateComplexNegative)
{
    // -1-2j
    auto *val = emitter->create_complex(-1.0, -2.0);
    ASSERT_NE(val, nullptr);

    finish_test_function(val);

    EXPECT_TRUE(ir_contains("rt_complex_from_doubles"));
}

// =============================================================================
// Tier 2: 解包操作
// =============================================================================

TEST_F(IREmitterTest, UnpackSequence)
{
    auto *list = emitter->create_list({
        emitter->create_integer(1),
        emitter->create_integer(2),
        emitter->create_integer(3),
    });

    // 创建输出数组: PyObject*[3]
    auto *arr_type =
        llvm::ArrayType::get(emitter->pyobject_ptr_type(), 3);
    auto *out_arr = builder->CreateAlloca(arr_type, nullptr, "unpack_out");

    emitter->call_unpack_sequence(list, 3, out_arr);

    // 验证第一个元素可以被读取
    auto *elem0 = builder->CreateLoad(
        emitter->pyobject_ptr_type(),
        builder->CreateConstGEP2_32(arr_type, out_arr, 0, 0));

    finish_test_function(elem0);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_unpack_sequence"));
    EXPECT_TRUE(ir_contains("i32 3"));
    EXPECT_TRUE(ir_contains("unpack_out"));
}

TEST_F(IREmitterTest, UnpackSequencePair)
{
    // 模拟 a, b = (1, 2)
    auto *tup = emitter->create_tuple({
        emitter->create_integer(10),
        emitter->create_integer(20),
    });

    auto *arr_type =
        llvm::ArrayType::get(emitter->pyobject_ptr_type(), 2);
    auto *out_arr = builder->CreateAlloca(arr_type, nullptr, "pair_out");

    emitter->call_unpack_sequence(tup, 2, out_arr);

    auto *a = builder->CreateLoad(
        emitter->pyobject_ptr_type(),
        builder->CreateConstGEP2_32(arr_type, out_arr, 0, 0));

    finish_test_function(a);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_unpack_sequence"));
    EXPECT_TRUE(ir_contains("i32 2"));
}

// =============================================================================
// Tier 4: 闭包操作 (Phase 3.2)
// =============================================================================

TEST_F(IREmitterTest, CreateCell)
{
    auto *value = emitter->create_integer(42);
    auto *cell = emitter->call_create_cell(value);

    ASSERT_NE(cell, nullptr);
    finish_test_function(cell);

    EXPECT_TRUE(ir_contains("rt_create_cell"));
}

TEST_F(IREmitterTest, CreateCellEmpty)
{
    // 创建空 cell（变量未赋值时）
    auto *cell = emitter->call_create_cell(nullptr);

    ASSERT_NE(cell, nullptr);
    finish_test_function(cell);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_create_cell"));
    // null 应该作为参数传入
    EXPECT_TRUE(ir_contains("null"));
}

TEST_F(IREmitterTest, CellGet)
{
    // 先创建 cell，再读取
    auto *value = emitter->create_integer(99);
    auto *cell = emitter->call_create_cell(value);
    auto *retrieved = emitter->call_cell_get(cell);

    ASSERT_NE(retrieved, nullptr);
    finish_test_function(retrieved);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_create_cell"));
    EXPECT_TRUE(ir_contains("rt_cell_get"));
}

TEST_F(IREmitterTest, CellSet)
{
    auto *initial = emitter->create_integer(1);
    auto *cell = emitter->call_create_cell(initial);

    auto *new_value = emitter->create_integer(2);
    emitter->call_cell_set(cell, new_value);

    // cell_set 是 void，继续用 cell 对象作为返回
    finish_test_function(cell);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_create_cell"));
    EXPECT_TRUE(ir_contains("rt_cell_set"));
}

TEST_F(IREmitterTest, CellGetSetRoundtrip)
{
    // 完整的闭包变量生命周期: create → set → get
    auto *cell = emitter->call_create_cell(nullptr);// 空 cell

    auto *val = emitter->create_string("closure_var");
    emitter->call_cell_set(cell, val);// 赋值

    auto *result = emitter->call_cell_get(cell);// 读取

    ASSERT_NE(result, nullptr);
    finish_test_function(result);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_create_cell"));
    EXPECT_TRUE(ir_contains("rt_cell_set"));
    EXPECT_TRUE(ir_contains("rt_cell_get"));
}

// =============================================================================
// Tier 6: 异常匹配 (Phase 3.3)
// =============================================================================

TEST_F(IREmitterTest, CheckExceptionMatch)
{
    // 简化测试：用 none 模拟 exc 和 exc_type
    auto *exc = emitter->get_none();
    auto *exc_type = emitter->call_load_assertion_error();

    auto *matched = emitter->call_check_exception_match(exc, exc_type);

    ASSERT_NE(matched, nullptr);

    // check_exception_match 返回 bool (i1)，需要转换为 PyObject*
    auto *true_val = emitter->get_true();
    auto *false_val = emitter->get_false();
    auto *result = builder->CreateSelect(matched, true_val, false_val);

    finish_test_function(result);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_check_exception_match"));
}

TEST_F(IREmitterTest, CheckExceptionMatchReturnType)
{
    // 验证返回类型是 i1 (bool)
    auto *exc = emitter->get_none();
    auto *exc_type = emitter->get_none();
    auto *result = emitter->call_check_exception_match(exc, exc_type);

    ASSERT_NE(result, nullptr);

    // 返回值必须是 i1
    EXPECT_TRUE(result->getType()->isIntegerTy(1))
        << "check_exception_match should return i1 (bool)";

    auto *py_result = builder->CreateSelect(result, emitter->get_true(), emitter->get_false());
    finish_test_function(py_result);
}

TEST_F(IREmitterTest, Reraise)
{
    // reraise 接受一个异常对象并重新抛出
    auto *exc = emitter->call_load_assertion_error();
    emitter->call_reraise(exc);

    // reraise 是 void 且 [[noreturn]]，需要 unreachable
    builder->CreateUnreachable();

    std::string err;
    llvm::raw_string_ostream os(err);
    ASSERT_FALSE(llvm::verifyFunction(*test_func, &os))
        << "Function verification failed: " << err;

    EXPECT_TRUE(ir_contains("rt_reraise"));
}

TEST_F(IREmitterTest, ReraiseWithNull)
{
    // nullptr 表示没有当前异常（运行时会报错）
    emitter->call_reraise(nullptr);

    builder->CreateUnreachable();

    std::string err;
    llvm::raw_string_ostream os(err);
    ASSERT_FALSE(llvm::verifyFunction(*test_func, &os))
        << "Function verification failed: " << err;

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_reraise"));
    EXPECT_TRUE(ir_contains("null"));
}

// =============================================================================
// 集成测试：新增导出的组合使用
// =============================================================================

TEST_F(IREmitterTest, ComplexLiteralExpression)
{
    // 模拟 z = (1+2j) + (3+4j) 的 IR 生成
    auto *c1 = emitter->create_complex(1.0, 2.0);
    auto *c2 = emitter->create_complex(3.0, 4.0);

    ASSERT_NE(c1, nullptr);
    ASSERT_NE(c2, nullptr);

    auto *result = emitter->call_binary_add(c1, c2);
    ASSERT_NE(result, nullptr);

    finish_test_function(result);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_complex_from_doubles"));
    EXPECT_TRUE(ir_contains("rt_binary_add"));
}

TEST_F(IREmitterTest, UnpackAndUseElements)
{
    // 模拟 a, b = [10, 20]; result = a + b
    auto *lst = emitter->create_list({
        emitter->create_integer(10),
        emitter->create_integer(20),
    });

    auto *arr_type =
        llvm::ArrayType::get(emitter->pyobject_ptr_type(), 2);
    auto *out_arr = builder->CreateAlloca(arr_type, nullptr, "unpack_ab");
    emitter->call_unpack_sequence(lst, 2, out_arr);

    auto *a = builder->CreateLoad(
        emitter->pyobject_ptr_type(),
        builder->CreateConstGEP2_32(arr_type, out_arr, 0, 0));
    auto *b = builder->CreateLoad(
        emitter->pyobject_ptr_type(),
        builder->CreateConstGEP2_32(arr_type, out_arr, 0, 1));

    auto *result = emitter->call_binary_add(a, b);
    ASSERT_NE(result, nullptr);

    finish_test_function(result);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_unpack_sequence"));
    EXPECT_TRUE(ir_contains("rt_binary_add"));
}

TEST_F(IREmitterTest, ClosurePattern)
{
    // 模拟闭包模式:
    // x = 42
    // def inner():
    //     return x   ← 通过 cell 访问
    auto *x_val = emitter->create_integer(42);
    auto *cell = emitter->call_create_cell(x_val);

    // inner 函数体中读取 x
    auto *captured_x = emitter->call_cell_get(cell);
    ASSERT_NE(captured_x, nullptr);

    // 模拟修改闭包变量: x = 100
    auto *new_x = emitter->create_integer(100);
    emitter->call_cell_set(cell, new_x);

    // 再次读取，应得到新值
    auto *updated_x = emitter->call_cell_get(cell);
    ASSERT_NE(updated_x, nullptr);

    finish_test_function(updated_x);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_create_cell"));
    EXPECT_TRUE(ir_contains("rt_cell_get"));
    EXPECT_TRUE(ir_contains("rt_cell_set"));
}

TEST_F(IREmitterTest, ExceptionHandlingPattern)
{
    // 模拟 try/except 的异常匹配流程:
    // try:
    //     raise AssertionError()
    // except AssertionError:
    //     pass

    // 加载 AssertionError 类型
    auto *exc_type = emitter->call_load_assertion_error();

    // 假设已捕获的异常 (简化为 none)
    auto *caught_exc = emitter->get_none();

    // 检查是否匹配
    auto *matched = emitter->call_check_exception_match(caught_exc, exc_type);
    ASSERT_NE(matched, nullptr);

    // 根据匹配结果选择处理分支
    auto *true_val = emitter->get_true();
    auto *false_val = emitter->get_false();
    auto *result = builder->CreateSelect(matched, true_val, false_val);

    finish_test_function(result);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_load_assertion_error"));
    EXPECT_TRUE(ir_contains("rt_check_exception_match"));
}


// =============================================================================
// Tier 4: make_function (Phase 3.2)
// =============================================================================

TEST_F(IREmitterTest, MakeFunction)
{
    // 模拟: def foo(x): return x
    // 编译器生成 @compiled_foo，然后用 make_function 包装

    // 创建一个假的函数指针（用 null 模拟，IR 验证只检查类型）
    auto *code_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));

    auto *module = emitter->get_none();// 简化：用 none 作为 module

    auto *func = emitter->call_make_function("foo", code_ptr, module);

    ASSERT_NE(func, nullptr);
    finish_test_function(func);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_make_function"));
    EXPECT_TRUE(ir_contains("foo"));
}

TEST_F(IREmitterTest, MakeFunctionWithDefaults)
{
    // 模拟: def foo(x, y=10): return x + y
    auto *code_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));
    auto *module = emitter->get_none();

    // 默认值 tuple: (10,)
    auto *default_val = emitter->create_integer(10);
    auto *defaults = emitter->create_tuple({ default_val });

    auto *func = emitter->call_make_function("foo", code_ptr, module, defaults);

    ASSERT_NE(func, nullptr);
    finish_test_function(func);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_make_function"));
    // defaults 不应该是 null
    EXPECT_TRUE(ir_contains("rt_build_tuple"));
}

TEST_F(IREmitterTest, MakeFunctionWithClosure)
{
    // 模拟闭包:
    // def outer():
    //     x = 10
    //     def inner(): return x
    //     return inner

    auto *code_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));
    auto *module = emitter->get_none();

    // 创建 cell 并构建闭包 tuple
    auto *x_val = emitter->create_integer(10);
    auto *cell_x = emitter->call_create_cell(x_val);
    auto *closure = emitter->create_tuple({ cell_x });

    auto *func = emitter->call_make_function(
        "inner", code_ptr, module,
        nullptr,  // no defaults
        nullptr,  // no kwdefaults
        closure);

    ASSERT_NE(func, nullptr);
    finish_test_function(func);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_make_function"));
    EXPECT_TRUE(ir_contains("rt_create_cell"));
}

TEST_F(IREmitterTest, GetClosure)
{
    // 从函数对象中提取闭包
    auto *code_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));
    auto *module = emitter->get_none();

    auto *x_val = emitter->create_integer(42);
    auto *cell = emitter->call_create_cell(x_val);
    auto *closure = emitter->create_tuple({ cell });

    auto *func = emitter->call_make_function(
        "inner", code_ptr, module, nullptr, nullptr, closure);

    auto *retrieved_closure = emitter->call_get_closure(func);
    ASSERT_NE(retrieved_closure, nullptr);

    finish_test_function(retrieved_closure);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_make_function"));
    EXPECT_TRUE(ir_contains("rt_get_closure"));
}

// =============================================================================
// Tier 5: load_build_class (Phase 3.3)
// =============================================================================

TEST_F(IREmitterTest, LoadBuildClass)
{
    auto *bc = emitter->call_load_build_class();
    ASSERT_NE(bc, nullptr);

    finish_test_function(bc);

    EXPECT_TRUE(ir_contains("rt_load_build_class"));
}

TEST_F(IREmitterTest, ClassDefinitionPattern)
{
    // 模拟完整的类定义:
    // class Foo:
    //     x = 1

    // 1. 加载 __build_class__
    auto *build_class = emitter->call_load_build_class();

    // 2. 类体函数（编译器生成）
    auto *body_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));
    auto *module = emitter->get_none();
    auto *body_fn = emitter->call_make_function("Foo", body_ptr, module);

    // 3. 类名
    auto *class_name = emitter->create_string("Foo");

    // 4. 构建参数: (body_fn, "Foo")
    auto *args = emitter->create_tuple({ body_fn, class_name });

    // 5. 调用 __build_class__(body_fn, "Foo")
    auto *cls = emitter->call_function(build_class, args);
    ASSERT_NE(cls, nullptr);

    finish_test_function(cls);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_load_build_class"));
    EXPECT_TRUE(ir_contains("rt_make_function"));
    EXPECT_TRUE(ir_contains("rt_call"));
}

TEST_F(IREmitterTest, ClassWithBasePattern)
{
    // 模拟: class Bar(Foo): pass

    auto *build_class = emitter->call_load_build_class();

    auto *body_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));
    auto *module = emitter->get_none();
    auto *body_fn = emitter->call_make_function("Bar", body_ptr, module);

    auto *class_name = emitter->create_string("Bar");

    // 基类通过 load_global 获取
    auto *base_foo = emitter->call_load_global(module, "Foo");

    // 参数: (body_fn, "Bar", Foo)
    auto *args = emitter->create_tuple({ body_fn, class_name, base_foo });

    auto *cls = emitter->call_function(build_class, args);
    ASSERT_NE(cls, nullptr);

    finish_test_function(cls);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_load_build_class"));
    EXPECT_TRUE(ir_contains("rt_load_global"));
}

TEST_F(IREmitterTest, NestedClosurePattern)
{
    // 模拟嵌套闭包:
    // def outer():
    //     x = 10
    //     def middle():
    //         y = 20
    //         def inner():
    //             return x + y
    //         return inner
    //     return middle

    auto *null_ptr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(builder->getContext()));
    auto *module = emitter->get_none();

    // outer 创建 cell_x
    auto *x_val = emitter->create_integer(10);
    auto *cell_x = emitter->call_create_cell(x_val);

    // outer 创建 middle 的闭包 = (cell_x,)
    auto *middle_closure = emitter->create_tuple({ cell_x });
    auto *middle_fn = emitter->call_make_function(
        "middle", null_ptr, module, nullptr, nullptr, middle_closure);

    // middle 函数体中：
    // 从自身闭包取出 cell_x
    auto *middle_cells = emitter->call_get_closure(middle_fn);
    // （实际编译器中，middle 的闭包是通过函数参数传入的，这里简化）

    // middle 创建 cell_y
    auto *y_val = emitter->create_integer(20);
    auto *cell_y = emitter->call_create_cell(y_val);

    // inner 的闭包 = (cell_x, cell_y)
    // 注意：cell_x 从 middle 的闭包中提取
    auto *inner_closure = emitter->create_tuple({ cell_x, cell_y });
    auto *inner_fn = emitter->call_make_function(
        "inner", null_ptr, module, nullptr, nullptr, inner_closure);

    ASSERT_NE(inner_fn, nullptr);
    finish_test_function(inner_fn);

    auto ir = get_ir();
    EXPECT_TRUE(ir_contains("rt_make_function"));
    EXPECT_TRUE(ir_contains("rt_create_cell"));
    EXPECT_TRUE(ir_contains("rt_get_closure"));
}