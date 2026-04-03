#include "_SimpleDriver.hpp"

#include "ast/AST.hpp"
#include "compiler/Support/ScopeTimer.hpp"
#include "compiler/Support/gtest_wrapper.hpp"
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/Value.hpp"
#include "runtime/export/rt_lifecycle.cpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace pylang;

// =============================================================================
// Test Fixture — 共享 SimpleDriver（只加载一次 runtime.bc）
// =============================================================================

class SimpleDriverTest : public ::testing::Test
{
  protected:
	static llvm::LLVMContext s_ctx;
	static std::unique_ptr<SimpleDriver> s_shared_driver;
	static std::string s_build_dir;

	static void SetUpTestSuite()
	{

		rt_init();
		log::set_level(spdlog::level::info);

		PYLANG_TIMER_INFO("SetUpTestSuite::create_shared_driver");

		s_build_dir = CMAKE_BINARY_DIR;

		SimpleDriver::Options opts;
		opts.runtime_bc_path = PYLANG_RUNTIME_BC_DEFAULT;
		opts.output_kind = SimpleDriver::OutputKind::Executable;
		opts.opt_level = 0;
		opts.dump_ir_before_opt = false;
		opts.dump_ir_after_opt = false;
		const std::string spdlog_lib =
#ifdef NDEBUG
			"-lspdlog";
#else
			"-lspdlogd";
#endif
		opts.extra_link_flags = { "-g",
			"-fno-omit-frame-pointer",
			//"-fsanitize=address",
			"-L" + s_build_dir + "/_deps/spdlog-build",
			"-L" + s_build_dir + "/_deps/bdwgc-build",
			"-L" + s_build_dir + "/_deps/cpptrace-build",
			"-L" + s_build_dir + "/_deps/libdwarf-build/src/lib/libdwarf",
			spdlog_lib,
			"-lgmpxx",
			"-lgc",
			"-lgmp",
			"-licuuc",
			"-licudata",
			"-lcpptrace",
			"-ldwarf",
			"-lzstd",
			"-lz"

		};
		if (std::system("ld.lld --version > /dev/null 2>&1") == 0) {
			opts.linker_cmd = "clang++";// 使用 clang 驱动程序
			opts.extra_link_flags.push_back("-fuse-ld=lld");
		} else {
			opts.linker_cmd = "c++";
		}
		spdlog::info("Using linker: {}", opts.linker_cmd);

		auto driver = SimpleDriver::create(std::move(opts), s_ctx);
		if (driver.has_value()) {
			s_shared_driver = std::make_unique<SimpleDriver>(std::move(*driver));
		} else {
			spdlog::error("Failed to create shared driver: {}", driver.error().message());
		}
	}

	static void TearDownTestSuite() { s_shared_driver.reset(); }

	static SimpleDriver &shared_driver()
	{
		EXPECT_NE(s_shared_driver, nullptr) << "Shared driver not available";
		return *s_shared_driver;
	}

	SourceLocation loc()
	{
		static const std::string s_filename = "<test>";
		Position p{ 1, 0, s_filename.c_str() };
		return SourceLocation{ p, p };
	}
};

llvm::LLVMContext SimpleDriverTest::s_ctx;
std::unique_ptr<SimpleDriver> SimpleDriverTest::s_shared_driver;
std::string SimpleDriverTest::s_build_dir;

// =============================================================================
// Helper: AOT 测试执行器（使用共享 driver）
// =============================================================================
static void run_aot_test(SimpleDriver &driver,
	const std::string &test_name,
	const std::string &code,
	const std::vector<std::string> &expected_outputs)
{
	PYLANG_TIMER_INFO("run_aot_test:" + test_name);

	std::string build_dir = CMAKE_BINARY_DIR;

	std::shared_ptr<ast::Module> mod;
	{
		PYLANG_TIMER("  parse:" + test_name);
		auto lexer = Lexer::create(code, "<" + test_name + ">");
		parser::Parser parser(lexer);
		parser.parse();
		mod = parser.module();
	}
	ASSERT_TRUE(mod != nullptr) << "Parser failed for " << test_name;

	std::string mod_name = "__test_" + test_name + "__";

	{
		PYLANG_TIMER("  emit_ir:" + test_name);
		if (auto ir = driver.emit_unoptimized_ir(*mod, mod_name)) {
			std::ofstream ofs(build_dir + "/" + test_name + "_unopt.ll");
			ofs << *ir;
		}
	}

	std::filesystem::path exe_path;
	{
		PYLANG_TIMER("  compile:" + test_name);
		auto result = driver.compile_module(*mod, mod_name);
		ASSERT_TRUE(result.has_value()) << result.error().to_string();
		exe_path = *result;
	}

	std::string output;
	int exit_code;
	{
		PYLANG_TIMER("  execute:" + test_name);
		auto cmd = std::format("{} 2>&1", exe_path.string());
		auto *pipe = popen(cmd.c_str(), "r");
		ASSERT_NE(pipe, nullptr);

		char buf[256];
		while (fgets(buf, sizeof(buf), pipe)) { output += buf; }

		int raw_exit = pclose(pipe);
		exit_code = WIFEXITED(raw_exit) ? WEXITSTATUS(raw_exit) : raw_exit;
	}

	auto debug_msg = "\n================ Output ================\n" + output;
	EXPECT_EQ(exit_code, 0) << "Program crashed." << debug_msg;

	for (const auto &exp : expected_outputs) {
		EXPECT_TRUE(output.find(exp) != std::string::npos)
			<< "Missing expected string: '" << exp << "'" << debug_msg;
	}
}

