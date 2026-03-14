#include "_SimpleDriver.hpp"

#include "compiler/Support/ScopeTimer.hpp"

#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Bitcode/BitcodeReader.h>// [New] needed for precompile
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include <cstdlib>
#include <format>
#include <mutex>

namespace pylang {

// =============================================================================
// 静态初始化
// =============================================================================

VoidResult SimpleDriver::initialize_targets()
{
	static std::once_flag s_flag;
	static bool s_ok = false;

	std::call_once(s_flag, [] {
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		s_ok = true;
	});

	if (!s_ok) {
		return MAKE_ERROR(ErrorKind::InternalError, "Failed to initialize LLVM native target");
	}
	return {};
}

Result<std::unique_ptr<llvm::TargetMachine>> SimpleDriver::create_host_target_machine(int opt_level)
{
	auto triple_str = llvm::sys::getDefaultTargetTriple();
	std::string error;
	const auto *target = llvm::TargetRegistry::lookupTarget(triple_str, error);
	if (!target) {
		return MAKE_ERROR(
			ErrorKind::InternalError, "Target lookup failed for '{}': {}", triple_str, error);
	}

	auto cg_opt = [&]() -> llvm::CodeGenOptLevel {
		switch (opt_level) {
		case 0:
			return llvm::CodeGenOptLevel::None;
		case 1:
			return llvm::CodeGenOptLevel::Less;
		case 3:
			return llvm::CodeGenOptLevel::Aggressive;
		default:
			return llvm::CodeGenOptLevel::Default;
		}
	}();

	llvm::TargetOptions target_opts;
	auto *tm = target->createTargetMachine(
		triple_str, "generic", "", target_opts, llvm::Reloc::PIC_, std::nullopt, cg_opt);

	if (!tm) { return MAKE_ERROR(ErrorKind::InternalError, "Failed to create TargetMachine"); }

	log::compiler()->info("TargetMachine: {} (opt=O{})", triple_str, opt_level);
	return std::unique_ptr<llvm::TargetMachine>(tm);
}

// =============================================================================
// 工厂方法
// =============================================================================

SimpleDriver::SimpleDriver(Options opts,
	llvm::LLVMContext &ctx,
	LLVMModuleLoader loader,
	RuntimeLinker linker,
	std::unique_ptr<llvm::TargetMachine> tm,
	std::filesystem::path cached_runtime_obj)
	: m_opts(std::move(opts)), m_ctx(&ctx), m_loader(std::move(loader)),
	  m_linker(std::move(linker)), m_target_machine(std::move(tm)),
	  m_cached_runtime_obj(std::move(cached_runtime_obj))
{}

Result<std::filesystem::path> SimpleDriver::precompile_runtime_module(
    const std::filesystem::path &bc_path,
    llvm::LLVMContext &ctx,
    llvm::TargetMachine &tm)
{
    // 简单的缓存策略：如果 /tmp/pylang_runtime.o 存在且较新，直接复用
    // 实际生产环境应更严谨地处理版本和哈希
    auto cache_path = std::filesystem::temp_directory_path() / "pylang_runtime_cache.o";

    // [Fix] 检查时间戳：只有当缓存文件存在且修改时间晚于等于源 BC 文件时才复用
    bool use_cache = false;
    std::error_code ec;

    if (std::filesystem::exists(cache_path, ec) && std::filesystem::file_size(cache_path, ec) > 0) {
        if (std::filesystem::exists(bc_path, ec)) {
            auto bc_time = std::filesystem::last_write_time(bc_path, ec);
            if (!ec) {
                auto cache_time = std::filesystem::last_write_time(cache_path, ec);
                // 只有缓存时间戳 >= 源文件时间戳才有效
                if (!ec && cache_time >= bc_time) {
                    use_cache = true;
                } else {
                    log::compiler()->info("Runtime module updated (cache outdated), recompiling...");
                }
            }
        }
    }

    if (use_cache) {
        return cache_path;
    }

    PYLANG_TIMER_INFO("precompile_runtime");
    log::compiler()->info("Pre-compiling runtime to object file: {}", cache_path.string());

    llvm::SMDiagnostic err;
	auto mod = llvm::parseIRFile(bc_path.string(), err, ctx);
	if (!mod) {
		return MAKE_ERROR(ErrorKind::IOError,
			"Failed to load runtime for precompilation: {}",
			err.getMessage().str());
	}

	mod->setDataLayout(tm.createDataLayout());
	mod->setTargetTriple(tm.getTargetTriple().str());

	//std::error_code ec;
	llvm::raw_fd_ostream dest(cache_path.string(), ec, llvm::sys::fs::OF_None);
	if (ec) { return MAKE_ERROR(ErrorKind::IOError, "Cannot open cache file: {}", ec.message()); }

	llvm::legacy::PassManager emit_pm;
	if (tm.addPassesToEmitFile(emit_pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
		return MAKE_ERROR(ErrorKind::InternalError, "TargetMachine cannot emit object file");
	}

	emit_pm.run(*mod);
	dest.flush();

	return cache_path;
}

Result<SimpleDriver> SimpleDriver::create(Options opts, llvm::LLVMContext &ctx)
{
	PYLANG_TIMER_INFO("SimpleDriver::create");

	// 1. 初始化 LLVM 目标
	PYLANG_TRY_VOID(initialize_targets());

	// 2. 创建 TargetMachine
	PYLANG_TRY(tm, create_host_target_machine(opts.opt_level));

	// 3. (新) 处理分离编译缓存
	std::filesystem::path runtime_obj_path;
	if (opts.separate_runtime_linking && opts.output_kind == OutputKind::Executable) {
		// 如果开启分离编译，尝试预编译 runtime
		auto res = precompile_runtime_module(opts.runtime_bc_path, ctx, *tm);
		if (res) {
			runtime_obj_path = *res;
			log::compiler()->info("Using separated runtime compilation (cached object: {})",
				runtime_obj_path.string());
		} else {
			log::compiler()->warn("Separate compilation failed, falling back to IR merging: {}",
				res.error().message());
			opts.separate_runtime_linking = false;
		}
	}

	// 4. 加载 runtime.bc (即使分离编译也需要加载 BC 用于符号解析和声明)
	LLVMModuleLoader loader(ctx);
	PYLANG_TRY(linker, RuntimeLinker::create(loader, opts.runtime_bc_path));

	log::compiler()->info("SimpleDriver created (opt: O{}, separate_link: {})",
		opts.opt_level,
		opts.separate_runtime_linking);

	return SimpleDriver(std::move(opts),
		ctx,
		std::move(loader),
		std::move(linker),
		std::move(tm),
		std::move(runtime_obj_path));
}

// =============================================================================
// 管道阶段实现
// =============================================================================

Result<std::unique_ptr<llvm::Module>> SimpleDriver::stage_codegen(ast::Module &module,
	std::string_view name)
{
	PYLANG_TIMER_INFO("stage_codegen");
	log::compiler()->info("Stage 1: Codegen for '{}'", name);

	return PylangCodegen::compile(&module, name, *m_ctx, m_linker)
		.transform([](PylangCodegen::CompileResult cr) { return std::move(cr.module); });
}

Result<std::unique_ptr<llvm::Module>> SimpleDriver::stage_link_runtime(
	std::unique_ptr<llvm::Module> user_mod)
{
	PYLANG_TIMER_INFO("stage_link_runtime");

	// [Fix] 如果开启了分离编译，直接跳过 IR 链接
	if (m_opts.separate_runtime_linking) {
		log::compiler()->info("Stage 2: Skipping IR linking (separate compilation active)");
		// RuntimeLinker 仍然需要确保 Module 里有必要的声明，但通常 PylangCodegen 已经处理了
		return std::move(user_mod);
	}

	log::compiler()->info("Stage 2: Linking runtime into '{}'", user_mod->getName().str());

	return m_linker.link_into(user_mod.get()).transform([&user_mod]() {
		return std::move(user_mod);
	});
}

VoidResult SimpleDriver::stage_optimize(llvm::Module &module)
{
	PYLANG_TIMER_INFO("stage_optimize");
	log::compiler()->info(
		"Stage 3: Optimizing '{}' at O{}", module.getName().str(), m_opts.opt_level);

	if (m_opts.dump_ir_before_opt) {
		log::opt()->info("=== IR Before Optimization ===");
		module.print(llvm::errs(), nullptr);
	}

	if (m_opts.opt_level == 0) {
		log::opt()->info("Skipping optimization (O0)");
		return {};
	}

	// 新 PassManager 管道
	llvm::PassBuilder pb(m_target_machine.get());

	llvm::LoopAnalysisManager lam;
	llvm::FunctionAnalysisManager fam;
	llvm::CGSCCAnalysisManager cgam;
	llvm::ModuleAnalysisManager mam;

	pb.registerModuleAnalyses(mam);
	pb.registerCGSCCAnalyses(cgam);
	pb.registerFunctionAnalyses(fam);
	pb.registerLoopAnalyses(lam);
	pb.crossRegisterProxies(lam, fam, cgam, mam);

	auto opt_level = [&]() -> llvm::OptimizationLevel {
		switch (m_opts.opt_level) {
		case 1:
			return llvm::OptimizationLevel::O1;
		case 3:
			return llvm::OptimizationLevel::O3;
		default:
			return llvm::OptimizationLevel::O2;
		}
	}();

	auto mpm = pb.buildPerModuleDefaultPipeline(opt_level);
	mpm.run(module, mam);

	if (m_opts.dump_ir_after_opt) {
		log::opt()->info("=== IR After Optimization ===");
		module.print(llvm::errs(), nullptr);
	}

	// 优化后验证
	if (m_opts.verify_module) {
		std::string err;
		llvm::raw_string_ostream os(err);
		if (llvm::verifyModule(module, &os)) {
			return MAKE_ERROR(
				ErrorKind::VerificationFailed, "Post-optimization verification failed: {}", err);
		}
	}

	log::opt()->info("Optimization complete");
	return {};
}

Result<std::string> SimpleDriver::stage_to_ir_string(llvm::Module &module)
{
	std::string ir;
	llvm::raw_string_ostream os(ir);
	module.print(os, nullptr);
	return ir;
}

Result<std::filesystem::path> SimpleDriver::stage_to_object(llvm::Module &module,
	const std::filesystem::path &output)
{
	PYLANG_TIMER_INFO("stage_to_object");
	log::compiler()->info("Stage 4b: Emitting object file → {}", output.string());

	module.setDataLayout(m_target_machine->createDataLayout());
	module.setTargetTriple(m_target_machine->getTargetTriple().str());

	std::error_code ec;
	llvm::raw_fd_ostream dest(output.string(), ec, llvm::sys::fs::OF_None);
	if (ec) {
		return MAKE_ERROR(
			ErrorKind::IOError, "Cannot open output file '{}': {}", output.string(), ec.message());
	}

	llvm::legacy::PassManager emit_pm;
	if (m_target_machine->addPassesToEmitFile(
			emit_pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
		return MAKE_ERROR(ErrorKind::InternalError, "TargetMachine cannot emit object file");
	}

	emit_pm.run(module);
	dest.flush();

	log::compiler()->info("Object file written: {}", output.string());
	return output;
}

Result<std::filesystem::path> SimpleDriver::stage_to_bitcode(llvm::Module &module,
	const std::filesystem::path &output)
{
	std::error_code ec;
	llvm::raw_fd_ostream dest(output.string(), ec, llvm::sys::fs::OF_None);
	if (ec) {
		return MAKE_ERROR(
			ErrorKind::IOError, "Cannot open output file '{}': {}", output.string(), ec.message());
	}

	llvm::WriteBitcodeToFile(module, dest);
	dest.flush();
	return output;
}

Result<std::filesystem::path> SimpleDriver::stage_link_executable(
	const std::filesystem::path &obj_path)
{
	PYLANG_TIMER_INFO("stage_link_executable");

	auto exe_path =
		m_opts.output_path.empty() ? obj_path.parent_path() / obj_path.stem() : m_opts.output_path;

	// 构建链接命令
	std::string cmd =
		std::format("{} -o {} {}", m_opts.linker_cmd, exe_path.string(), obj_path.string());

	// [Fix] 链接预编译的 runtime 对象文件
	if (m_opts.separate_runtime_linking && !m_cached_runtime_obj.empty()) {
		cmd += " " + m_cached_runtime_obj.string();
	}

	// 添加必要的系统库
	//cmd += " -lstdc++ -lm -lpthread";

	// 添加用户指定的额外链接参数
	for (const auto &flag : m_opts.extra_link_flags) { cmd += " " + flag; }

	log::compiler()->info("Stage 5: Linking → {}", exe_path.string());
	log::compiler()->debug("Link command: {}", cmd);

	int ret = std::system(cmd.c_str());
	if (ret != 0) {
		return MAKE_ERROR(
			ErrorKind::LinkError, "System linker failed (exit code {}): {}", ret, cmd);
	}

	log::compiler()->info("Executable written: {}", exe_path.string());
	return exe_path;
}

// =============================================================================
// 输出路径推断
// =============================================================================

std::filesystem::path SimpleDriver::infer_output_path(std::string_view module_name,
	OutputKind kind) const
{
	if (!m_opts.output_path.empty()) { return m_opts.output_path; }

	auto base = std::filesystem::temp_directory_path() / std::string(module_name);
	switch (kind) {
	case OutputKind::IRText:
		return base.replace_extension(".ll");
	case OutputKind::Bitcode:
		return base.replace_extension(".bc");
	case OutputKind::ObjectFile:
		return base.replace_extension(".o");
	case OutputKind::Executable:
		return base;
	}
	return base;
}

// =============================================================================
// 公开入口 — 管道组合
// =============================================================================

Result<std::string> SimpleDriver::emit_unoptimized_ir(ast::Module &module,
	std::string_view module_name)
{
	// AST → codegen → IR text (不链接 runtime, 不优化)
	return stage_codegen(module, module_name).and_then([](auto mod) {
		return stage_to_ir_string(*mod);
	});
}

Result<std::string> SimpleDriver::compile_to_ir(ast::Module &module, std::string_view module_name)
{
	// AST → codegen → link runtime → optimize → IR text
	return stage_codegen(module, module_name)
		.and_then([this](auto mod) { return stage_link_runtime(std::move(mod)); })
		.and_then([this](auto mod) -> Result<std::unique_ptr<llvm::Module>> {
			auto r = stage_optimize(*mod);
			if (!r) { return std::unexpected(std::move(r).error()); }
			return std::move(mod);
		})
		.and_then([](auto mod) { return stage_to_ir_string(*mod); });
}

Result<std::filesystem::path> SimpleDriver::compile_module(ast::Module &module,
	std::string_view module_name)
{
	auto output = infer_output_path(module_name, m_opts.output_kind);

	// AST → codegen → link runtime → optimize → emit → link
	return stage_codegen(module, module_name)
		.and_then([this](auto mod) { return stage_link_runtime(std::move(mod)); })
		.and_then([this](auto mod) -> Result<std::unique_ptr<llvm::Module>> {
			auto r = stage_optimize(*mod);
			if (!r) { return std::unexpected(std::move(r).error()); }
			return std::move(mod);
		})
		.and_then([this, &output](auto mod) -> Result<std::filesystem::path> {
			switch (m_opts.output_kind) {
			case OutputKind::IRText: {
				return stage_to_ir_string(*mod).and_then(
					[&output](const std::string &ir) -> Result<std::filesystem::path> {
						std::error_code ec;
						llvm::raw_fd_ostream os(output.string(), ec, llvm::sys::fs::OF_Text);
						if (ec) {
							return MAKE_ERROR(
								ErrorKind::IOError, "Cannot write IR: {}", ec.message());
						}
						os << ir;
						return output;
					});
			}
			case OutputKind::Bitcode:
				return stage_to_bitcode(*mod, output);

			case OutputKind::ObjectFile:
				return stage_to_object(*mod, output);

			case OutputKind::Executable: {
				auto obj = output;
				obj.replace_extension(".o");
				return stage_to_object(*mod, obj).and_then(
					[this](auto obj_path) { return stage_link_executable(obj_path); });
			}
			}
			return MAKE_ERROR(ErrorKind::InternalError, "Unknown output kind");
		});
}

}// namespace pylang