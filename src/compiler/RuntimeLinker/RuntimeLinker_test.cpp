

#include "RuntimeLinker.hpp"
#include "compiler/Support/gtest_wrapper.hpp"


#include <llvm-20/llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Linker/Linker.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <vector>

using namespace pylang;

class RuntimeLinkerTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		loader = std::make_unique<LLVMModuleLoader>(ctx);

		const char *env_path = std::getenv("PYLANG_RUNTIME_BC");
		if (env_path && std::filesystem::exists(env_path)) {
			runtime_bc_path = env_path;
		} else {
			std::vector<std::string> candidates = { "build/debug/runtime.bc",
				"../build/debug/runtime.bc",
				"../../build/debug/runtime.bc",
				CMAKE_BINARY_DIR "/runtime.bc" };
			for (const auto &path : candidates) {
				if (std::filesystem::exists(path)) {
					runtime_bc_path = path;
					break;
				}
			}
		}

		if (runtime_bc_path.empty()) {
			GTEST_SKIP() << "runtime.bc not found. Set PYLANG_RUNTIME_BC or build runtime first.";
		}
	}

	llvm::LLVMContext ctx;
	std::unique_ptr<LLVMModuleLoader> loader;
	std::string runtime_bc_path;
};

TEST_F(RuntimeLinkerTest, LoadRuntimeBitcode)
{
	auto result = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(result.has_value()) << result.error().to_string();

	auto &linker = *result;
	EXPECT_GT(linker.function_count(), 0);
}

TEST_F(RuntimeLinkerTest, GetFunction_BinaryAdd)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	auto func = linker->get_function("binary_add");
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
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	// 测试单例函数
	auto none_func = linker->get_function("none");
	ASSERT_TRUE(none_func.has_value());
	EXPECT_EQ((*none_func)->category, "singleton");
	EXPECT_EQ((*none_func)->return_type, "obj");
	EXPECT_TRUE((*none_func)->param_types.empty());

	auto true_func = linker->get_function("true");
	ASSERT_TRUE(true_func.has_value());
	EXPECT_EQ((*true_func)->category, "singleton");
}

TEST_F(RuntimeLinkerTest, ListByCategory)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	auto ops = linker->list_by_category("op");
	EXPECT_GT(ops.size(), 0);

	// 验证所有 op 函数的 category 都是 "op"
	for (const auto *func : ops) {
		EXPECT_EQ(func->category, "op");
		EXPECT_NE(func->llvm_func, nullptr);
	}

	// 测试其他 category
	auto singletons = linker->list_by_category("singleton");
	EXPECT_GT(singletons.size(), 0);

	auto creates = linker->list_by_category("create");
	EXPECT_GT(creates.size(), 0);
}

TEST_F(RuntimeLinkerTest, DeclareInUserModule)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	llvm::Module user_module("test_user", ctx);

	// 第一次声明
	auto *decl1 = linker->declare_in(&user_module, "binary_add");
	ASSERT_NE(decl1, nullptr);

	// ✅ 验证是声明而非定义
	EXPECT_TRUE(decl1->isDeclaration());
	EXPECT_TRUE(decl1->empty());// 声明没有基本块

	// 验证函数签名
	EXPECT_FALSE(decl1->getName().empty());
	EXPECT_NE(decl1->getFunctionType(), nullptr);
	EXPECT_EQ(decl1->arg_size(), 2);// binary_add 有 2 个参数

	// 第二次声明应返回同一个（缓存命中）
	auto *decl2 = linker->declare_in(&user_module, "binary_add");
	EXPECT_EQ(decl1, decl2);

	// 声明多个函数
	auto *decl_sub = linker->declare_in(&user_module, "binary_sub");
	ASSERT_NE(decl_sub, nullptr);
	EXPECT_NE(decl1, decl_sub);// 不同函数应该是不同的声明
	EXPECT_TRUE(decl_sub->isDeclaration());
	EXPECT_TRUE(decl_sub->empty());
}

TEST_F(RuntimeLinkerTest, LinkRuntimeModule)
{
    auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
    ASSERT_TRUE(linker.has_value());

    llvm::Module user_module("test_user", ctx);

    auto *decl = linker->declare_in(&user_module, "binary_add");
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(decl->isDeclaration());

    // ✅ 使用封装的 link_into 方法
    auto link_result = linker->link_into(&user_module);
    ASSERT_TRUE(link_result.has_value()) << link_result.error().to_string();

    auto *resolved = user_module.getFunction(decl->getName());
    ASSERT_NE(resolved, nullptr);
    EXPECT_FALSE(resolved->isDeclaration());
    EXPECT_FALSE(resolved->empty());
}