// =============================================================================
// 基础功能（使用共享 driver，不再各自创建）
// =============================================================================

TEST_F(SimpleDriverTest, CreateDriver)
{
	ASSERT_NE(s_shared_driver, nullptr) << "Shared driver creation failed in SetUpTestSuite";
}

TEST_F(SimpleDriverTest, EmitEmptyModule)
{
	auto &driver = shared_driver();

	ast::Module mod("<test>");
	auto ir = driver.emit_unoptimized_ir(mod, "__test__");
	ASSERT_TRUE(ir.has_value()) << ir.error().to_string();
	EXPECT_TRUE(ir->find("PyInit___test__") != std::string::npos);
	EXPECT_TRUE(ir->find("@main") != std::string::npos);
}

TEST_F(SimpleDriverTest, CompileToIRWithRuntime)
{
	auto &driver = shared_driver();

	// x = 1 + 2
	ast::Module mod("<test>");
	auto lhs = std::make_shared<ast::Constant>(int64_t(1), loc());
	auto rhs = std::make_shared<ast::Constant>(int64_t(2), loc());
	auto binop = std::make_shared<ast::BinaryExpr>(ast::BinaryOpType::PLUS, lhs, rhs, loc());
	auto target = std::make_shared<ast::Name>("x", ast::ContextType::STORE, loc());
	mod.emplace(std::make_shared<ast::Assign>(
		std::vector<std::shared_ptr<ast::ASTNode>>{ target }, binop, "", loc()));

	auto ir = driver.compile_to_ir(mod, "__test__");
	ASSERT_TRUE(ir.has_value()) << ir.error().to_string();

	EXPECT_TRUE(ir->find("define") != std::string::npos);
}

// =============================================================================
// Hello World 里程碑（优化: 减少冗余编译）
// =============================================================================

TEST_F(SimpleDriverTest, HelloWorldExecutable)
{
	auto &driver = shared_driver();

	// print("hello world")
	ast::Module mod("<hello>");
	auto print_name = std::make_shared<ast::Name>("print", ast::ContextType::LOAD, loc());
	auto hello_str = std::make_shared<ast::Constant>(std::string("hello world"), loc());
	auto call = std::make_shared<ast::Call>(print_name,
		std::vector<std::shared_ptr<ast::ASTNode>>{ hello_str },
		std::vector<std::shared_ptr<ast::Keyword>>{},
		loc());
	mod.emplace(std::make_shared<ast::Expression>(call, loc()));

	// 只 dump 未优化 IR（不重复编译）
	if (auto ir = driver.emit_unoptimized_ir(mod, "__hello__")) {
		std::ofstream ofs(s_build_dir + "/hello_unoptimized.ll");
		ofs << *ir;
	}

	// 一次完整编译管道
	auto result = driver.compile_module(mod, "__hello__");
	ASSERT_TRUE(result.has_value()) << result.error().to_string();

	auto cmd = std::format("{} 2>&1", result->string());
	auto *pipe = popen(cmd.c_str(), "r");
	ASSERT_NE(pipe, nullptr);

	std::string output;
	char buf[256];
	while (fgets(buf, sizeof(buf), pipe)) { output += buf; }
	int exit_code = pclose(pipe);

	EXPECT_EQ(exit_code, 0);
	EXPECT_EQ(output, "hello world\n");
}

// =============================================================================
// 手动 AST 编译测试
// =============================================================================

