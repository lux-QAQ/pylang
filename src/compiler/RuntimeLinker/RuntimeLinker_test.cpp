#include "RuntimeLinker.hpp"
#include "compiler/Support/Log.hpp"
#include "compiler/Support/gtest_wrapper.hpp"

#include <llvm-20/llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Linker/Linker.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

using namespace pylang;

class RuntimeLinkerTest : public ::testing::Test
{
  protected:
	// =========================================================================
	// Suite-level: runtime.bc 只加载一次
	// =========================================================================
	static void SetUpTestSuite()
	{
		s_loader = std::make_unique<LLVMModuleLoader>(s_ctx);

		const char *env_path = std::getenv("PYLANG_RUNTIME_BC");
		if (env_path && std::filesystem::exists(env_path)) {
			s_runtime_bc_path = env_path;
		} else {
			std::vector<std::string> candidates = { "build/debug/runtime.bc",
				"../build/debug/runtime.bc",
				"../../build/debug/runtime.bc",
				CMAKE_BINARY_DIR "/runtime.bc" };
			for (const auto &path : candidates) {
				if (std::filesystem::exists(path)) {
					s_runtime_bc_path = path;
					break;
				}
			}
		}

		if (s_runtime_bc_path.empty()) {
			s_skip_reason = "runtime.bc not found. Set PYLANG_RUNTIME_BC or build runtime first.";
			return;
		}

		auto result = RuntimeLinker::create(*s_loader, s_runtime_bc_path);
		if (!result.has_value()) {
			s_skip_reason = result.error().to_string();
			return;
		}
		s_linker = std::make_unique<RuntimeLinker>(std::move(*result));
	}

	static void TearDownTestSuite()
	{
		s_linker.reset();
		s_loader.reset();
	}

    void SetUp() override
    {
        if (!s_skip_reason.empty()) { GTEST_SKIP() << s_skip_reason; }
        if (!s_linker) { GTEST_SKIP() << "linker not initialized"; }
        // 确保开启缓存进行测试
        s_linker->options().enable_function_cache = true;
    }
	void TearDown() override
    {
        // [CRITICAL FIX]
        // 测试用例会在栈上创建 llvm::Module (如 user_module)。
        // 下一个测试可能会在相同的内存地址创建新 Module。
        // 如果不清除缓存，s_linker 会持有指向旧 Module 内部 Function 的悬垂指针，
        // 导致下一个测试由于地址命中缓存而拿到无效指针。
        if (s_linker) {
            s_linker->clear_cache();
        }
    }

	// 便捷访问
	static RuntimeLinker &linker() { return *s_linker; }

	static llvm::LLVMContext s_ctx;
	static std::unique_ptr<LLVMModuleLoader> s_loader;
	static std::unique_ptr<RuntimeLinker> s_linker;
	static std::string s_runtime_bc_path;
	static std::string s_skip_reason;
};

llvm::LLVMContext RuntimeLinkerTest::s_ctx;
std::unique_ptr<LLVMModuleLoader> RuntimeLinkerTest::s_loader;
std::unique_ptr<RuntimeLinker> RuntimeLinkerTest::s_linker;
std::string RuntimeLinkerTest::s_runtime_bc_path;
std::string RuntimeLinkerTest::s_skip_reason;

// =============================================================================

TEST_F(RuntimeLinkerTest, LoadRuntimeBitcode) { EXPECT_GT(linker().function_count(), 0); }

TEST_F(RuntimeLinkerTest, GetFunction_BinaryAdd)
{
	auto func = linker().get_function("binary_add");
	ASSERT_TRUE(func.has_value()) << func.error().to_string();

	const auto *rt_func = *func;
	EXPECT_EQ(rt_func->category, "op");
	EXPECT_EQ(rt_func->return_type, "obj");
	EXPECT_EQ(rt_func->param_types, "obj,obj");
	EXPECT_NE(rt_func->llvm_func, nullptr);
	EXPECT_NE(rt_func->llvm_func_type, nullptr);
}