TEST_F(RuntimeLinkerTest, GetFunction_TupleGetitem)
{
    auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
    ASSERT_TRUE(linker.has_value());

    auto func = linker->get_function("tuple_getitem");
    ASSERT_TRUE(func.has_value()) << func.error().to_string();

    const auto *rt_func = *func;
    EXPECT_EQ(rt_func->category, "subscr");
    EXPECT_EQ(rt_func->return_type, "obj");
    EXPECT_EQ(rt_func->param_types, "obj,i32");
    EXPECT_NE(rt_func->llvm_func, nullptr);

    auto *func_type = rt_func->llvm_func_type;
    ASSERT_NE(func_type, nullptr);
    EXPECT_EQ(func_type->getNumParams(), 2);
    EXPECT_TRUE(func_type->getParamType(0)->isPointerTy());
    EXPECT_TRUE(func_type->getParamType(1)->isIntegerTy(32));
}

TEST_F(RuntimeLinkerTest, LinkAndCallTupleGetitem)
{
    auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
    ASSERT_TRUE(linker.has_value());

    llvm::Module user_module("test_tuple_getitem", ctx);

    auto *tuple_getitem = linker->declare_in(&user_module, "tuple_getitem");
    ASSERT_NE(tuple_getitem, nullptr);

    auto *obj_ptr_ty = linker->pyobject_ptr_type();
    auto *func_type = llvm::FunctionType::get(obj_ptr_ty, {obj_ptr_ty}, false);
    auto *test_func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, "test_func", &user_module);
    
    auto *entry = llvm::BasicBlock::Create(ctx, "entry", test_func);
    llvm::IRBuilder<> builder(entry);

    auto *tuple_arg = test_func->getArg(0);
    auto *index = builder.getInt32(0);
    auto *result = builder.CreateCall(tuple_getitem, {tuple_arg, index});
    builder.CreateRet(result);

    auto link_result = linker->link_into(&user_module);
    ASSERT_TRUE(link_result.has_value()) << link_result.error().to_string();

    auto *resolved = user_module.getFunction(tuple_getitem->getName());
    ASSERT_NE(resolved, nullptr);
    EXPECT_FALSE(resolved->isDeclaration());
    EXPECT_FALSE(resolved->empty());
}

TEST_F(RuntimeLinkerTest, UnknownFunction)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	auto result = linker->get_function("nonexistent_function_xyz");
	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind(), ErrorKind::RuntimeFunctionNotFound);
}

TEST_F(RuntimeLinkerTest, DeclareUnknownFunction)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	llvm::Module user_module("test_user", ctx);

	// 尝试声明不存在的函数应返回 nullptr
	auto *decl = linker->declare_in(&user_module, "nonexistent_function_xyz AND This error is expected");
	EXPECT_EQ(decl, nullptr);
}

TEST_F(RuntimeLinkerTest, PyObjectPtrType)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	auto *ptr_type = linker->pyobject_ptr_type();
	ASSERT_NE(ptr_type, nullptr);
	EXPECT_TRUE(ptr_type->isPointerTy());
}

TEST_F(RuntimeLinkerTest, PrintRegistry)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	// 不 ASSERT，只验证不崩溃
	EXPECT_NO_THROW(linker->print_registry());
}

TEST_F(RuntimeLinkerTest, RuntimeFunctionParams)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	auto func = linker->get_function("binary_add");
	ASSERT_TRUE(func.has_value());

	const auto *rt_func = *func;
	auto params = rt_func->params();
	EXPECT_EQ(params.size(), 2);
	EXPECT_EQ(params[0], "obj");
	EXPECT_EQ(params[1], "obj");

	// 测试无参数函数
	auto none_func = linker->get_function("none");
	ASSERT_TRUE(none_func.has_value());
	auto none_params = (*none_func)->params();
	EXPECT_EQ(none_params.size(), 0);
}

TEST_F(RuntimeLinkerTest, VerifyAllCategories)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	// 验证所有预期的 category 都有函数
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
		auto funcs = linker->list_by_category(cat);
		EXPECT_GT(funcs.size(), 0) << "Category '" << cat << "' has no functions";
	}
}

TEST_F(RuntimeLinkerTest, ConcurrentAccess)
{
	auto linker = RuntimeLinker::create(*loader, runtime_bc_path);
	ASSERT_TRUE(linker.has_value());

	llvm::Module user_module("test_concurrent", ctx);

	std::vector<std::thread> threads;
	std::atomic<int> success_count{ 0 };

	for (int i = 0; i < 10; ++i) {
		threads.emplace_back([&]() {
			auto *decl = linker->declare_in(&user_module, "binary_add");
			if (decl != nullptr) { success_count++; }
		});
	}

	for (auto &t : threads) { t.join(); }

	EXPECT_EQ(success_count, 10);
}