TEST_F(SimpleDriverTest, ComplexAOTCompile_ManualAST)
{
	auto &driver = shared_driver();

	ast::Module mod("<complex>");

	auto pass_node = std::make_shared<ast::Pass>(loc());
	auto class_def = std::make_shared<ast::ClassDefinition>("MyClass",
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		std::vector<std::shared_ptr<ast::Keyword>>{},
		std::vector<std::shared_ptr<ast::ASTNode>>{ pass_node },
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		loc());
	mod.emplace(class_def);

	auto assert_name = std::make_shared<ast::Name>("AssertionError", ast::ContextType::LOAD, loc());
	auto err_msg = std::make_shared<ast::Constant>(std::string("stop"), loc());
	auto assert_call = std::make_shared<ast::Call>(assert_name,
		std::vector<std::shared_ptr<ast::ASTNode>>{ err_msg },
		std::vector<std::shared_ptr<ast::Keyword>>{},
		loc());
	auto raise_stmt = std::make_shared<ast::Raise>(assert_call, nullptr, loc());

	auto empty_args =
		std::make_shared<ast::Arguments>(std::vector<std::shared_ptr<ast::Argument>>{},
			std::vector<std::shared_ptr<ast::Argument>>{},
			nullptr,
			std::vector<std::shared_ptr<ast::Argument>>{},
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			nullptr,
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			loc());

	auto func_def = std::make_shared<ast::FunctionDefinition>("rec",
		empty_args,
		std::vector<std::shared_ptr<ast::ASTNode>>{ raise_stmt },
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		nullptr,
		"",
		loc());
	mod.emplace(func_def);

	auto rec_name = std::make_shared<ast::Name>("rec", ast::ContextType::LOAD, loc());
	auto rec_call = std::make_shared<ast::Call>(rec_name,
		std::vector<std::shared_ptr<ast::ASTNode>>{},
		std::vector<std::shared_ptr<ast::Keyword>>{},
		loc());
	auto rec_call_expr = std::make_shared<ast::Expression>(rec_call, loc());

	auto print_name = std::make_shared<ast::Name>("print", ast::ContextType::LOAD, loc());
	auto caught_str = std::make_shared<ast::Constant>(std::string("caught exception"), loc());
	auto print_call = std::make_shared<ast::Call>(print_name,
		std::vector<std::shared_ptr<ast::ASTNode>>{ caught_str },
		std::vector<std::shared_ptr<ast::Keyword>>{},
		loc());
	auto print_expr = std::make_shared<ast::Expression>(print_call, loc());

	auto except_handler = std::make_shared<ast::ExceptHandler>(
		assert_name, "", std::vector<std::shared_ptr<ast::ASTNode>>{ print_expr }, loc());

	auto try_stmt =
		std::make_shared<ast::Try>(std::vector<std::shared_ptr<ast::ASTNode>>{ rec_call_expr },
			std::vector<std::shared_ptr<ast::ExceptHandler>>{ except_handler },
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			std::vector<std::shared_ptr<ast::ASTNode>>{},
			loc());
	mod.emplace(try_stmt);

	auto result = driver.compile_module(mod, "__test2__");
	ASSERT_TRUE(result.has_value()) << result.error().to_string();

	auto cmd = std::format("{} 2>&1", result->string());
	auto *pipe = popen(cmd.c_str(), "r");
	ASSERT_NE(pipe, nullptr);

	std::string output;
	char buf[256];
	while (fgets(buf, sizeof(buf), pipe)) { output += buf; }
	int exit_code = pclose(pipe);

	EXPECT_EQ(exit_code, 0);
	EXPECT_EQ(output, "caught exception\n");
}

TEST_F(SimpleDriverTest, ComplexAOTCompile_WithParser)
{
	std::string code = R"(
class Math:
    pi = 3.14159
    
    def fib(self, n):
        if n <= 1:
            return n
        return self.fib(n - 1) + self.fib(n - 2)

try:
    m = Math()
    print("Class variable pi =", Math.pi)
    print("fib(5) =", m.fib(5))
    
    raise AssertionError("Test Exception")
except AssertionError:
    print("caught exception successfully")
)";

	auto lexer = Lexer::create(code, "<complex>");
	parser::Parser parser(lexer);
	parser.parse();

	auto mod = parser.module();
	ASSERT_TRUE(mod != nullptr);

	auto result = shared_driver().compile_module(*mod, "__test_complex__");
	ASSERT_TRUE(result.has_value()) << result.error().to_string();

	auto cmd = std::format("{} 2>&1", result->string());
	auto *pipe = popen(cmd.c_str(), "r");
	ASSERT_NE(pipe, nullptr);

	std::string output;
	char buf[256];
	while (fgets(buf, sizeof(buf), pipe)) { output += buf; }
	int exit_code = pclose(pipe);

	EXPECT_EQ(exit_code, 0) << "Output:\n" << output;
	EXPECT_TRUE(output.find("Class variable pi = 3.14159") != std::string::npos) << output;
	EXPECT_TRUE(output.find("fib(5) = 5") != std::string::npos) << output;
	EXPECT_TRUE(output.find("caught exception successfully") != std::string::npos) << output;
}

TEST_F(SimpleDriverTest, ControlFlowAndIterators)
{
	std::string code = R"(
def test_flow(n):
    i = 0
    acc = 0
    while True:
        try:
            if i >= n:
                break
            elif i == 1:
                i = i + 1
                continue
            else:
                acc = acc + i
        finally:
            print("finally block, i=", i)
        i = i + 1
    return acc

print("test_flow(3) =", test_flow(3))

def test_iter():
    total = 0
    for x in [10, 20, 30]:
        total = total + x
    return total

print("test_iter() =", test_iter())
)";

	auto lexer = Lexer::create(code, "<flow>");
	parser::Parser parser(lexer);
	parser.parse();

	auto mod = parser.module();
	ASSERT_TRUE(mod != nullptr);

	if (auto ir = shared_driver().emit_unoptimized_ir(*mod, "__test_flow__")) {
		std::ofstream ofs(s_build_dir + "/flow_unoptimized.ll");
		ofs << *ir;
	}

	auto result = shared_driver().compile_module(*mod, "__test_flow__");
	ASSERT_TRUE(result.has_value()) << result.error().to_string();

	auto cmd = std::format("{} 2>&1", result->string());
	auto *pipe = popen(cmd.c_str(), "r");
	ASSERT_NE(pipe, nullptr);

	std::string output;
	char buf[256];
	while (fgets(buf, sizeof(buf), pipe)) { output += buf; }
	int raw_exit = pclose(pipe);
	int exit_code = WIFEXITED(raw_exit) ? WEXITSTATUS(raw_exit) : raw_exit;

	EXPECT_EQ(exit_code, 0) << "Output:\n" << output;
	EXPECT_TRUE(output.find("finally block, i= 0") != std::string::npos) << output;
	EXPECT_TRUE(output.find("finally block, i= 2") != std::string::npos) << output;
	EXPECT_TRUE(output.find("finally block, i= 3") != std::string::npos) << output;
	EXPECT_TRUE(output.find("test_flow(3) = 2") != std::string::npos) << output;
	EXPECT_TRUE(output.find("test_iter() = 60") != std::string::npos) << output;
}