TEST_F(RuntimeLinkerTest, GetFunction_Singleton)
{
	auto none_func = linker().get_function("none");
	ASSERT_TRUE(none_func.has_value());
	EXPECT_EQ((*none_func)->category, "singleton");
	EXPECT_EQ((*none_func)->return_type, "obj");
	EXPECT_TRUE((*none_func)->param_types.empty());

	auto true_func = linker().get_function("true");
	ASSERT_TRUE(true_func.has_value());
	EXPECT_EQ((*true_func)->category, "singleton");
}

TEST_F(RuntimeLinkerTest, ListByCategory)
{
	auto ops = linker().list_by_category("op");
	EXPECT_GT(ops.size(), 0);
	for (const auto *func : ops) {
		EXPECT_EQ(func->category, "op");
		EXPECT_NE(func->llvm_func, nullptr);
	}

	auto singletons = linker().list_by_category("singleton");
	EXPECT_GT(singletons.size(), 0);

	auto creates = linker().list_by_category("create");
	EXPECT_GT(creates.size(), 0);
}

TEST_F(RuntimeLinkerTest, DeclareInUserModule)
{
	// declare_in 写入 user_module，每个测试用独立 module
	llvm::Module user_module("test_user", s_ctx);

	auto *decl1 = linker().declare_in(&user_module, "binary_add");
	ASSERT_NE(decl1, nullptr);
	EXPECT_TRUE(decl1->isDeclaration());
	EXPECT_TRUE(decl1->empty());
	EXPECT_FALSE(decl1->getName().empty());
	EXPECT_NE(decl1->getFunctionType(), nullptr);
	EXPECT_EQ(decl1->arg_size(), 2);

	// 第二次声明应返回同一个（缓存命中）
	auto *decl2 = linker().declare_in(&user_module, "binary_add");
	EXPECT_EQ(decl1, decl2);

	auto *decl_sub = linker().declare_in(&user_module, "binary_sub");
	ASSERT_NE(decl_sub, nullptr);
	EXPECT_NE(decl1, decl_sub);
	EXPECT_TRUE(decl_sub->isDeclaration());
	EXPECT_TRUE(decl_sub->empty());
}

TEST_F(RuntimeLinkerTest, LinkRuntimeModule)
{
	llvm::Module user_module("test_user", s_ctx);

	auto *decl = linker().declare_in(&user_module, "binary_add");
	ASSERT_NE(decl, nullptr);
	std::string func_name = decl->getName().str();

	auto link_result = linker().link_into(&user_module);
	ASSERT_TRUE(link_result.has_value()) << link_result.error().to_string();

	auto *resolved = user_module.getFunction(func_name);
	ASSERT_NE(resolved, nullptr);
	EXPECT_FALSE(resolved->isDeclaration());
	EXPECT_FALSE(resolved->empty());
}

TEST_F(RuntimeLinkerTest, LinkAndCallTupleGetitem)
{
	llvm::Module user_module("test_tuple_getitem", s_ctx);

	auto *tuple_getitem = linker().declare_in(&user_module, "tuple_getitem");
	ASSERT_NE(tuple_getitem, nullptr);
	std::string func_name = tuple_getitem->getName().str();

	auto *obj_ptr_ty = linker().pyobject_ptr_type();
	auto *func_type = llvm::FunctionType::get(obj_ptr_ty, { obj_ptr_ty }, false);
	auto *test_func = llvm::Function::Create(
		func_type, llvm::Function::ExternalLinkage, "test_func", &user_module);

	auto *entry = llvm::BasicBlock::Create(s_ctx, "entry", test_func);
	llvm::IRBuilder<> builder(entry);
	auto *tuple_arg = test_func->getArg(0);
	auto *index = builder.getInt32(0);
	auto *result = builder.CreateCall(tuple_getitem, { tuple_arg, index });
	builder.CreateRet(result);

	auto link_result = linker().link_into(&user_module);
	ASSERT_TRUE(link_result.has_value()) << link_result.error().to_string();

	auto *resolved = user_module.getFunction(func_name);
	ASSERT_NE(resolved, nullptr);
	EXPECT_FALSE(resolved->isDeclaration());
	EXPECT_FALSE(resolved->empty());
}

TEST_F(RuntimeLinkerTest, UnknownFunction)
{
	auto result = linker().get_function("nonexistent_function_xyz");
	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind(), ErrorKind::RuntimeFunctionNotFound);
}

