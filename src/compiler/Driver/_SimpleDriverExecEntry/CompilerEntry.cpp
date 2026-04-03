#include "ast/AST.hpp"

#include "compiler/Driver/_SimpleDriver.hpp"
#include "compiler/Support/Log.hpp"
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "runtime/export/rt_lifecycle.cpp"

#include <cpptrace/cpptrace.hpp>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <iostream>

// 如果 CMake 定义了这个宏，则包含生成的头文件
#ifdef PYLANG_EMBEDDED_RES
#include "EmbeddedResources.hpp"
#endif

using namespace pylang;
namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
// 资源管理器：负责释放嵌入的静态库和 Runtime
// -----------------------------------------------------------------------------
struct ResourceManager
{
	fs::path temp_lib_dir;
	bool is_initialized = false;

	ResourceManager()
	{
		// 创建唯一的临时目录，例如 /tmp/pylangc_static/
		// 简单起见使用固定名称以便于调试，生产环境建议加 PID 或 Random
		auto tmp_root = fs::temp_directory_path();
		temp_lib_dir = tmp_root / "pylangc_static";
	}

	// 释放所有嵌入的资源到磁盘
	// 返回库文件所在目录
	void setup_resources(SimpleDriver::Options &opts)
	{
#ifdef PYLANG_EMBEDDED_RES
		if (!fs::exists(temp_lib_dir)) { fs::create_directories(temp_lib_dir); }

		auto extract = [&](std::string_view name, std::string_view data) {
			fs::path p = temp_lib_dir / name;
			// 只有当文件大小不同或不存在时才写入 (简单缓存策略)
			if (!fs::exists(p) || fs::file_size(p) != data.size()) {
				std::ofstream ofs(p, std::ios::binary);
				ofs.write(data.data(), data.size());
			}
			return p.string();
		};

		log::compiler()->debug("Extracting embedded resources to {}", temp_lib_dir.string());

		// 1. 提取静态库 (文件名需要和 CMake 中链接的内容对应)
		// 这些函数名由 cmake/EmbedResources.cmake 生成
		extract("libspdlog.a", embedded::LIB_SPDLOG());
		extract("libgmp.a", embedded::LIB_GMP());
		extract("libgmpxx.a", embedded::LIB_GMPXX());
		extract("libicuuc.a", embedded::LIB_ICUUC());
		extract("libicudata.a", embedded::LIB_ICUDATA());

		// #ifndef NDEBUG
		//  在 Debug 模式下提取 cpptrace 及其依赖项
		extract("libcpptrace.a", embedded::LIB_CPPTRACE());
		extract("libdwarf.a", embedded::LIB_DWARF());// 新增 libdwarf
		// #endif

#ifdef PYLANG_USE_Boehm_GC
		// 提取 GC 静态库
		extract("libgc.a", embedded::LIB_GC());
#endif

		// 2. 配置 Driver 链接选项
		opts.extra_link_flags.push_back("-L" + temp_lib_dir.string());

		// 告诉链接器链接这些特定的静态库 (注意顺序)
		// #ifndef NDEBUG
		//  链接 cpptrace 及其底层依赖 (注意顺序！)
		opts.extra_link_flags.push_back("-lcpptrace");
		opts.extra_link_flags.push_back("-ldwarf");// 新增: libcpptrace 依赖 libdwarf
		opts.extra_link_flags.push_back("-lzstd");// libdwarf 可能依赖 zstd
		opts.extra_link_flags.push_back("-lz");// libdwarf 依赖 zlib
		opts.extra_link_flags.push_back("-ldl");
		// #endif
		opts.extra_link_flags.push_back("-lspdlog");
		opts.extra_link_flags.push_back("-lgmpxx");
		opts.extra_link_flags.push_back("-lgmp");
		opts.extra_link_flags.push_back("-licuuc");
		opts.extra_link_flags.push_back("-licudata");

#ifdef PYLANG_USE_Boehm_GC
		// 加上 Boehm GC 链接，以及必需的线程支持库
		opts.extra_link_flags.push_back("-lgc");
		opts.extra_link_flags.push_back("-pthread");
#endif

		is_initialized = true;
#else
		log::compiler()->warn("Embedded resources not found. Static linking may fail.");
#endif
	}

	// 尝试获取嵌入的 runtime.bc 并释放到磁盘供 Driver 使用
	// 如果用户没有指定 runtime 路径，这将作为默认回退
	std::string setup_runtime_bc()
	{
#ifdef PYLANG_EMBEDDED_RES
		if (!fs::exists(temp_lib_dir)) { fs::create_directories(temp_lib_dir); }

		fs::path dumped_bc = temp_lib_dir / "runtime.bc";
		std::string_view data = embedded::RUNTIME_BC();

		// 无论如何强制覆盖，避免修改代码后生成的 bc 文件大小恰巧相同导致的缓存更新失效
		{
			std::ofstream ofs(dumped_bc, std::ios::binary);
			ofs.write(data.data(), data.size());
			log::compiler()->debug("Extracted embedded runtime.bc to {}", dumped_bc.string());
		}

		return dumped_bc.string();
#else
		return "";
#endif
	}
};

