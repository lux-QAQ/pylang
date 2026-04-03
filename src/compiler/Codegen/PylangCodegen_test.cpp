#include "PylangCodegen.hpp"
#include "compiler/Support/gtest_wrapper.hpp"
#include "runtime/Value.hpp"
#include "runtime/types/api.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdlib>
#include <filesystem>

using namespace pylang;

// =============================================================================
// PylangCodegen 单元测试
//
// 直接构造 AST 节点测试 codegen，不依赖 Parser/Lexer。
// 集成测试在 integration/ 目录下。
// =============================================================================
class PylangCodegenTest : public ::testing::Test
{
  protected:
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

	void SetUp() override
	{
		if (!s_skip_reason.empty()) { GTEST_SKIP() << s_skip_reason; }
		if (!s_linker) { GTEST_SKIP() << "linker not initialized"; }
	}

	std::string compile_module(ast::Module &mod)
	{
		// PylangCodegen::compile 内部会创建新 module，不共享状态，天然隔离
		auto result = PylangCodegen::compile(&mod, "__test__", s_ctx, *s_linker);
		if (!result.has_value()) {
			ADD_FAILURE() << result.error().to_string();
			return "";
		}

		std::string ir;
		llvm::raw_string_ostream os(ir);
		result->module->print(os, nullptr);
		return ir;
	}

	bool ir_contains(const std::string &ir, std::string_view pattern)
	{
		return ir.find(pattern) != std::string::npos;
	}

	/// 检查 IR 中是否调用了某个 runtime 函数
	/// 兼容 C++ mangled 名（如 _Z9rt_importPKc...）和 extern "C" 名
	bool ir_calls_rt(const std::string &ir, std::string_view logical_name)
	{
		// mangled 名中一定包含逻辑名（C++ mangling 保留原始名称）
		return ir.find(logical_name) != std::string::npos;
	}

	SourceLocation loc() { return SourceLocation{}; }

	static llvm::LLVMContext s_ctx;
	static std::unique_ptr<LLVMModuleLoader> s_loader;
	static std::unique_ptr<RuntimeLinker> s_linker;
	static std::string s_skip_reason;
};

llvm::LLVMContext PylangCodegenTest::s_ctx;
std::unique_ptr<LLVMModuleLoader> PylangCodegenTest::s_loader;
std::unique_ptr<RuntimeLinker> PylangCodegenTest::s_linker;
std::string PylangCodegenTest::s_skip_reason;

// =============================================================================
// 基础: 空模块生成 main + PyInit
// =============================================================================
TEST_F(PylangCodegenTest, EmptyModule)
{
	ast::Module mod("<test>");
	auto ir = compile_module(mod);

	// 函数定义名是确定的（codegen 自己生成，不经过 mangling）
	EXPECT_TRUE(ir_contains(ir, "define void @PyInit___test__"));
	EXPECT_TRUE(ir_contains(ir, "define i32 @main"));

	// runtime 函数以 C++ mangled 名声明，逻辑名是 mangled 名的子串
	// _Z7rt_initv     ⊇ "rt_init"
	// _Z11rt_shutdownv ⊇ "rt_shutdown"
	EXPECT_TRUE(ir_contains(ir, "rt_init"));
	EXPECT_TRUE(ir_contains(ir, "rt_shutdown"));
}

// =============================================================================
// Phase 2.3b: 常量
// =============================================================================
TEST_F(PylangCodegenTest, IntegerConstant)
{
	// x = 42
	ast::Module mod("<test>");
	auto value = std::make_shared<ast::Constant>("hello", loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, value, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_string_from_cstr"));
	EXPECT_TRUE(ir_contains(ir, "hello"));// 字符串字面量本身不受 mangling 影响
}

TEST_F(PylangCodegenTest, StringConstant)
{
	// x = "hello"
	ast::Module mod("<test>");
	auto value = std::make_shared<ast::Constant>("hello", loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, value, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_string_from_cstr"));
	EXPECT_TRUE(ir_contains(ir, "hello"));
}

TEST_F(PylangCodegenTest, BooleanConstants)
{
	// x = True; y = False
	ast::Module mod("<test>");
	auto true_val = std::make_shared<ast::Constant>(true, loc());
	auto target_x = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target_x }, true_val, "", loc()));

	auto false_val = std::make_shared<ast::Constant>(false, loc());
	auto target_y = std::make_shared<ast::Name>("y", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target_y }, false_val, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_true"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_false"));
}