TEST_F(RuntimeLinkerTest, DeclareUnknownFunction)
{
	llvm::Module user_module("test_user", s_ctx);
	auto *decl =
		linker().declare_in(&user_module, "nonexistent_function_xyz AND This error is expected");
	EXPECT_EQ(decl, nullptr);
}

TEST_F(RuntimeLinkerTest, PyObjectPtrType)
{
	auto *ptr_type = linker().pyobject_ptr_type();
	ASSERT_NE(ptr_type, nullptr);
	EXPECT_TRUE(ptr_type->isPointerTy());
}

TEST_F(RuntimeLinkerTest, PrintRegistry) { EXPECT_NO_THROW(linker().print_registry()); }

TEST_F(RuntimeLinkerTest, RuntimeFunctionParams)
{
	auto func = linker().get_function("binary_add");
	ASSERT_TRUE(func.has_value());

	auto params = (*func)->params();
	EXPECT_EQ(params.size(), 2);
	EXPECT_EQ(params[0], "obj");
	EXPECT_EQ(params[1], "obj");

	auto none_func = linker().get_function("none");
	ASSERT_TRUE(none_func.has_value());
	EXPECT_EQ((*none_func)->params().size(), 0);
}

TEST_F(RuntimeLinkerTest, VerifyAllCategories)
{
	std::vector<std::string> expected_categories = { "op",
		"cmp",
		"create",
		"attr",
		"subscr",
		"convert",
		"module",
		"lifecycle",
		"singleton",
		"error",
		"func" };

	for (const auto &cat : expected_categories) {
		auto funcs = linker().list_by_category(cat);
		EXPECT_GT(funcs.size(), 0) << "Category '" << cat << "' has no functions";
	}
}

TEST_F(RuntimeLinkerTest, ConcurrentAccess)
{
	// declare_in 写入同一个 module，需要外部加锁
	// 每个线程用独立 module 才是正确的并发测试
	std::vector<std::thread> threads;
	std::atomic<int> success_count{ 0 };
	std::mutex module_mutex;

	llvm::Module user_module("test_concurrent", s_ctx);

	for (int i = 0; i < 10; ++i) {
		threads.emplace_back([&]() {
			// get_function 是只读操作，无需加锁
			auto func = linker().get_function("binary_add");
			if (!func.has_value()) { return; }

			// declare_in 写入 module，需要加锁
			std::lock_guard<std::mutex> lock(module_mutex);
			auto *decl = linker().declare_in(&user_module, "binary_add");
			if (decl != nullptr) { success_count++; }
		});
	}

	for (auto &t : threads) { t.join(); }

	EXPECT_EQ(success_count, 10);
}


TEST_F(RuntimeLinkerTest, CacheBehavior)
{
    llvm::Module mod1("mod1", s_ctx);
    llvm::Module mod2("mod2", s_ctx);

    // 1. 首次声明，缓存未命中
    auto *f1 = linker().declare_in(&mod1, "binary_add");
    EXPECT_NE(f1, nullptr);

    // 2. 再次声明（相同模块），应命中缓存
    auto *f2 = linker().declare_in(&mod1, "binary_add");
    EXPECT_EQ(f1, f2) << "应该是同一个指针（缓存命中）";

    // 3. 不同模块，应该是新指针
    auto *f3 = linker().declare_in(&mod2, "binary_add");
    EXPECT_NE(f1, f3);

    // 4. 清除特定模块缓存后，重新声明应通过 LLVM 符号表找到同一个 Function
    linker().forget_module(&mod1);
    auto *f4 = linker().declare_in(&mod1, "binary_add");
    EXPECT_EQ(f4, f1) << "Function 仍在 Module 中，通过符号表找到";

    // 5. 测试 forget_module 确实清除了缓存
    //    通过验证 forget 后 declare_in 走了 slow path（间接验证）
    linker().forget_module(&mod2);
    auto *f5 = linker().declare_in(&mod2, "binary_add");
    EXPECT_EQ(f5, f3) << "Function 仍在 Module 中，通过符号表找到";

    // 6. 测试 clear_cache
    linker().clear_cache();
    auto *f6 = linker().declare_in(&mod1, "binary_add");
    EXPECT_EQ(f6, f1) << "clear_cache 后仍能通过符号表找到";
}

