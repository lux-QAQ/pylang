#pragma once

#include "compiler/Support/Error.hpp"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>// ← CloneModule

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace pylang {

// =============================================================================
// LLVM 错误转字符串
// =============================================================================
inline std::string llvm_error_to_string(llvm::Error err)
{
	std::string msg;
	llvm::raw_string_ostream os(msg);
	os << std::move(err);
	return msg;
}

// =============================================================================
// LLVMModuleLoader — LLVM Module 加载与缓存
// =============================================================================
class LLVMModuleLoader
{
  public:
	explicit LLVMModuleLoader(llvm::LLVMContext &ctx) : m_context(ctx) {}


    std::unique_ptr<llvm::Module> load_from_memory(std::string_view data,
        std::string_view name = "<embedded_runtime>")
    {
        llvm::SMDiagnostic err;
        auto memory_buffer =
            llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(data.data(), data.size()),
                name,
                false
            );

        // [FIX] 使用 m_context
        auto mod = llvm::parseIR(*memory_buffer, err, m_context);
        if (!mod) {
            err.print("LLVMModuleLoader", llvm::errs());
            return nullptr;
        }
        return mod;
    }

	Result<llvm::Module *> load_bitcode(const std::filesystem::path &path)
	{
		auto path_str = path.string();

		auto it = m_cache.find(path_str);
		if (it != m_cache.end()) { return it->second.get(); }

		if (!std::filesystem::exists(path)) {
			return MAKE_ERROR(ErrorKind::BitcodeLoadFailed, "Bitcode file not found: {}", path_str);
		}

		auto buf_or_err = llvm::MemoryBuffer::getFile(path_str);
		if (!buf_or_err) {
			return MAKE_ERROR(ErrorKind::BitcodeLoadFailed, "Failed to read file: {}", path_str);
		}

		auto mod_or_err = llvm::parseBitcodeFile(buf_or_err->get()->getMemBufferRef(), m_context);
		if (!mod_or_err) {
			return MAKE_ERROR(ErrorKind::BitcodeLoadFailed,
				"Failed to parse bitcode: {}",
				llvm_error_to_string(mod_or_err.takeError()));
		}

		PYLANG_TRY_VOID(verify_module(**mod_or_err));

		auto *mod_ptr = mod_or_err->get();
		m_cache[path_str] = std::move(*mod_or_err);
		return mod_ptr;
	}

	Result<llvm::Module *> load_ir(const std::filesystem::path &path)
	{
		auto path_str = path.string();

		auto it = m_cache.find(path_str);
		if (it != m_cache.end()) { return it->second.get(); }

		llvm::SMDiagnostic diag;
		auto mod = llvm::parseIRFile(path_str, diag, m_context);
		if (!mod) {
			return MAKE_ERROR(ErrorKind::BitcodeLoadFailed,
				"Failed to parse IR file {}: {}",
				path_str,
				diag.getMessage().str());
		}

		PYLANG_TRY_VOID(verify_module(*mod));

		auto *mod_ptr = mod.get();
		m_cache[path_str] = std::move(mod);
		return mod_ptr;
	}

	static VoidResult verify_module(const llvm::Module &mod)
	{
		std::string err;
		llvm::raw_string_ostream os(err);
		if (llvm::verifyModule(mod, &os)) {
			return MAKE_ERROR(ErrorKind::VerificationFailed, "Module verification failed: {}", err);
		}
		return {};
	}

	Result<std::unique_ptr<llvm::Module>> clone_module(llvm::Module *mod)
	{
		if (!mod) { return MAKE_ERROR(ErrorKind::InternalError, "Cannot clone null module"); }
		return llvm::CloneModule(*mod);
	}

	llvm::LLVMContext &context() { return m_context; }

  private:
	llvm::LLVMContext &m_context;
	std::unordered_map<std::string, std::unique_ptr<llvm::Module>> m_cache;
};

}// namespace pylang