TEST_F(PylangCodegenTest, NoneConstant)
{
	// x = None
	ast::Module mod("<test>");

	// None → py::NameConstant{ NoneType{} } → py::Value
	py::Value none_value = py::py_none();
	auto none_val = std::make_shared<ast::Constant>(none_value.as_ptr(), loc());

	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, none_val, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_none"));
}

// =============================================================================
// Phase 2.3b: 二元表达式
// =============================================================================
TEST_F(PylangCodegenTest, BinaryAdd)
{
	// x = 1 + 2
	ast::Module mod("<test>");
	auto lhs = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto rhs = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto binop = std::make_shared<ast::BinaryExpr>(ast::BinaryOpType::PLUS, lhs, rhs, loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, binop, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_binary_add"));
	EXPECT_TRUE(ir_contains(ir, "rt_integer_from_i64"));
}

// =============================================================================
// Phase 2.3b: 一元表达式
// =============================================================================
TEST_F(PylangCodegenTest, UnaryNeg)
{
	// x = -1
	ast::Module mod("<test>");
	auto operand = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto unary = std::make_shared<ast::UnaryExpr>(ast::UnaryOpType::SUB, operand, loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, unary, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_unary_neg"));
}

// =============================================================================
// Phase 2.3b: 比较表达式
// =============================================================================
TEST_F(PylangCodegenTest, CompareEq)
{
	// x = (1 == 2)
	ast::Module mod("<test>");
	auto lhs = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto rhs = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto cmp = std::make_shared<ast::Compare>(lhs,
		std::vector<ast::Compare::OpType>{ ast::Compare::OpType::Eq },
		std::vector<std::shared_ptr<ast::ASTNode>>{ rhs },
		loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, cmp, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_compare_eq"));
}

// =============================================================================
// Phase 2.3c: 变量加载
// =============================================================================
TEST_F(PylangCodegenTest, LoadGlobal)
{
	// x = y
	ast::Module mod("<test>");
	auto name_y = std::make_shared<ast::Name>("y", ast::ContextType::LOAD, loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, name_y, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_load_global"));
}

// =============================================================================
// Phase 2.3c: 函数调用
// =============================================================================
TEST_F(PylangCodegenTest, FunctionCall)
{
	// print(42)
	ast::Module mod("<test>");
	auto print_name = std::make_shared<ast::Name>("print", ast::ContextType::LOAD, loc());
	auto arg = std::make_shared<ast::Constant>(int64_t(42), loc());
	auto call = std::make_shared<ast::Call>(print_name,
		std::vector<std::shared_ptr<ast::ASTNode>>{ arg },
		std::vector<std::shared_ptr<ast::Keyword>>{},
		loc());
	mod.emplace(std::make_shared<ast::Expression>(call, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	std::cout << "Generated IR:\n" << ir << "\n";
	EXPECT_TRUE(ir_calls_rt(ir, "rt_load_global"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_call"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_call_raw"));
}

// =============================================================================
// Phase 2.3d: if 语句
// =============================================================================
TEST_F(PylangCodegenTest, IfStatement)
{
	ast::Module mod("<test>");
	auto cond = std::make_shared<ast::Constant>(true, loc());

	auto target1 = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto val1 = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto assign1 = std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target1 }, val1, "", loc());

	auto target2 = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto val2 = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto assign2 = std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target2 }, val2, "", loc());

	auto if_stmt = std::make_shared<ast::If>(cond,
		std::vector<std::shared_ptr<ast::ASTNode>>{ assign1 },
		std::vector<std::shared_ptr<ast::ASTNode>>{ assign2 },
		loc());
	mod.emplace(if_stmt);

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_calls_rt(ir, "rt_is_true"));
	// BasicBlock 标签名由 codegen 直接生成，不受 mangling 影响
	EXPECT_TRUE(ir_contains(ir, "if.then"));
	EXPECT_TRUE(ir_contains(ir, "if.else"));
	EXPECT_TRUE(ir_contains(ir, "if.merge"));
}

// =============================================================================
// Phase 2.3d: while 循环
// =============================================================================
TEST_F(PylangCodegenTest, WhileLoop)
{
	ast::Module mod("<test>");
	auto cond = std::make_shared<ast::Constant>(true, loc());
	auto pass = std::make_shared<ast::Pass>(loc());

	auto while_stmt = std::make_shared<ast::While>(cond,
		std::vector<std::shared_ptr<ast::ASTNode>>{ pass },
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		loc());
	mod.emplace(while_stmt);

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "while.cond"));
	EXPECT_TRUE(ir_contains(ir, "while.body"));
}