// =============================================================================
// 特性测试 1-24：全部使用 shared_driver()
// =============================================================================

TEST_F(SimpleDriverTest, Feature_BinaryUnaryOps)
{
	std::string code = R"(
a = 10
b = 3
print("add", a + b)
print("sub", a - b)
print("mul", a * b)
print("truediv", a / b)
print("floordiv", a // b)
print("mod", a % b)
print("pow", a ** b)
print("lshift", a << 1)
print("rshift", a >> 1)
print("and", a & 2)
print("or", a | 4)
print("xor", a ^ 3)
print("neg", -a)
print("pos", +a)
print("inv", ~a)
print("not", not (a == 10))
)";

	std::vector<std::string> expected = { "add 13",
		"sub 7",
		"mul 30",
		"floordiv 3",
		"mod 1",
		"pow 1000",
		"lshift 20",
		"rshift 5",
		"and 2",
		"or 14",
		"xor 9",
		"neg -10",
		"pos 10",
		"inv -11",
		"not False" };
	run_aot_test(shared_driver(), "binary_unary", code, expected);
}

TEST_F(SimpleDriverTest, Feature_DataStructures)
{
	std::string code = R"(
l = [1, 2, 3]
l[0] = 10
print("list", l[0], l[2])
d = {"a": 1, "b": 2}
d["c"] = 3
print("dict", d["b"], d["c"])
t = (7, 8, 9)
print("tuple", t[1])
s = {100, 200}
print("set in", 100 in s)
x, y = (50, 60)
print("unpack", x, y)
)";
	std::vector<std::string> expected = {
		"list 10 3", "dict 2 3", "tuple 8", "set in True", "unpack 50 60"
	};
	run_aot_test(shared_driver(), "data_structures", code, expected);
}

TEST_F(SimpleDriverTest, Feature_ComparisonsAndBoolOps)
{
	std::string code = R"(
a = 5
print("chained_true", 1 < a <= 5 < 10)
print("chained_false", 1 < a > 10)
def check(val, p):
    print("eval", p)
    return val
print("and_op", check(True, 1) and check(False, 2) and check(True, 3))
print("or_op", check(False, 4) or check(True, 5) or check(True, 6))
)";
	std::vector<std::string> expected = { "chained_true True",
		"chained_false False",
		"eval 1",
		"eval 2",
		"and_op False",
		"eval 4",
		"eval 5",
		"or_op True" };
	run_aot_test(shared_driver(), "compare_bool", code, expected);
}

TEST_F(SimpleDriverTest, Feature_ClosuresAndFunctions)
{
	std::string code = R"(
def make_adder(x):
    def adder(y):
        return x + y
    return adder
add5 = make_adder(5)
print("closure", add5(10))
def multi(a, b, c):
    print("call", a, b, c)
multi(1, *(2, 3))
)";
	std::vector<std::string> expected = { "closure 15", "call 1 2 3" };
	run_aot_test(shared_driver(), "closures", code, expected);
}

TEST_F(SimpleDriverTest, Feature_AugAssignAndIfExpr)
{
	std::string code = R"(
x = 10
x += 5
print("aug_add", x)
x *= 2
print("aug_mul", x)
class Counter:
    def __init__(self):
        self.count = 0
    def get_attr(self):
        print("get_attr called")
        return "val"
c = Counter()
c.val = 100
setattr(c, c.get_attr(), getattr(c, c.get_attr()) + 1)
print("attr", c.val)
y = 999 if x > 20 else 0
print("if_expr", y)
)";
	std::vector<std::string> expected = {
		"aug_add 15", "aug_mul 30", "get_attr called", "attr 101", "if_expr 999"
	};
	run_aot_test(shared_driver(), "augassign_ifexpr", code, expected);
}

TEST_F(SimpleDriverTest, Feature_SimpleClosure)
{
	std::string code = R"(
def outer():
    x = 100
    def inner():
        return x
    return inner()
print("simple_closure", outer())
)";
	run_aot_test(shared_driver(), "simple_closure", code, { "simple_closure 100" });
}