// -----------------------------------------------------------------------------
// 辅助函数：根据文件名推断模块名
// -----------------------------------------------------------------------------
std::string derive_module_name(const std::string &filepath)
{
	auto path = fs::path(filepath);
	auto stem = path.stem().string();
	if (stem.empty()) return "__main__";
	return stem;
}

// -----------------------------------------------------------------------------
// 辅助函数：查找 runtime.bc
// 优先级:
// 1. 命令行参数 --runtime
// 2. 环境变量 PYLANG_RUNTIME_BC
// 3. 嵌入的资源 (解压到临时目录)
// 4. 当前目录下的 runtime.bc
// 5. 构建目录默认位置 (CMake 传入, 开发环境)
// -----------------------------------------------------------------------------
std::string resolve_runtime_bc(const std::string &cmd_arg, ResourceManager &res_mgr)
{
	// 1. 命令行
	if (!cmd_arg.empty()) return cmd_arg;

	// 2. 环境变量
	const char *env_path = std::getenv("PYLANG_RUNTIME_BC");
	if (env_path && fs::exists(env_path)) return env_path;

	// 3. 嵌入资源 (最优先的默认值，用于分发)
	std::string embedded_path = res_mgr.setup_runtime_bc();
	if (!embedded_path.empty()) return embedded_path;

	// 4. 当前工作目录
	if (fs::exists("runtime.bc")) return "runtime.bc";

	// 5. 开发构建目录默认值
#ifdef PYLANG_RUNTIME_BC_DEFAULT
	if (fs::exists(PYLANG_RUNTIME_BC_DEFAULT)) return PYLANG_RUNTIME_BC_DEFAULT;
#endif

	return "";
}