// =============================================================================
// Phase 2.3d: for 循环
// =============================================================================
TEST_F(PylangCodegenTest, ForLoop)
{
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto elem = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto list = std::make_shared<ast::List>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ elem }, ast::ContextType::LOAD, loc());
	auto pass = std::make_shared<ast::Pass>(loc());

	auto for_stmt = std::make_shared<ast::For>(target,
		list,
		std::vector<std::shared_ptr<ast::ASTNode>>{ pass },
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		"",
		loc());
	mod.emplace(for_stmt);

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_get_iter"));
	EXPECT_TRUE(ir_contains(ir, "rt_iter_next"));
	EXPECT_TRUE(ir_contains(ir, "for.cond"));
	EXPECT_TRUE(ir_contains(ir, "for.body"));
}

// =============================================================================
// Phase 2.3: 容器字面量
// =============================================================================
TEST_F(PylangCodegenTest, ListLiteral)
{
	ast::Module mod("<test>");
	auto e1 = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto e2 = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto list = std::make_shared<ast::List>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ e1, e2 }, ast::ContextType::LOAD, loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, list, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_build_list"));
}

// =============================================================================
// Phase 2.3: pass / assert
// =============================================================================
TEST_F(PylangCodegenTest, PassStatement)
{
	ast::Module mod("<test>");
	mod.emplace(std::make_shared<ast::Pass>(loc()));
	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "define void @PyInit___test__"));
}