TEST_F(SimpleDriverTest, Feature_NestedClosure)
{
	std::string code = R"(
def level1():
    a = 1
    def level2():
        b = 2
        def level3():
            return a + b
        return level3()
    return level2()
print("nested", level1())
)";
	run_aot_test(shared_driver(), "nested_closure", code, { "nested 3" });
}

TEST_F(SimpleDriverTest, Feature_NonlocalClosure)
{
	std::string code = R"(
def make_counter():
    count = 0
    def increment():
        nonlocal count
        count = count + 1
        return count
    return increment
counter = make_counter()
print("c1", counter())
print("c2", counter())
print("c3", counter())
)";
	run_aot_test(shared_driver(), "nonlocal_closure", code, { "c1 1", "c2 2", "c3 3" });
}

TEST_F(SimpleDriverTest, Feature_ClosureFactory)
{
	std::string code = R"PY(
def make_adder(x):
    def adder(y):
        return x + y
    return adder
add3 = make_adder(3)
add7 = make_adder(7)
print("add3(10)", add3(10))
print("add7(10)", add7(10))
print("add3(1)", add3(1))
)PY";
	run_aot_test(
		shared_driver(), "closure_factory", code, { "add3(10) 13", "add7(10) 17", "add3(1) 4" });
}

TEST_F(SimpleDriverTest, Feature_MethodBinding)
{
	std::string code = R"(
class Greeter:
    def __init__(self, name):
        self.name = name
    def greet(self):
        return self.name
g = Greeter("world")
print("greet", g.greet())
class Config:
    version = 42
print("version", Config.version)
)";
	run_aot_test(shared_driver(), "method_binding", code, { "greet world", "version 42" });
}

TEST_F(SimpleDriverTest, Feature_Recursion)
{
	std::string code = R"(
def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)
print("fact5", factorial(5))
print("fact0", factorial(0))
)";
	run_aot_test(shared_driver(), "recursion", code, { "fact5 120", "fact0 1" });
}

TEST_F(SimpleDriverTest, Feature_MultiExcept)
{
	std::string code = R"(
def test_exc(n):
    try:
        if n == 1:
            raise ValueError("val")
        elif n == 2:
            raise TypeError("typ")
        else:
            raise RuntimeError("rt")
    except ValueError:
        print("caught ValueError")
    except TypeError:
        print("caught TypeError")
    except Exception:
        print("caught Exception")
test_exc(1)
test_exc(2)
test_exc(3)
)";
	run_aot_test(shared_driver(),
		"multi_except",
		code,
		{ "caught ValueError", "caught TypeError", "caught Exception" });
}

TEST_F(SimpleDriverTest, Feature_TryElseFinally)
{
	std::string code = R"(
def test_no_error():
    try:
        x = 10
    except Exception:
        print("ERROR: should not reach except")
    else:
        print("else_ok")
    finally:
        print("finally_ok")
test_no_error()
def test_with_error():
    try:
        raise ValueError("oops")
    except ValueError:
        print("except_ok")
    else:
        print("ERROR: should not reach else")
    finally:
        print("finally2_ok")
test_with_error()
)";
	run_aot_test(shared_driver(),
		"try_else_finally",
		code,
		{ "else_ok", "finally_ok", "except_ok", "finally2_ok" });
}

TEST_F(SimpleDriverTest, Feature_ForElse)
{
	std::string code = R"(
for i in [1, 2, 3]:
    pass
else:
    print("for_else_ok")
for i in [1, 2, 3]:
    if i == 2:
        break
else:
    print("ERROR: should not reach")
print("after_break", i)
)";
	run_aot_test(shared_driver(), "for_else", code, { "for_else_ok", "after_break 2" });
}

TEST_F(SimpleDriverTest, Feature_WhileElse)
{
	std::string code = R"(
x = 0
while x < 3:
    x = x + 1
else:
    print("while_else_ok", x)
y = 0
while True:
    y = y + 1
    if y == 5:
        break
else:
    print("ERROR: should not reach")
print("while_break", y)
)";
	run_aot_test(shared_driver(), "while_else", code, { "while_else_ok 3", "while_break 5" });
}

TEST_F(SimpleDriverTest, Feature_StringOps)
{
	std::string code = R"(
s = "hello" + " " + "world"
print("concat", s)
print("eq", s == "hello world")
print("ne", s != "foo")
print("in", "llo" in s)
print("not_in", "xyz" not in s)
)";
	run_aot_test(shared_driver(),
		"string_ops",
		code,
		{ "concat hello world", "eq True", "ne True", "in True", "not_in True" });
}

TEST_F(SimpleDriverTest, Feature_MultiAssign)
{
	std::string code = R"(
a = b = 10
print("multi", a, b)
x, y = 1, 2
x, y = y, x
print("swap", x, y)
a, b, c = [10, 20, 30]
print("list_unpack", a, b, c)
)";
	run_aot_test(shared_driver(),
		"multi_assign",
		code,
		{ "multi 10 10", "swap 2 1", "list_unpack 10 20 30" });
}