// -----------------------------------------------------------------------------
// 主逻辑
// -----------------------------------------------------------------------------
int run_compiler(int argc, char **argv)
{
	try {
		cxxopts::Options options("pyc", "Python++ Compiler (AOT)");

		// clang-format off
        options.add_options()
            ("source", "Input Python source file", cxxopts::value<std::string>())
            ("o,output", "Output filename", cxxopts::value<std::string>())
            ("runtime", "Path to runtime.bc", cxxopts::value<std::string>()->default_value(""))
            
            // [修改点 1] 添加 asan 与 force-rebuild 参数，修改 debug-info 的默认值为 true
            ("asan", "Enable AddressSanitizer and frame pointers", cxxopts::value<bool>()->default_value("false"))
            ("force-rebuild", "Force rebuild of runtime cache (runtime.o)", cxxopts::value<bool>()->default_value("false"))
            ("g,debug-info", "Include debug info", cxxopts::value<bool>()->default_value("true"))
            
            ("O,opt-level", "Optimization level (0-3)", cxxopts::value<int>()->default_value("2"))
            
            ("emit-ir", "Emit LLVM IR to stdout (processed)", cxxopts::value<bool>()->default_value("false"))
            ("emit-raw-ir", "Emit raw LLVM IR to stdout (before optimization)", cxxopts::value<bool>()->default_value("false"))
            
            ("no-separate-link", "Disable separate runtime linking (Force LTO-like merging)", cxxopts::value<bool>()->default_value("false"))
            ("d,dump-passes", "Dump IR before/after optimization phases", cxxopts::value<bool>()->default_value("false"))
            
            ("v,verbose", "Enable verbose logging (info)")
            ("vv,trace", "Enable trace logging (debug)")
            ("h,help", "Print usage");
		// clang-format on

		options.parse_positional({ "source" });

		auto result = options.parse(argc, argv);

		if (result.count("help") || !result.count("source")) {
			std::cout << options.help() << std::endl;
			return EXIT_SUCCESS;
		}

		// 1. 配置日志
		if (result.count("trace")) {
			log::set_level(spdlog::level::trace);
		} else if (result.count("verbose")) {
			log::set_level(spdlog::level::debug);
		} else {
			log::set_level(spdlog::level::warn);// 默认只显示警告和错误
		}

		ResourceManager res_mgr;

		// 2. 准备 Driver 选项
		SimpleDriver::Options driver_opts;
		int opt_level = result["opt-level"].as<int>();
		// 限制在 0-3 之间
		opt_level = std::clamp(opt_level, 0, 3);

		driver_opts.opt_level = opt_level;
		driver_opts.extra_link_flags.push_back("-O" + std::to_string(driver_opts.opt_level));

		// [修改点 2] 注入 ASAN, Debug 以及 Frame-Pointer 对应的链接和运行时参数
		if (result["asan"].as<bool>()) {
			driver_opts.extra_link_flags.push_back("-fsanitize=address");
			driver_opts.extra_link_flags.push_back("-fno-omit-frame-pointer");
		}
		if (result["debug-info"].as<bool>()) {
			driver_opts.extra_link_flags.push_back("-g");
			// 如果没开 ASAN 但开了 -g，同样保留帧指针以协助 CppTrace 展开崩溃堆栈
			if (!result["asan"].as<bool>()) {
				driver_opts.extra_link_flags.push_back("-fno-omit-frame-pointer");
			}
		}

		driver_opts.separate_runtime_linking = !result["no-separate-link"].as<bool>();
		driver_opts.dump_ir_before_opt = result["dump-passes"].as<bool>();
		driver_opts.dump_ir_after_opt = result["dump-passes"].as<bool>();

		// [修改点 3] 如果直接指定了 runtime 或是强行重建，则删除本地缓存的 runtime.o
		// 触发强制降级重编
		if (result.count("runtime") || result["force-rebuild"].as<bool>()) {
			std::error_code ec;
			fs::path cache_path = "/tmp/pylang_runtime_cache.o";
			if (fs::exists(cache_path, ec)) {
				fs::remove(cache_path, ec);
				log::compiler()->info("Cleared old runtime.o cache to force fresh rebuild.");
			}
		}

		// 2.1 释放静态库并配置链接参数
		res_mgr.setup_resources(driver_opts);

		// 2.2 确定输出类型
		if (result["emit-ir"].as<bool>()) {
			driver_opts.output_kind = SimpleDriver::OutputKind::IRText;
		} else if (result.count("output")) {
			driver_opts.output_path = result["output"].as<std::string>();
			driver_opts.output_kind = SimpleDriver::OutputKind::Executable;// 默认
		}

		// 2.3 查找并设置 runtime.bc
		driver_opts.runtime_bc_path =
			resolve_runtime_bc(result["runtime"].as<std::string>(), res_mgr);
		if (driver_opts.runtime_bc_path.empty() || !fs::exists(driver_opts.runtime_bc_path)) {
			std::cerr
				<< "Error: runtime.bc not found. Use --runtime, set PYLANG_RUNTIME_BC, or ensure "
				   "embedded resources are built.\n";
			return EXIT_FAILURE;
		}

		// 3. 解析源代码
		std::string source_path = result["source"].as<std::string>();
		std::string module_name = derive_module_name(source_path);

		if (!fs::exists(source_path)) {
			std::cerr << "Error: Input file '" << source_path << "' does not exist.\n";
			return EXIT_FAILURE;
		}

		auto lexer = Lexer::create(fs::absolute(source_path));
		parser::Parser parser(lexer);

		log::compiler()->info("Parsing '{}'...", source_path);
		parser.parse();

		auto mod = parser.module();
		if (!mod) {
			std::cerr << "Error: Parsing failed.\n";
			return EXIT_FAILURE;
		}

		// 4. 初始化 Driver
		llvm::LLVMContext ctx;
		auto driver_res = SimpleDriver::create(std::move(driver_opts), ctx);
		if (!driver_res) {
			std::cerr << "Compiler init failed: " << driver_res.error().message() << "\n";
			return EXIT_FAILURE;
		}
		auto &driver = *driver_res;

		// 5. 执行编译任务
		if (result["emit-raw-ir"].as<bool>()) {
			// 任务: 仅输出未经处理的 IR
			auto ir_res = driver.emit_unoptimized_ir(*mod, module_name);
			if (!ir_res) {
				std::cerr << "Codegen failed: " << ir_res.error().message() << "\n";
				return EXIT_FAILURE;
			}
			std::cout << *ir_res << std::endl;
		} else if (result["emit-ir"].as<bool>()) {
			// 任务: 输出优化后的 IR (包含 runtime 声明或定义)
			auto ir_res = driver.compile_to_ir(*mod, module_name);
			if (!ir_res) {
				std::cerr << "Compilation failed: " << ir_res.error().message() << "\n";
				return EXIT_FAILURE;
			}
			std::cout << *ir_res << std::endl;
		} else {
			// 任务: 编译为可执行文件
			auto path_res = driver.compile_module(*mod, module_name);
			if (!path_res) {
				std::cerr << "Build failed: " << path_res.error().message() << "\n";
				// 输出一些调试建议
#ifdef PYLANG_EMBEDDED_RES
				std::cerr << "Tip: Check if static libraries in " << res_mgr.temp_lib_dir
						  << " match your system architecture.\n";
#endif
				return EXIT_FAILURE;
			}
			std::cout << "Build successful: " << path_res->string() << std::endl;
		}

		return EXIT_SUCCESS;

	} catch (const std::exception &e) {
		std::cerr << "\n[CRITICAL ERROR] Uncaught exception:\n";
		std::cerr << e.what() << "\n\nStack trace:\n";
		cpptrace::generate_trace().print();
		return EXIT_FAILURE;
	}
}

int main(int argc, char **argv)
{
	rt_init();
	// 注册崩溃处理 (Segfault 等)
	cpptrace::register_terminate_handler();
	return run_compiler(argc, argv);
}