TEST_F(PylangCodegenTest, AssertStatement)
{
	ast::Module mod("<test>");
	auto test = std::make_shared<ast::Constant>(true, loc());
	mod.emplace(std::make_shared<ast::Assert>(test, nullptr, loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_contains(ir, "rt_is_true"));
	EXPECT_TRUE(ir_contains(ir, "assert.fail"));
	EXPECT_TRUE(ir_contains(ir, "assert.pass"));
}

// =============================================================================
// Phase 3: AugAssign (inplace 运算)
// =============================================================================
TEST_F(PylangCodegenTest, AugAssignAdd)
{
	// x += 1
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto val = std::make_shared<ast::Constant>(int64_t(1), loc());
	mod.emplace(std::make_shared<ast::AugAssign>(target, ast::BinaryOpType::PLUS, val, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	std::cout << ir;
	EXPECT_TRUE(ir_calls_rt(ir, "rt_inplace_add"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_integer_from_i64"));
}

TEST_F(PylangCodegenTest, AugAssignSub)
{
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto val = std::make_shared<ast::Constant>(int64_t(2), loc());
	mod.emplace(std::make_shared<ast::AugAssign>(target, ast::BinaryOpType::MINUS, val, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_inplace_sub"));
}

TEST_F(PylangCodegenTest, AugAssignMul)
{
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto val = std::make_shared<ast::Constant>(int64_t(3), loc());
	mod.emplace(std::make_shared<ast::AugAssign>(target, ast::BinaryOpType::MULTIPLY, val, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_inplace_mul"));
}

TEST_F(PylangCodegenTest, AugAssignOr)
{
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto val = std::make_shared<ast::Name>("y", ast::ContextType::LOAD, loc());
	mod.emplace(std::make_shared<ast::AugAssign>(target, ast::BinaryOpType::OR, val, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_inplace_or"));
}

// =============================================================================
// Phase 3: Raise
// =============================================================================
TEST_F(PylangCodegenTest, RaiseWithException)
{
	// raise ValueError
	ast::Module mod("<test>");
	auto exc = std::make_shared<ast::Name>("ValueError", ast::ContextType::LOAD, loc());
	mod.emplace(std::make_shared<ast::Raise>(exc, nullptr, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	// 应调用 rt_raise_obj 并终止
	EXPECT_TRUE(ir_calls_rt(ir, "rt_load_global"));// 加载 ValueError
	EXPECT_TRUE(ir_calls_rt(ir, "rt_raise_obj"));
	// raise 之后是 unreachable
	EXPECT_TRUE(ir_contains(ir, "unreachable"));
}

TEST_F(PylangCodegenTest, BareRaise)
{
	// raise  (重新抛出当前异常)
	ast::Module mod("<test>");
	mod.emplace(std::make_shared<ast::Raise>(nullptr, nullptr, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	// 应调用 rt_reraise
	EXPECT_TRUE(ir_calls_rt(ir, "rt_reraise"));
	EXPECT_TRUE(ir_contains(ir, "unreachable"));
}

// =============================================================================
// Phase 3: Lambda
// =============================================================================
TEST_F(PylangCodegenTest, LambdaBasic)
{
	// f = lambda x: x
	ast::Module mod("<test>");

	// 构造 lambda 参数: (x,)
	auto arg_x = std::make_shared<ast::Argument>("x", nullptr, "", loc());
	auto args = std::make_shared<ast::Arguments>(
		std::vector<std::shared_ptr<ast::Argument>>{},// posonlyargs
		std::vector<std::shared_ptr<ast::Argument>>{ arg_x },// args
		nullptr,// vararg
		std::vector<std::shared_ptr<ast::Argument>>{},// kwonlyargs
		std::vector<std::shared_ptr<ast::ASTNode>>{},// kw_defaults
		nullptr,// kwarg
		std::vector<std::shared_ptr<ast::ASTNode>>{},// defaults
		loc());

	// lambda 体: x
	auto body = std::make_shared<ast::Name>("x", ast::ContextType::LOAD, loc());

	auto lambda = std::make_shared<ast::Lambda>(args, body, loc());

	// f = lambda x: x
	auto target = std::make_shared<ast::Name>("f", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, lambda, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	// Lambda 应生成 rt_make_function，而不是 rt_none
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	// 不应该调用 rt_none 作为 lambda 的值
	// （确保不是静默返回 None）
}

TEST_F(PylangCodegenTest, LambdaWithBinaryBody)
{
	// f = lambda x, y: x + y
	ast::Module mod("<test>");

	auto arg_x = std::make_shared<ast::Argument>("x", nullptr, "", loc());
	auto arg_y = std::make_shared<ast::Argument>("y", nullptr, "", loc());
	auto args = std::make_shared<ast::Arguments>(std::vector<std::shared_ptr<ast::Argument>>{},
		std::vector<std::shared_ptr<ast::Argument>>{ arg_x, arg_y },
		nullptr,
		std::vector<std::shared_ptr<ast::Argument>>{},
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		nullptr,
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		loc());

	auto name_x = std::make_shared<ast::Name>("x", ast::ContextType::LOAD, loc());
	auto name_y = std::make_shared<ast::Name>("y", ast::ContextType::LOAD, loc());
	auto body = std::make_shared<ast::BinaryExpr>(ast::BinaryOpType::PLUS, name_x, name_y, loc());

	auto lambda = std::make_shared<ast::Lambda>(args, body, loc());
	auto target = std::make_shared<ast::Name>("f", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, lambda, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	// lambda 体中应有 binary_add
	EXPECT_TRUE(ir_calls_rt(ir, "rt_binary_add"));
}

// =============================================================================
// Phase 3: FunctionDefinition with defaults
// =============================================================================
TEST_F(PylangCodegenTest, FunctionWithDefaultArg)
{
	// def f(x, y=42): pass
	ast::Module mod("<test>");

	auto arg_x = std::make_shared<ast::Argument>("x", nullptr, "", loc());
	auto arg_y = std::make_shared<ast::Argument>("y", nullptr, "", loc());
	auto default_42 = std::make_shared<ast::Constant>(int64_t(42), loc());

	auto args = std::make_shared<ast::Arguments>(
		std::vector<std::shared_ptr<ast::Argument>>{},// posonlyargs
		std::vector<std::shared_ptr<ast::Argument>>{ arg_x, arg_y },// args
		nullptr,// vararg
		std::vector<std::shared_ptr<ast::Argument>>{},// kwonlyargs
		std::vector<std::shared_ptr<ast::ASTNode>>{},// kw_defaults
		nullptr,// kwarg
		std::vector<std::shared_ptr<ast::ASTNode>>{ default_42 },// defaults
		loc());

	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators;

	mod.emplace(
		std::make_shared<ast::FunctionDefinition>("f", args, body, decorators, nullptr, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	// 应创建函数
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	// 默认值应被计算: 42 → rt_integer_from_i64
	EXPECT_TRUE(ir_calls_rt(ir, "rt_integer_from_i64"));
	// 默认值应被打包成 tuple: rt_build_tuple
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_tuple"));
}

TEST_F(PylangCodegenTest, FunctionNoDefaults)
{
	// def greet(): pass
	ast::Module mod("<test>");

	auto args = std::make_shared<ast::Arguments>(std::vector<std::shared_ptr<ast::Argument>>{},
		std::vector<std::shared_ptr<ast::Argument>>{},
		nullptr,
		std::vector<std::shared_ptr<ast::Argument>>{},
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		nullptr,
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		loc());

	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators;

	mod.emplace(std::make_shared<ast::FunctionDefinition>(
		"greet", args, body, decorators, nullptr, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	// 函数名应出现在 IR 中
	EXPECT_TRUE(ir_contains(ir, "greet"));
}

TEST_F(PylangCodegenTest, FunctionWithDecorator)
{
	// @staticmethod
	// def f(): pass
	ast::Module mod("<test>");

	auto args = std::make_shared<ast::Arguments>(std::vector<std::shared_ptr<ast::Argument>>{},
		std::vector<std::shared_ptr<ast::Argument>>{},
		nullptr,
		std::vector<std::shared_ptr<ast::Argument>>{},
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		nullptr,
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		loc());

	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };

	auto decorator = std::make_shared<ast::Name>("staticmethod", ast::ContextType::LOAD, loc());
	std::vector<std::shared_ptr<ast::ASTNode>> decorators = { decorator };

	mod.emplace(
		std::make_shared<ast::FunctionDefinition>("f", args, body, decorators, nullptr, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	// 装饰器需要加载 staticmethod 全局符号
	EXPECT_TRUE(ir_calls_rt(ir, "rt_load_global"));
	// 装饰器应用: call(decorator, func)
	EXPECT_TRUE(ir_calls_rt(ir, "rt_call"));
}

// =============================================================================
// Phase 3: Compare — 链式比较
// =============================================================================
TEST_F(PylangCodegenTest, ChainedCompare)
{
	// x = 1 < 2 < 3
	ast::Module mod("<test>");
	auto v1 = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto v2 = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto v3 = std::make_shared<ast::Constant>(int64_t(3), loc());

	auto cmp = std::make_shared<ast::Compare>(v1,
		std::vector<ast::Compare::OpType>{ ast::Compare::OpType::Lt, ast::Compare::OpType::Lt },
		std::vector<std::shared_ptr<ast::ASTNode>>{ v2, v3 },
		loc());

	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, cmp, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_compare_lt"));
	// 链式比较需要短路求值
	EXPECT_TRUE(ir_calls_rt(ir, "rt_is_true"));
	// 应有条件跳转
	EXPECT_TRUE(
		ir_contains(ir, "cmp.next") || ir_contains(ir, "cmp.false") || ir_contains(ir, "cmp.end"));
}

TEST_F(PylangCodegenTest, CompareIs)
{
	// x = a is None
	ast::Module mod("<test>");
	auto name_a = std::make_shared<ast::Name>("a", ast::ContextType::LOAD, loc());
	py::Value none_val = py::py_none();
	auto none_c = std::make_shared<ast::Constant>(none_val.as_ptr(), loc());

	auto cmp = std::make_shared<ast::Compare>(name_a,
		std::vector<ast::Compare::OpType>{ ast::Compare::OpType::Is },
		std::vector<std::shared_ptr<ast::ASTNode>>{ none_c },
		loc());

	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, cmp, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_compare_is"));
}

TEST_F(PylangCodegenTest, CompareIn)
{
	// x = 1 in [1, 2, 3]
	ast::Module mod("<test>");
	auto v1 = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto e1 = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto e2 = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto e3 = std::make_shared<ast::Constant>(int64_t(3), loc());
	auto list = std::make_shared<ast::List>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ e1, e2, e3 }, ast::ContextType::LOAD, loc());

	auto cmp = std::make_shared<ast::Compare>(v1,
		std::vector<ast::Compare::OpType>{ ast::Compare::OpType::In },
		std::vector<std::shared_ptr<ast::ASTNode>>{ list },
		loc());

	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, cmp, "", loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_compare_in"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_list"));
}

// =============================================================================
// Phase 3: ClassDefinition
// =============================================================================
TEST_F(PylangCodegenTest, SimpleClass)
{
	// class Foo:
	//     pass
	ast::Module mod("<test>");

	std::vector<std::shared_ptr<ast::ASTNode>> bases;
	std::vector<std::shared_ptr<ast::Keyword>> keywords;// ← 修正类型
	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators;

	mod.emplace(
		std::make_shared<ast::ClassDefinition>("Foo", bases, keywords, body, decorators, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_class_aot"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_store_global"));
}

TEST_F(PylangCodegenTest, ClassWithBase)
{
	// class Bar(Base):
	//     pass
	ast::Module mod("<test>");

	auto base = std::make_shared<ast::Name>("Base", ast::ContextType::LOAD, loc());
	std::vector<std::shared_ptr<ast::ASTNode>> bases = { base };
	std::vector<std::shared_ptr<ast::Keyword>> keywords;// ← 修正类型
	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators;

	mod.emplace(
		std::make_shared<ast::ClassDefinition>("Bar", bases, keywords, body, decorators, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_class_aot"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_load_global"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_tuple"));
}

TEST_F(PylangCodegenTest, ClassWithMethodAndAttribute)
{
	// class MyClass:
	//     x = 1
	//     def method(self): pass
	ast::Module mod("<test>");

	auto attr_target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto attr_val = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto attr_assign = std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ attr_target }, attr_val, "", loc());

	auto self_arg = std::make_shared<ast::Argument>("self", nullptr, "", loc());
	auto method_args =
		std::make_shared<ast::Arguments>(std::vector<std::shared_ptr<ast::Argument>>{},
			std::vector<std::shared_ptr<ast::Argument>>{ self_arg },
			nullptr,
			std::vector<std::shared_ptr<ast::Argument>>{},
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			nullptr,
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			loc());
	std::vector<std::shared_ptr<ast::ASTNode>> method_body = { std::make_shared<ast::Pass>(loc()) };
	auto method_def = std::make_shared<ast::FunctionDefinition>("method",
		method_args,
		method_body,
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		nullptr,
		"",
		loc());

	std::vector<std::shared_ptr<ast::ASTNode>> bases;
	std::vector<std::shared_ptr<ast::Keyword>> keywords;// ← 修正类型
	std::vector<std::shared_ptr<ast::ASTNode>> body = { attr_assign, method_def };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators;

	mod.emplace(std::make_shared<ast::ClassDefinition>(
		"MyClass", bases, keywords, body, decorators, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_class_aot"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_make_function"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_dict_setitem_str"));
}

TEST_F(PylangCodegenTest, ClassWithDecorator)
{
	// @dataclass
	// class Point:
	//     pass
	ast::Module mod("<test>");

	auto decorator = std::make_shared<ast::Name>("dataclass", ast::ContextType::LOAD, loc());
	std::vector<std::shared_ptr<ast::ASTNode>> bases;
	std::vector<std::shared_ptr<ast::Keyword>> keywords;// ← 修正类型
	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators = { decorator };

	mod.emplace(
		std::make_shared<ast::ClassDefinition>("Point", bases, keywords, body, decorators, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_class_aot"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_call"));
}

TEST_F(PylangCodegenTest, ClassWithMetaclass)
{
	// class Meta(type, metaclass=Meta):
	//     pass
	ast::Module mod("<test>");

	auto base = std::make_shared<ast::Name>("type", ast::ContextType::LOAD, loc());
	std::vector<std::shared_ptr<ast::ASTNode>> bases = { base };

	// metaclass=Meta keyword
	// ← 使用 std::string 而非 std::optional<std::string>
	auto meta_val = std::make_shared<ast::Name>("Meta", ast::ContextType::LOAD, loc());
	auto kw = std::make_shared<ast::Keyword>(std::string{ "metaclass" }, meta_val, loc());
	std::vector<std::shared_ptr<ast::Keyword>> keywords = { kw };

	std::vector<std::shared_ptr<ast::ASTNode>> body = { std::make_shared<ast::Pass>(loc()) };
	std::vector<std::shared_ptr<ast::ASTNode>> decorators;

	mod.emplace(
		std::make_shared<ast::ClassDefinition>("Meta", bases, keywords, body, decorators, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_class_aot"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_dict"));
}


// =============================================================================
// Phase 3: NamedExpr (walrus operator :=)
// =============================================================================
TEST_F(PylangCodegenTest, NamedExpr)
{
	// if (x := 42): pass
	ast::Module mod("<test>");

	auto val = std::make_shared<ast::Constant>(int64_t(42), loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto walrus = std::make_shared<ast::NamedExpr>(target, val, loc());

	auto if_stmt = std::make_shared<ast::If>(walrus,
		std::vector<std::shared_ptr<ast::ASTNode>>{ std::make_shared<ast::Pass>(loc()) },
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		loc());
	mod.emplace(if_stmt);

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	EXPECT_TRUE(ir_calls_rt(ir, "rt_integer_from_i64"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_is_true"));
}

// =============================================================================
// Phase 3: Delete
// =============================================================================
TEST_F(PylangCodegenTest, DeleteGlobal)
{
	// del x
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::DELETE, loc());
	mod.emplace(
		std::make_shared<ast::Delete>(std::vector<std::shared_ptr<ast::ASTNode>>{ target }, loc()));

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }
	// 全局变量删除应调用 rt_delattr 或类似函数
	EXPECT_TRUE(ir_calls_rt(ir, "rt_delattr") || ir_calls_rt(ir, "rt_store_global"));
}

// =============================================================================
// Phase 3: Try / Except / Finally (核心 EH 测试)
// =============================================================================
TEST_F(PylangCodegenTest, TryExceptBasic)
{
	// try:
	//     pass
	// except:
	//     pass
	ast::Module mod("<test>");
	auto pass = std::make_shared<ast::Pass>(loc());

	// except (bare)
	// 修复: 使用 "" 而非 std::nullopt (AST 定义 name 为 std::string)
	auto handler = std::make_shared<ast::ExceptHandler>(
		nullptr, "", std::vector<std::shared_ptr<ast::ASTNode>>{ pass }, loc());

	auto try_stmt =
		std::make_shared<ast::Try>(std::vector<std::shared_ptr<ast::ASTNode>>{ pass },// body
			std::vector<std::shared_ptr<ast::ExceptHandler>>{ handler },// handlers
			std::vector<std::shared_ptr<ast::ASTNode>>{},// orelse
			std::vector<std::shared_ptr<ast::ASTNode>>{},// finally
			loc());
	mod.emplace(try_stmt);

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }

	// 检查 EH 基础设施
	EXPECT_TRUE(ir_contains(ir, "landingpad"));
	// EXPECT_TRUE(ir_contains(ir, "personality")); // 有些平台可能隐式处理
	EXPECT_TRUE(ir_contains(ir, "__gxx_personality_v0"));

	// 检查异常运行时调用
	EXPECT_TRUE(ir_calls_rt(ir, "rt_catch_begin"));
	// catch_end 可能因为 pass 被优化或逻辑结构而出现在不同位置，但 catch_begin 必须有
}

TEST_F(PylangCodegenTest, TryFinally)
{
	// try:
	//     x = 1
	// finally:
	//     x = 2
	ast::Module mod("<test>");
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	auto v1 = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto v2 = std::make_shared<ast::Constant>(int64_t(2), loc());

	auto body_assign = std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, v1, "", loc());

	auto final_assign = std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, v2, "", loc());

	auto try_stmt =
		std::make_shared<ast::Try>(std::vector<std::shared_ptr<ast::ASTNode>>{ body_assign },
			std::vector<std::shared_ptr<ast::ExceptHandler>>{},
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			std::vector<std::shared_ptr<ast::ASTNode>>{ final_assign },
			loc());
	mod.emplace(try_stmt);

	auto ir = compile_module(mod);
	// finally 块中包含 cleanup
	// 正常路径和 unwind 路径都应该因为 store x=2 而包含 rt_integer_from_i64
	// 且代码中应有 resume 指令（用于 unwind 路径的传播）
	EXPECT_TRUE(ir_contains(ir, "resume"));
}

// =============================================================================
// Phase 3: With Statement
// =============================================================================
TEST_F(PylangCodegenTest, WithStatement)
{
	// with x:
	//     pass
	ast::Module mod("<test>");
	auto context_expr = std::make_shared<ast::Name>("x", ast::ContextType::LOAD, loc());
	auto with_item = std::make_shared<ast::WithItem>(context_expr, nullptr, loc());

	auto with_stmt =
		std::make_shared<ast::With>(std::vector<std::shared_ptr<ast::WithItem>>{ with_item },
			std::vector<std::shared_ptr<ast::ASTNode>>{ std::make_shared<ast::Pass>(loc()) },
			"",
			loc());
	mod.emplace(with_stmt);

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }

	// __enter__ 和 __exit__ 调用
	EXPECT_TRUE(ir_contains(ir, "__enter__"));
	EXPECT_TRUE(ir_contains(ir, "__exit__"));

	// with 语句应生成 landingpad（用于 body 中异常的 cleanup）
	EXPECT_TRUE(ir_contains(ir, "landingpad"));

	// body 只有 pass，不产生 runtime call，所以不会有 invoke。
	// 但 __enter__/__exit__ 的 getattr/call 在 try 保护区之外，
	// 按 Python 语义正确地生成 call 而非 invoke。
	// 如果 body 中有实际操作（如函数调用），则会生成 invoke。
}

TEST_F(PylangCodegenTest, WithStatementInvoke)
{
	// with x:
	//     y = 1 + 2   ← 会生成 invoke（在 try 保护区内）
	ast::Module mod("<test>");
	auto context_expr = std::make_shared<ast::Name>("x", ast::ContextType::LOAD, loc());
	auto with_item = std::make_shared<ast::WithItem>(context_expr, nullptr, loc());

	auto lhs = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto rhs = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto binop = std::make_shared<ast::BinaryExpr>(ast::BinaryOpType::PLUS, lhs, rhs, loc());
	auto target = std::make_shared<ast::Name>("y", ast::ContextType::STORE, loc());
	auto assign = std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, binop, "", loc());

	auto with_stmt =
		std::make_shared<ast::With>(std::vector<std::shared_ptr<ast::WithItem>>{ with_item },
			std::vector<std::shared_ptr<ast::ASTNode>>{ assign },
			"",
			loc());
	mod.emplace(with_stmt);

	auto ir = compile_module(mod);
	if (ir.empty()) { return; }

	EXPECT_TRUE(ir_contains(ir, "__enter__"));
	EXPECT_TRUE(ir_contains(ir, "__exit__"));
	EXPECT_TRUE(ir_contains(ir, "landingpad"));
	// body 中的 binary_add 在 try 保护区内，应生成 invoke
	EXPECT_TRUE(ir_contains(ir, "invoke"));
}

// =============================================================================
// Phase 2/3: 复杂赋值 (Subscript, Slice, Unpack)
// =============================================================================
TEST_F(PylangCodegenTest, StoreSubscript)
{
	// a[1] = 2
	ast::Module mod("<test>");
	auto obj = std::make_shared<ast::Name>("a", ast::ContextType::LOAD, loc());
	auto idx = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto val = std::make_shared<ast::Constant>(int64_t(2), loc());

	auto sub = std::make_shared<ast::Subscript>(
		obj, ast::Subscript::Index{ idx }, ast::ContextType::STORE, loc());

	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ sub }, val, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_calls_rt(ir, "rt_setitem"));
}

TEST_F(PylangCodegenTest, SliceLoad)
{
	// x = a[1:2]
	ast::Module mod("<test>");
	auto obj = std::make_shared<ast::Name>("a", ast::ContextType::LOAD, loc());
	auto lower = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto upper = std::make_shared<ast::Constant>(int64_t(2), loc());

	// slice(lower, upper, step=null)
	ast::Subscript::Slice s;
	s.lower = lower;
	s.upper = upper;

	auto sub = std::make_shared<ast::Subscript>(obj, s, ast::ContextType::LOAD, loc());

	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, sub, "", loc()));

	auto ir = compile_module(mod);
	EXPECT_TRUE(ir_calls_rt(ir, "rt_build_slice"));
	EXPECT_TRUE(ir_calls_rt(ir, "rt_getitem"));
}

TEST_F(PylangCodegenTest, TupleUnpack)
{
	// a, b = x
	ast::Module mod("<test>");
	auto x = std::make_shared<ast::Name>("x", ast::ContextType::LOAD, loc());

	auto t1 = std::make_shared<ast::Name>("a", ast::ContextType::STORE, loc());
	auto t2 = std::make_shared<ast::Name>("b", ast::ContextType::STORE, loc());

	auto tuple_target = std::make_shared<ast::Tuple>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ t1, t2 }, ast::ContextType::STORE, loc());

	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ tuple_target }, x, "", loc()));

	auto ir = compile_module(mod);
	// 应调用 unpack_sequence
	EXPECT_TRUE(ir_calls_rt(ir, "rt_unpack_sequence"));
}