TEST_F(SimpleDriverTest, Feature_NestedIfElif)
{
	std::string code = R"(
def classify(n):
    if n < 0:
        return "negative"
    elif n == 0:
        return "zero"
    elif n < 10:
        return "small"
    elif n < 100:
        return "medium"
    else:
        return "large"
print(classify(-5))
print(classify(0))
print(classify(7))
print(classify(42))
print(classify(999))
)";
	run_aot_test(
		shared_driver(), "nested_if", code, { "negative", "zero", "small", "medium", "large" });
}

TEST_F(SimpleDriverTest, Feature_GlobalDecl)
{
	std::string code = R"(
counter = 0
def increment():
    global counter
    counter = counter + 1
increment()
increment()
increment()
print("global_counter", counter)
)";
	run_aot_test(shared_driver(), "global_decl", code, { "global_counter 3" });
}

TEST_F(SimpleDriverTest, Feature_ClassInheritance)
{
	std::string code = R"(
class Animal:
    def speak(self):
        return "..."
class Dog(Animal):
    def speak(self):
        return "woof"
class Cat(Animal):
    def speak(self):
        return "meow"
d = Dog()
c = Cat()
print("dog", d.speak())
print("cat", c.speak())
print("is_animal", isinstance(d, Animal))
)";
	run_aot_test(
		shared_driver(), "class_inherit", code, { "dog woof", "cat meow", "is_animal True" });
}

TEST_F(SimpleDriverTest, Feature_Delete)
{
	std::string code = R"(
d = {"a": 1, "b": 2, "c": 3}
del d["b"]
print("after_del", "b" in d, len(d))
l = [10, 20, 30]
del l[1]
print("list_del", l[0], l[1])
)";
	run_aot_test(shared_driver(), "delete", code, { "after_del False 2", "list_del 10 30" });
}

TEST_F(SimpleDriverTest, Feature_ReturnThroughFinally)
{
	std::string code = R"(
def test_return():
    try:
        return 42
    finally:
        print("finally_before_return")
result = test_return()
print("returned", result)
)";
	run_aot_test(
		shared_driver(), "return_finally", code, { "finally_before_return", "returned 42" });
}

TEST_F(SimpleDriverTest, Feature_WithStatement)
{
	std::string code = R"(
class CM:
    def __init__(self, name):
        self.name = name
    def __enter__(self):
        print("enter", self.name)
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        print("exit", self.name)
        return False
with CM("test") as c:
    print("inside", c.name)
)";
	run_aot_test(shared_driver(), "with_stmt", code, { "enter test", "inside test", "exit test" });
}

TEST_F(SimpleDriverTest, Feature_StressTest)
{
	std::string code = R"(
class Accumulator:
    def __init__(self):
        self.total = 0
    def add(self, n):
        self.total = self.total + n
def make_processor(acc):
    def process(items):
        for item in items:
            try:
                if item < 0:
                    raise ValueError("negative")
                acc.add(item)
            except ValueError:
                pass
    return process
a = Accumulator()
p = make_processor(a)
p([1, 2, -3, 4, -5, 6])
print("stress_total", a.total)
)";
	run_aot_test(shared_driver(), "stress", code, { "stress_total 13" });
}


// =============================================================================
// OOP & Advanced Inheritance Tests
// =============================================================================

TEST_F(SimpleDriverTest, Feature_C3Linearization)
{
	// 测试著名的钻石继承问题 (Diamond Problem)
	// 验证 C3 线性化算法是否正确解析 MRO
	//    A
	//   / \
    //  B   C
	//   \ /
	//    D
	std::string code = R"(class A:
    def name(self):
        return "A"

class B(A):
    pass

class C(A):
    def name(self):
        return "C"

class D(B, C):
    pass

d = D()
print("MRO:", d.name())

class E(C, B):
    pass

e = E()
print("MRO_E:", e.name())
)";// 结束括号也要注意
	run_aot_test(shared_driver(), "c3_linearization", code, { "MRO: C", "MRO_E: C" });
}

TEST_F(SimpleDriverTest, Feature_SuperCall)
{
	// 测试 super() 代理调用
	// 包含无参 super() (这是编译器最难处理的部分之一，需要正确的 __classcell__ 闭包处理)
	std::string code = R"(
class Base:
    def greet(self):
        return "Base"

class Child(Base):
    def greet(self):
        return "Child->" + super().greet()

class GrandChild(Child):
    def greet(self):
        return "GrandChild->" + super().greet()

g = GrandChild()
print(g.greet())
)";
	run_aot_test(shared_driver(), "super_call", code, { "GrandChild->Child->Base" });
}

TEST_F(SimpleDriverTest, Feature_Polymorphism)
{
	// 测试多态：不同类实现相同接口
	std::string code = R"(
class Duck:
    def quack(self):
        return "Quack"

class Person:
    def quack(self):
        return "I'm quacking like a duck"

def make_it_quack(thing):
    print(thing.quack())

d = Duck()
p = Person()

print("Duck:")
make_it_quack(d)
print("Person:")
make_it_quack(p)
)";
	run_aot_test(shared_driver(),
		"polymorphism",
		code,
		{ "Duck:", "Quack", "Person:", "I'm quacking like a duck" });
}

