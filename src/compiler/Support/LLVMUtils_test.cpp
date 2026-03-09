

#include "compiler/Support/LLVMUtils.hpp"
#include "compiler/Support/gtest_wrapper.hpp"

#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <filesystem>
#include <fstream>

using namespace pylang;

class LLVMModuleLoaderTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		loader = std::make_unique<LLVMModuleLoader>(ctx);

		// 创建临时测试文件
		test_dir = std::filesystem::temp_directory_path() / "pylang_test";
		std::filesystem::create_directories(test_dir);
	}

	void TearDown() override { std::filesystem::remove_all(test_dir); }

	llvm::LLVMContext ctx;
	std::unique_ptr<LLVMModuleLoader> loader;
	std::filesystem::path test_dir;
};

TEST_F(LLVMModuleLoaderTest, LoadNonexistentFile)
{
	auto result = loader->load_bitcode(test_dir / "nonexistent.bc");
	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind(), ErrorKind::BitcodeLoadFailed);
}

TEST_F(LLVMModuleLoaderTest, LoadInvalidBitcode)
{
	// 创建一个无效的 bitcode 文件
	auto invalid_bc = test_dir / "invalid.bc";
	std::ofstream(invalid_bc) << "This is not valid LLVM bitcode";

	auto result = loader->load_bitcode(invalid_bc);
	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind(), ErrorKind::BitcodeLoadFailed);
}

TEST_F(LLVMModuleLoaderTest, CacheHit)
{
    const char *env_path = std::getenv("PYLANG_RUNTIME_BC");
#ifdef PYLANG_RUNTIME_BC_DEFAULT
    std::string runtime_bc = env_path ? env_path : PYLANG_RUNTIME_BC_DEFAULT;
#else
    std::string runtime_bc = env_path ? env_path : "";
#endif

    if (runtime_bc.empty() || !std::filesystem::exists(runtime_bc)) {
        GTEST_SKIP() << "runtime.bc not found at: " << runtime_bc;
    }

    // 第一次加载
    auto result1 = loader->load_bitcode(runtime_bc);
    ASSERT_TRUE(result1.has_value()) << "First load failed";

    // 第二次加载同一个文件 — 应命中缓存，返回同一个指针
    auto result2 = loader->load_bitcode(runtime_bc);
    ASSERT_TRUE(result2.has_value()) << "Second load failed";

    // load_bitcode 返回 Module*（裸指针），直接解引用比较
    EXPECT_EQ(*result1, *result2)
        << "Cache miss: two loads returned different Module instances";
}


TEST_F(LLVMModuleLoaderTest, VerifyModule)
{
	// 创建一个简单的有效 Module
	llvm::Module mod("test", ctx);
	auto *func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false);
	llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "test_func", &mod);

	auto result = LLVMModuleLoader::verify_module(mod);
	EXPECT_TRUE(result.has_value());
}

TEST_F(LLVMModuleLoaderTest, VerifyInvalidModule)
{
	// 创建一个无效的 Module（空的函数体但不是声明）
	llvm::Module mod("test_invalid", ctx);
	auto *func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false);
	auto *func =
		llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "invalid_func", &mod);

	// 创建一个基本块但不添加终止指令（无效）
	llvm::BasicBlock::Create(ctx, "entry", func);

	auto result = LLVMModuleLoader::verify_module(mod);
	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind(), ErrorKind::VerificationFailed);
}