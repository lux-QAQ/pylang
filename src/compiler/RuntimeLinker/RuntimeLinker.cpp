#include "RuntimeLinker.hpp"

#include "compiler/Support/StringUtils.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Linker/Linker.h>


namespace pylang {

// =============================================================================
// RuntimeFunction
// =============================================================================
std::vector<std::string_view> RuntimeFunction::params() const
{
    if (param_types.empty()) return {};
    return str::split(param_types, ',');
}

// =============================================================================
// RuntimeLinker::create
// =============================================================================
Result<RuntimeLinker> RuntimeLinker::create(
    LLVMModuleLoader &loader,
    const std::filesystem::path &bitcode_path)
{
    log::linker()->info("Loading runtime bitcode: {}", bitcode_path.string());

    PYLANG_TRY(module, loader.load_bitcode(bitcode_path));

    RuntimeLinker linker;
    linker.m_runtime_module = module;

    PYLANG_TRY_VOID(linker.scan_annotations());

    log::linker()->info("Loaded {} runtime functions", linker.m_functions.size());
    return linker;
}

// =============================================================================
// RuntimeLinker::scan_annotations
// =============================================================================
VoidResult RuntimeLinker::scan_annotations()
{
    auto *annotations = m_runtime_module->getNamedGlobal("llvm.global.annotations");
    if (!annotations) {
        return MAKE_ERROR(
            ErrorKind::AnnotationParseFailed, "No @llvm.global.annotations found in runtime.bc");
    }

    auto *initializer = annotations->getInitializer();
    if (!initializer) {
        return MAKE_ERROR(
            ErrorKind::AnnotationParseFailed, "@llvm.global.annotations has no initializer");
    }

    auto *array = llvm::dyn_cast<llvm::ConstantArray>(initializer);
    if (!array) {
        return MAKE_ERROR(
            ErrorKind::AnnotationParseFailed, "@llvm.global.annotations is not a ConstantArray");
    }

    size_t parsed = 0;
    size_t skipped = 0;

    for (unsigned i = 0; i < array->getNumOperands(); ++i) {
        auto *entry = llvm::dyn_cast<llvm::ConstantStruct>(array->getOperand(i));
        if (!entry || entry->getNumOperands() < 2) {
            log::linker()->warn("Malformed annotation entry at index {}", i);
            ++skipped;
            continue;
        }

        auto *func_val = entry->getOperand(0)->stripPointerCasts();
        auto *func = llvm::dyn_cast<llvm::Function>(func_val);
        if (!func) {
            log::linker()->warn("Annotation entry {} does not point to a function", i);
            ++skipped;
            continue;
        }

        auto *str_gv =
            llvm::dyn_cast<llvm::GlobalVariable>(entry->getOperand(1)->stripPointerCasts());
        if (!str_gv || !str_gv->hasInitializer()) {
            log::linker()->warn("Annotation entry {} has invalid string pointer", i);
            ++skipped;
            continue;
        }

        auto *str_init = str_gv->getInitializer();
        auto *str_data = llvm::dyn_cast<llvm::ConstantDataArray>(str_init);
        if (!str_data || !str_data->isCString()) {
            log::linker()->warn("Annotation entry {} string is not a C string", i);
            ++skipped;
            continue;
        }

        std::string_view annotation_str = str_data->getAsCString();

        if (!annotation_str.starts_with("pylang_export:")) { continue; }

        auto result = parse_annotation(annotation_str, func);
        if (!result) {
            log::linker()->warn(
                "Failed to parse annotation '{}': {}", annotation_str, result.error().message());
            ++skipped;
            continue;
        }

        auto rt_func = *std::move(result);

        if (m_functions.contains(rt_func.name)) {
            log::linker()->warn("Duplicate runtime function name: {}", rt_func.name);
            ++skipped;
            continue;
        }

        log::linker()->debug("Registered: {} (category={}, ret={}, params={})",
            rt_func.name,
            rt_func.category,
            rt_func.return_type,
            rt_func.param_types);

        m_functions[rt_func.name] = std::move(rt_func);
        ++parsed;
    }

    if (parsed == 0) {
        return MAKE_ERROR(
            ErrorKind::AnnotationParseFailed, "No valid pylang_export annotations found");
    }

    log::linker()->info("Parsed {} annotations, skipped {}", parsed, skipped);
    return {};
}

// =============================================================================
// RuntimeLinker::parse_annotation
// =============================================================================
Result<RuntimeFunction> RuntimeLinker::parse_annotation(
    std::string_view annotation_str,
    llvm::Function *func)
{
    auto parts = str::split(annotation_str, ':');
    if (parts.size() != 5) {
        return MAKE_ERROR(ErrorKind::AnnotationParseFailed,
            "Expected 5 fields (got {}): {}",
            parts.size(),
            annotation_str);
    }

    auto category = std::string(str::trim(parts[1]));
    auto name = std::string(str::trim(parts[2]));
    auto ret = std::string(str::trim(parts[3]));
    auto params = std::string(str::trim(parts[4]));

    if (name.empty()) {
        return MAKE_ERROR(ErrorKind::AnnotationParseFailed,
            "Empty function name in annotation: {}",
            annotation_str);
    }

    auto *func_type = func->getFunctionType();
    size_t expected_params = func_type->getNumParams();
    size_t declared_params = params.empty() ? 0 : str::count(params, ',') + 1;

    if (expected_params != declared_params) {
        log::linker()->warn(
            "Function {} param count mismatch: FunctionType has {}, annotation declares {}",
            name,
            expected_params,
            declared_params);
    }

    return RuntimeFunction{
        .category = std::move(category),
        .name = std::move(name),
        .return_type = std::move(ret),
        .param_types = std::move(params),
        .llvm_func = func,
        .llvm_func_type = func_type,
    };
}

// =============================================================================
// RuntimeLinker::get_function
// =============================================================================
Result<const RuntimeFunction *> RuntimeLinker::get_function(std::string_view name) const
{
    auto it = m_functions.find(std::string(name));
    if (it == m_functions.end()) {
        return MAKE_ERROR(
            ErrorKind::RuntimeFunctionNotFound, "Runtime function not found: {}", name);
    }
    return &it->second;
}

// =============================================================================
// RuntimeLinker::list_by_category
// =============================================================================
std::vector<const RuntimeFunction *> RuntimeLinker::list_by_category(
    std::string_view category) const
{
    std::vector<const RuntimeFunction *> result;
    for (const auto &[name, func] : m_functions) {
        if (func.category == category) { result.push_back(&func); }
    }
    return result;
}

// =============================================================================
// RuntimeLinker::declare_in
// =============================================================================
// llvm::Function *RuntimeLinker::declare_in(llvm::Module *user_module, std::string_view name)
// {
//     std::string name_str(name);

//     auto it = m_declared.find(name_str);
//     if (it != m_declared.end()) { return it->second; }

//     auto result = get_function(name);
//     if (!result) {
//         log::linker()->error("Cannot declare unknown function: {}", name);
//         return nullptr;
//     }

//     const auto *rt_func = *result;

//     auto *decl = llvm::Function::Create(rt_func->llvm_func_type,
//         llvm::Function::ExternalLinkage,
//         rt_func->llvm_func->getName(),
//         user_module);

//     decl->copyAttributesFrom(rt_func->llvm_func);

//     m_declared[name_str] = decl;
//     log::linker()->debug("Declared {} in user module", name);
//     return decl;
// }
// llvm::Function *RuntimeLinker::declare_in(llvm::Module *user_module, std::string_view name)
// {
//     std::string name_str(name);

//     auto it = m_declared.find(name_str);
//     if (it != m_declared.end()) { return it->second; }

//     auto result = get_function(name);
//     if (!result) {
//         log::linker()->error("Cannot declare unknown function: {}", name);
//         return nullptr;
//     }

//     const auto *rt_func = *result;

//     auto *decl = llvm::Function::Create(rt_func->llvm_func_type,
//         llvm::Function::ExternalLinkage,
//         rt_func->llvm_func->getName(),
//         user_module);

//     decl->copyAttributesFrom(rt_func->llvm_func);

//     // runtime.bc 中的函数用 C++ 异常（invoke/landingpad），
//     // 带有 personality function（__gxx_personality_v0）。
//     // extern 声明不包含函数体，不需要也不能跨模块引用 personality。
//     if (decl->hasPersonalityFn()) { decl->setPersonalityFn(nullptr); }

//     m_declared[name_str] = decl;
//     log::linker()->debug("Declared {} in user module", name);
//     return decl;
// }

llvm::Function *RuntimeLinker::declare_in(llvm::Module *user_module, std::string_view name)
{
    std::string name_str(name);

    // Fast path: 缓存命中
    if (m_options.enable_function_cache) {
        auto mod_it = m_cache.find(user_module);
        if (mod_it != m_cache.end()) {
            auto func_it = mod_it->second.find(name_str);
            if (func_it != mod_it->second.end()) {
                // 可以在此加入 assert(func_it->second->getParent() == user_module) 
                // 来检测悬垂指针，但这需要访问可能无效的内存，只能靠上层保证生命周期
                return func_it->second;
            }
        }
    }

    // Slow path: 全流程查找与创建

    // 1. 查找元数据
    auto result = get_function(name);
    if (!result) {
        log::linker()->error("Cannot declare unknown function: {}", name);
        // 缓存失败结果为 nullptr？不，通常直接返回
        return nullptr;
    }
    const auto *rt_func = *result;
    
    // 2. 获取 mangled name
    std::string mangled_name = rt_func->llvm_func->getName().str();

    // 3. 检查 Module 符号表 (LLVM 自身缓存)
    llvm::Function *decl = user_module->getFunction(mangled_name);
    
    if (!decl) {
        // 4. 创建新声明
        decl = llvm::Function::Create(rt_func->llvm_func_type,
            llvm::Function::ExternalLinkage,
            mangled_name,
            user_module);
        decl->copyAttributesFrom(rt_func->llvm_func);

        if (decl->hasPersonalityFn()) { decl->setPersonalityFn(nullptr); }

        log::linker()->debug("Declared {} in module {}", name, user_module->getName().str());
    }

    // 5. 更新缓存
    if (m_options.enable_function_cache && decl) {
        m_cache[user_module][name_str] = decl;
    }

    return decl;
}

// =============================================================================
// 缓存管理
// =============================================================================

void RuntimeLinker::forget_module(llvm::Module *module)
{
    if (m_options.enable_function_cache) {
        m_cache.erase(module);
    }
}

void RuntimeLinker::clear_cache()
{
    m_cache.clear();
}

// =============================================================================
// RuntimeLinker::pyobject_ptr_type
// =============================================================================
llvm::Type *RuntimeLinker::pyobject_ptr_type() const
{
    return llvm::PointerType::getUnqual(m_runtime_module->getContext());
}

// =============================================================================
// RuntimeLinker::print_registry
// =============================================================================
void RuntimeLinker::print_registry() const
{
    std::unordered_map<std::string, std::vector<std::string>> by_category;
    for (const auto &[name, func] : m_functions) {
        by_category[func.category].push_back(name);
    }

    log::linker()->info("Runtime function registry ({} total):", m_functions.size());
    for (const auto &[cat, names] : by_category) {
        log::linker()->info("  [{}]: {} functions", cat, names.size());
        for (const auto &n : names) {
            log::linker()->debug("    - {}", n);
        }
    }
}

// =============================================================================
// RuntimeLinker::link_into
// =============================================================================
VoidResult RuntimeLinker::link_into(llvm::Module *user_module)
{
    if (!m_runtime_module) {
        return MAKE_ERROR(ErrorKind::InternalError, "Runtime module not loaded");
    }

    auto cloned = llvm::CloneModule(*m_runtime_module);
    llvm::Linker linker(*user_module);

    if (linker.linkInModule(std::move(cloned), llvm::Linker::OverrideFromSrc)) {
        return MAKE_ERROR(ErrorKind::LinkError, "Failed to link runtime module");
    }

    // 链接后，所有声明变成了定义 (Declaration -> Definition)
    // 指针地址未变，Value 类型从 Declaration 变为 Function (with body)，
    // 缓存依然有效。
    
    log::linker()->info("Successfully linked runtime module into {}", user_module->getName().str());
    return {};
}

} // namespace pylang