TEST_F(SimpleDriverTest, Feature_MultipleInheritanceMixin)
{
	// 测试 Mixin 模式
	std::string code = R"(
class JsonMixin:
    def to_json(self):
        return "{" + self.key() + ": " + str(self.value()) + "}"

class User(JsonMixin):
    def __init__(self, name, age):
        self.name = name
        self.age = age
    
    def key(self):
        return "user"
        
    def value(self):
        return self.name

class Order(JsonMixin):
    def __init__(self, id):
        self.id = id
        
    def key(self):
        return "order_id"
        
    def value(self):
        return self.id

u = User("Alice", 30)
o = Order(101)

print("User JSON:", u.to_json())
print("Order JSON:", o.to_json())
)";
	// 假设 str() 和基本字符串拼接已实现
	run_aot_test(shared_driver(),
		"mixin",
		code,
		{ "User JSON: {user: Alice}", "Order JSON: {order_id: 101}" });
}

TEST_F(SimpleDriverTest, Feature_OperatorOverloading)
{
	// 测试魔术方法 (Magic Methods)
	std::string code = R"PY(
class Vector:
    def __init__(self, x, y):
        self.x = x
        self.y = y
    
    def __add__(self, other):
        return Vector(self.x + other.x, self.y + other.y)
    
    def __str__(self):
        # 简化测试，只返回固定格式
        return "Vector(" + str(self.x) + ", " + str(self.y) + ")"

v1 = Vector(2, 3)
v2 = Vector(4, 5)
v3 = v1 + v2
print("v3:", v3.x, v3.y)
)PY";
	run_aot_test(shared_driver(), "operator_overloading", code, { "v3: 6 8" });
}

TEST_F(SimpleDriverTest, Feature_ClassStaticMethods)
{
	// 测试 @classmethod 和 @staticmethod
	std::string code = R"(
class Tool:
    count = 0
    
    def __init__(self):
        Tool.count = Tool.count + 1

    @classmethod
    def get_count_cls(cls):
        return cls.count

    @staticmethod
    def calc(x):
        return x * 2

t1 = Tool()
t2 = Tool()

print("Count:", Tool.get_count_cls())
print("Static:", Tool.calc(10))
print("Static via instance:", t1.calc(5))
)";
	run_aot_test(shared_driver(),
		"class_static_methods",
		code,
		{ "Count: 2", "Static: 20", "Static via instance: 10" });
}

TEST_F(SimpleDriverTest, Feature_Properties)
{
	// 测试 @property 装饰器 (Getter/Setter)
	std::string code = R"(
class Celsius:
    def __init__(self, temp=0):
        self._temp = temp

    @property
    def temperature(self):
        return self._temp

    @temperature.setter
    def temperature(self, value):
        if value < -273:
            print("Too cold!")
            return
        self._temp = value

c = Celsius()
c.temperature = 37
print("Normal:", c.temperature)
c.temperature = -300
print("After invalid:", c.temperature)
)";
	run_aot_test(
		shared_driver(), "properties", code, { "Normal: 37", "Too cold!", "After invalid: 37" });
}

TEST_F(SimpleDriverTest, Feature_IsInstanceIsSubclass)
{
	// 测试 isinstance 和 issubclass
	std::string code = R"(
class A: pass
class B(A): pass
class C: pass

b = B()

print("isinstance(b, B):", isinstance(b, B))
print("isinstance(b, A):", isinstance(b, A))
print("isinstance(b, C):", isinstance(b, C))

print("issubclass(B, A):", issubclass(B, A))
print("issubclass(A, B):", issubclass(A, B))
)";
	run_aot_test(shared_driver(),
		"isinstance_check",
		code,
		{ "isinstance(b, B): True",
			"isinstance(b, A): True",
			"isinstance(b, C): False",
			"issubclass(B, A): True",
			"issubclass(A, B): False" });
}

// =============================================================================
// Import & 多文件测试
// =============================================================================

// TEST_F(SimpleDriverTest, Feature_ImportBuiltinModule)
// {
// 	// 测试导入内建模块并使用其函数
// 	std::string code = R"(
// import math
// print("pi:", math.pi)
// print("floor:", math.floor(3.7))
// print("ceil:", math.ceil(3.2))
// )";
// 	run_aot_test(shared_driver(), "import_builtin", code, { "pi: 3.14159", "floor: 3", "ceil: 4" });
// }

TEST_F(SimpleDriverTest, Feature_BigInteger_ComplexOps)
{
	std::string code = R"(
# ==========================================
# 1. 超大整数字面量（利用代数关系构造）
# a = 10^40 - 1 (40个9)
# b = 10^20 - 1 (20个9)
# ==========================================
a = 9999999999999999999999999999999999999999
b = 99999999999999999999

# 加法与减法（测试连续进位与借位）
# a + 1 = 10^40 (1后面40个0)
print("add_carry", a + 1)
# a - b = 9999999999999999999900000000000000000000
print("sub_borrow", a - b)

# ==========================================
# 2. 乘法、除法与取模
# 已知：(10^20 - 1) * (10^20 + 1) = 10^40 - 1
# 因此：a // b 必然等于 10^20 + 1，且 a % b == 0
# ==========================================
print("div", a // b)
print("mod", a % b)

# 反向验证乘法
c = 100000000000000000001
print("mul_check", c * b == a)

# ==========================================
# 3. 位运算 (Bitwise & Shift)
# x = 2^100, y = 2^99
# ==========================================
x = 1267650600228229401496703205376
y = 633825300114114700748351602688

# 或、与、异或
print("bit_or", x | y)
print("bit_and", x & y)
print("bit_xor", x ^ y)

# 移位运算验证
print("lshift", y << 1)
print("rshift", x >> 1)

# ==========================================
# 4. 符号与掩码边界测试
# ==========================================
# 负数比较
print("neg_cmp", -a < -b)

# 长位移与掩码校验 (2^100 - 1 右移 10 位 == 2^90 - 1)
mask = (1 << 100) - 1
print("mask_shift", (mask >> 10) == ((1 << 90) - 1))
)";

	run_aot_test(shared_driver(),
		"big_integer_complex_ops",
		code,
		{
			// 加减法进位/借位结果
			"add_carry 10000000000000000000000000000000000000000",
			"sub_borrow 9999999999999999999900000000000000000000",

			// 除法与取模结果 (10^20 + 1)
			"div 100000000000000000001",
			"mod 0",
			"mul_check True",

			// 位运算结果
			// x | y == x ^ y == 2^100 + 2^99
			"bit_or 1901475900342344102245054808064",
			// x & y == 0 (因为它们是不同的2的次幂)
			"bit_and 0",
			"bit_xor 1901475900342344102245054808064",

			// 移位验证 (互相转化)
			"lshift 1267650600228229401496703205376",
			"rshift 633825300114114700748351602688",

			// 边界/符号逻辑
			"neg_cmp True",
			"mask_shift True",
		});
}

TEST_F(SimpleDriverTest, Feature_LiteralsAndComplex)
{
	std::string code = R"(
# 1. 字面量格式
# 0b1101 = 13, 0x1A = 26, 1.5e2 = 150.0
# mix_list = [0b1101, 0x1A, 1.5e2]
# print("literals", mix_list[0], mix_list[1], mix_list[2])


# 3. 字节与原始字符串
data = b"hello"
raw = r"\n"
print("bytes_len", len(data))
print("raw_check", raw == "\\n")
print("hex_check", 0x10 == 16)
print("is_none", None is None)

# 4. 字典推导式 (如果 Codegen 已支持)
squares = {x: x**2 for x in [1, 2, 3]}
print("dict_comp", squares[2], squares[3])
)";

	run_aot_test(shared_driver(),
		"literals_complex",
		code,
		{ "bytes_len 5", "raw_check True", "hex_check True", "is_none True", "dict_comp 4 9" });
}


TEST_F(SimpleDriverTest, Feature_simpleHashAndEquality)
{
	std::string code = R"(
# 1. 核心比较与哈希一致性 (Hash Parity)
# Python 规定 1.0 == 1 必须成立，且它们的哈希值必须相等
print("eq_check", 1.0 == 1, 1.5 == 1)
print("hash_parity", hash(1.0) == hash(1))
print("hash_large", hash(123456789.0) == hash(123456789))

# 2. 符号与零的处理
# 0.0 与 -0.0 在比较上是相等的
print("zero_eq", 0.0 == -0.0)
print("abs_neg", abs(-1.5) == 1.5)

# 3. 浮点数特有方法 (无需 import)
print("is_int", (1.0).is_integer(), (1.5).is_integer())

# 4. 精确分数还原 (as_integer_ratio)
# 0.75 应该还原为 (3, 4)
ratio = (0.75).as_integer_ratio()
print("ratio", ratio[0], ratio[1])

# 5. 银行家舍入法 (Round half to even)
# Python 的 round() 在 .5 的时候会向偶数取整
print("round_half", round(0.5), round(1.5), round(2.5), round(3.5))

# 6. 类型转换语义
print("to_int", int(1.9), int(-1.9)) # 向零截断
print("from_str", float("1.25") == 1.25)

# 7. 特殊值构造与比较 (通过字符串构造以避开字面量格式测试)
inf = float("inf")
neg_inf = float("-inf")
nan = float("nan")

print("nan_self", nan == nan) # NaN 不等于自身是 IEEE 754 标准
print("inf_hash", hash(inf) == 314159) # Python 64位系统标准哈希常数

# 8. 逻辑运算
print("float_bool", bool(0.0), bool(1.5))
)";

	run_aot_test(shared_driver(),
		"simpleHashAndEquality",
		code,
		{ "eq_check True False",
			"hash_parity True",
			"hash_large True",
			"zero_eq True",
			"abs_neg True",
			"is_int True False",
			"ratio 3 4",
			"round_half 0 2 2 4",
			"to_int 1 -1",
			"from_str True",
			"nan_self False",
			"inf_hash True",
			"float_bool False True" });
}