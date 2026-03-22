#include "PyFunction.hpp"
#include "PyBoundMethod.hpp"
#include "PyCode.hpp"
#include "PyDict.hpp"
#include "PyNone.hpp"
#include "PyString.hpp"

#include "executable/Program.hpp"
#include "interpreter/InterpreterCore.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/RuntimeError.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


#include <variant>

#include "memory/GCVectorUtils.hpp"// 新增 include

namespace py {

PyFunction::PyFunction(PyType *type) : PyBaseObject(type) {}

PyFunction::PyFunction(std::vector<Value> defaults,
	std::vector<Value> kwonly_defaults,
	PyCode *code,
	PyTuple *closure,
	PyObject *globals)
	: PyBaseObject(types::BuiltinTypes::the().function()), m_code(code), m_globals(globals),
	  m_defaults(std::move(defaults)), m_kwonly_defaults(std::move(kwonly_defaults)),
	  m_closure(closure)
{
	auto name_ = PyString::create(code->name());
	if (name_.is_err()) { TODO(); }
	m_name = name_.unwrap();

	m_qualname = m_name;

	// FIXME: get the docstring from PyCode
	auto doc_ = PyString::create("");
	if (doc_.is_err()) { TODO(); }
	m_doc = doc_.unwrap();

	auto dict_ = PyDict::create();
	if (dict_.is_err()) { TODO(); }
	m_dict = dict_.unwrap();
	m_attributes = m_dict;

	if (!m_closure) { m_closure = PyTuple::create().unwrap(); }

	if (auto g = as<PyDict>(globals)) {
		if (auto it = g->map().find(String{ "__name__" }); it != g->map().end()) {
			m_module = PyObject::from(it->second).unwrap();
		}
	} else {
		auto it = globals->getitem(PyString::create("__name__").unwrap());
		ASSERT(!it.is_err());
		if (it.is_ok()) { m_module = it.unwrap(); }
	}
}


// =============================================================================
// AOT 编译器支持
// =============================================================================


// PyResult<PyNativeFunction *> PyNativeFunction::create_aot(std::string name,
// 	void *code_ptr,
// 	PyObject *module,
// 	PyObject *defaults,
// 	PyObject *kwdefaults,
// 	PyObject *closure)
// {
// 	PyNativeFunction *result = nullptr;

// 	if (closure && closure != py_none()) {
// 		// 有闭包: 签名 PyObject*(PyObject* module, PyObject* closure, PyTuple* args, PyDict*
// 		// kwargs)
// 		auto fn = reinterpret_cast<AOTClosureFuncPtr>(code_ptr);
// 		PyObject *captured_closure = closure;
// 		PyObject *captured_module = module;

// 		FreeFunctionType func = [fn, captured_module, captured_closure](
// 									PyTuple *args, PyDict *kwargs) -> PyResult<PyObject *> {
// 			auto *r = fn(captured_module, captured_closure, args, kwargs);
// 			if (!r) { return Err(runtime_error("compiled closure returned null")); }
// 			return Ok(r);
// 		};

// 		result = PYLANG_ALLOC(
// 			PyNativeFunction, std::move(name), FunctionType{ std::move(func) }, nullptr);
// 	} else {
// 		// 无闭包: 签名 PyObject*(PyObject* module, PyTuple* args, PyDict* kwargs)
// 		auto fn = reinterpret_cast<AOTFuncPtr>(code_ptr);
// 		PyObject *captured_module = module;

// 		FreeFunctionType func = [fn, captured_module](
// 									PyTuple *args, PyDict *kwargs) -> PyResult<PyObject *> {
// 			auto *r = fn(captured_module, args, kwargs);
// 			if (!r) { return Err(runtime_error("compiled function returned null")); }
// 			return Ok(r);
// 		};

// 		result = PYLANG_ALLOC(
// 			PyNativeFunction, std::move(name), FunctionType{ std::move(func) }, nullptr);
// 	}

// 	if (!result) { return Err(memory_error(sizeof(PyNativeFunction))); }

// 	result->m_closure =
// 		(closure && closure != py_none()) ? static_cast<PyTuple *>(closure) : nullptr;
// 	result->m_module_ref = module;

// 	if (closure && closure != py_none()) { result->m_captures.push_back(closure); }
// 	if (defaults) { result->m_captures.push_back(defaults); }
// 	if (kwdefaults) { result->m_captures.push_back(kwdefaults); }
// 	if (module) { result->m_captures.push_back(module); }

// 	return Ok(result);
// }


PyResult<PyNativeFunction *> PyNativeFunction::create_aot(std::string name,
	void *code_ptr,
	PyObject *module,
	PyObject *defaults,
	PyObject *kwdefaults,
	PyObject *closure)
{
	// 统一强制转换为 5 参数签名
	auto raw_fn = reinterpret_cast<AOTRawFuncPtr>(code_ptr);
	PyObject *captured_closure = (closure && closure != py_none()) ? closure : nullptr;
	PyObject *captured_module = module;

	// 桥接函数：用于从 Python (PyTuple) 环境调用 AOT 函数
	FreeFunctionType bridge_func = [raw_fn, captured_module, captured_closure](
									   PyTuple *args, PyDict *kwargs) -> PyResult<PyObject *> {
		// ✅ 优化：直接使用 GCVector::data()，不再使用 to_std_vector 触发堆分配
		auto *r = raw_fn(captured_module,
			captured_closure,
			args->elements().data(),
			static_cast<int32_t>(args->size()),
			kwargs);
		if (!r) { return Err(runtime_error("AOT function returned NULL")); }
		return Ok(r);
	};

	auto *result = PYLANG_ALLOC(
		PyNativeFunction, std::move(name), FunctionType{ std::move(bridge_func) }, nullptr);
	if (!result) { return Err(memory_error(sizeof(PyNativeFunction))); }

	// 关键：保存原始指针用于 call_raw 快速路径
	result->m_aot_ptr = raw_fn;
	result->m_closure = static_cast<PyTuple *>(captured_closure);
	result->m_module_ref = module;

	if (captured_closure) { result->m_captures.push_back(captured_closure); }
	if (defaults && defaults != py_none()) { result->m_captures.push_back(defaults); }
	if (kwdefaults && kwdefaults != py_none()) { result->m_captures.push_back(kwdefaults); }
	if (module) { result->m_captures.push_back(module); }

	return Ok(result);
}

void PyFunction::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	if (m_name) visitor.visit(*m_name);
	if (m_doc) visitor.visit(*m_doc);
	if (m_code) visitor.visit(*m_code);
	if (m_globals) visitor.visit(*m_globals);
	if (m_dict) visitor.visit(*m_dict);
	for (const auto &el : m_defaults) {
		if (std::holds_alternative<PyObject *>(el)) {
			if (std::get<PyObject *>(el)) { visitor.visit(*std::get<PyObject *>(el)); }
		}
	}
	for (const auto &el : m_kwonly_defaults) {
		if (std::holds_alternative<PyObject *>(el)) {
			if (std::get<PyObject *>(el)) { visitor.visit(*std::get<PyObject *>(el)); }
		}
	}
	if (m_closure) visitor.visit(*m_closure);
	if (m_module) visitor.visit(*m_module);
	if (m_qualname) visitor.visit(*m_qualname);
}

PyType *PyFunction::static_type() const { return types::function(); }

PyResult<PyObject *> PyFunction::__repr__() const
{
	return PyString::create(m_qualname).and_then([this](PyString *qualname) {
		return PyString::create(
			fmt::format("<function {} at {}>", qualname->value(), static_cast<const void *>(this)));
	});
}

PyResult<PyObject *> PyFunction::__get__(PyObject *instance, PyObject * /*owner*/) const
{
	if (!instance || instance == py_none()) { return Ok(const_cast<PyFunction *>(this)); }
	return PyBoundMethod::create(instance, const_cast<PyFunction *>(this));
}

PyResult<PyObject *> PyFunction::call_with_frame(PyObject *ns, PyTuple *args, PyDict *kwargs) const
{
	// 将可能的 GCVector 转为 std::vector<Value> 以匹配函数签名
	std::vector<Value> closure_elements;
	if (m_closure) {
		closure_elements = py::to_std_vector(m_closure->elements());
	} else {
		closure_elements = std::vector<Value>{};
	}

	return m_code->eval(m_globals,
		ns,
		args,
		kwargs,
		m_defaults,
		m_kwonly_defaults,
		std::move(closure_elements),
		m_name);
}

// PyResult<PyObject *> PyNativeFunction::call_raw(std::span<Value> args, PyDict *kwargs)
// {
//     // [防御性检查]：防止 AOT 编译器生成错误的 argc (-1) 导致崩溃
//     if (args.size() > 0x1000000) {
//         return Err(runtime_error("Invalid argument count in call_raw"));
//     }

//     // 1. 优先走 AOT 快速路径
//     if (m_aot_ptr) {
//         auto *res = m_aot_ptr(
//             m_module_ref, m_closure, args.data(), static_cast<int32_t>(args.size()), kwargs);
//         if (!res) return Err(runtime_error("AOT execution returned NULL (possible exception)"));
//         return Ok(res);
//     }

//     // 2. 处理已经通过 __get__ 绑定的原生方法
//     if (m_self) {
//         // 创建临时栈空间，将 self 插入第一项
//         size_t total_argc = args.size() + 1;
//         Value* stack_args = static_cast<Value*>(alloca(sizeof(Value) * total_argc));
//         new (&stack_args[0]) Value(m_self);
//         std::copy(args.begin(), args.end(), stack_args + 1);

//         // 注意：此处必须使用 std::span 显式调用基类的 call_raw 或直接分发
//         auto res = PyObject::call_raw(std::span<Value>(stack_args, total_argc), kwargs);

//         for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&stack_args[i]);
//         return res;
//     }

//     return PyObject::call_raw(args, kwargs);
// }

PyResult<PyObject *> PyNativeFunction::call_raw(std::span<const Value> args, PyDict *kwargs)
{

	spdlog::debug("[NativeFunc::call_raw] fn='{}', argc={}, has_aot={}, has_self={}",
		m_name,
		args.size(),
		m_aot_ptr != nullptr,
		m_self != nullptr);

	// 防御非法 argc
	if (args.size() > 1000000) {
		spdlog::critical("[call_raw] Corrupt argc detected: {}", args.size());
		return Err(runtime_error("Corrupt execution stack: invalid argument count"));
	}

	// 1. 如果有 self (BoundMethod 路径)，需要 prepend self
    if (m_self) {
        size_t total_argc = args.size() + 1;
        Value *stack_args = static_cast<Value *>(alloca(sizeof(Value) * total_argc));

        new (&stack_args[0]) Value(m_self);
        for (size_t i = 0; i < args.size(); ++i) { new (&stack_args[i + 1]) Value(args[i]); }

        // [修复编译错误]：使用 IIFE 立即初始化 PyResult
        auto result = [&]() -> PyResult<PyObject *> {
            if (m_aot_ptr) {
                auto *res = m_aot_ptr(
                    m_module_ref, m_closure, stack_args, static_cast<int32_t>(total_argc), kwargs);
                if (res) return Ok(res);
                return Err(runtime_error("AOT call failed"));
            }
            return PyObject::call_raw(std::span<const Value>(stack_args, total_argc), kwargs);
        }();

        for (size_t i = 0; i < total_argc; ++i) { std::destroy_at(&stack_args[i]); }
        return result;
    }

    // 2. 原始函数快速路径
    if (m_aot_ptr) {
        auto *res = m_aot_ptr(
            m_module_ref, m_closure, args.data(), static_cast<int32_t>(args.size()), kwargs);
        if (res) return Ok(res);
        return Err(runtime_error("AOT call failed"));
    }

    return PyObject::call_raw(args, kwargs);
}

PyResult<PyObject *> PyFunction::__call__(PyTuple *args, PyDict *kwargs)
{
	auto function_locals = PyDict::create();
	if (function_locals.is_err()) { return function_locals; }
	return call_with_frame(function_locals.unwrap(), args, kwargs);
}

std::string PyFunction::to_string() const
{
	return fmt::format("<function {} at {}>", m_name->value(), (void *)this);
}

namespace {
	std::once_flag function_flag;
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyFunction::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(function_flag, []() {
			type = std::move(klass<PyFunction>("function")
					.attr("__code__", &PyFunction::m_code)
					.attr("__globals__", &PyFunction::m_globals)
					.attr("__dict__", &PyFunction::m_dict)
					.attr("__name__", &PyFunction::m_name)
					.attr("__qualname__", &PyFunction::m_qualname)
					.attr("__doc__", &PyFunction::m_doc)
					.property_readonly(
						"__closure__", [](PyFunction *self) { return Ok(self->m_closure); })
					.property(
						"__doc__",
						[](PyFunction *self) { return Ok(self->m_doc); },
						[](PyFunction *self, PyObject *d) {
							self->m_doc = d;
							return Ok(std::monostate{});
						})
					.property_readonly(
						"__globals__", [](PyFunction *self) { return Ok(self->m_globals); })
					.property(
						"__module__",
						[](PyFunction *self) { return Ok(self->m_module); },
						[](PyFunction *self, PyObject *m) {
							self->m_module = m;
							return Ok(std::monostate{});
						})
					.type);
		});
		return std::move(type);
	};
}

PyNativeFunction::PyNativeFunction(PyType *type) : PyBaseObject(type) {}

PyNativeFunction::PyNativeFunction(std::string &&name, FunctionType &&function)
	: PyBaseObject(types::BuiltinTypes::the().native_function()), m_name(std::move(name)),
	  m_function(std::move(function))
{}

std::string PyNativeFunction::to_string() const
{
	if (is_method()) {
		return fmt::format("<built-in method {} of {} object at {}>",
			m_name,
			m_self->type()->name(),
			(void *)this);
	} else {
		return fmt::format("<built-in function {} at {}>", m_name, (void *)this);
	}
}

// PyResult<PyObject *> PyNativeFunction::__call__(PyTuple *args, PyDict *kwargs)
// {

// 	ASSERT(RuntimeContext::has_current() && RuntimeContext::current().has_interpreter());
// 	auto *interpreter = RuntimeContext::current().interpreter();

// 	auto result = [this, args, kwargs, interpreter]() {
// 		if (is_method()) {
// 			ASSERT(m_self);
// 			return interpreter->call(this, m_self, args, kwargs);
// 		} else {
// 			return interpreter->call(this, args, kwargs);
// 		}
// 	}();
// 	return result;
// }

PyResult<PyObject *> PyNativeFunction::__call__(PyTuple *args, PyDict *kwargs)
{
	// 直接调用存储的函数指针，不依赖 Interpreter / VM
	// 原实现绕道 interpreter->call(this, args, kwargs)，
	// 但 interpreter->call 只做两件事：
	//   1. operator()(args, kwargs)       ← 实际工作
	//   2. vm.reg(0) = result             ← VM 簿记（字节码指令自己也做）
	// 第 2 步是冗余的，且阻止了 AOT 模式下的调用。
	if (is_function()) {
		return operator()(args, kwargs);
	} else {
		ASSERT(m_self);
		return operator()(m_self, args, kwargs);
	}
}

PyResult<PyObject *> PyNativeFunction::__repr__() const { return PyString::create(to_string()); }

// PyResult<PyObject *> PyNativeFunction::__get__(PyObject *instance, PyObject * /*owner*/) const
// {
// 	// Python 3.9 描述符协议:
// 	//   Class.method      → instance=None  → 返回未绑定函数
// 	//   obj.method         → instance=obj   → 返回绑定方法
// 	//
// 	// CPython 中 builtin_function_or_method 的 __get__ 返回 PyMethod (bound method)
// 	// 我们用 PyNativeFunction 闭包模拟同样的效果
// 	if (!instance || instance == py_none()) { return Ok(const_cast<PyNativeFunction *>(this)); }

// 	// 创建绑定方法: 将 instance 前置到参数列表
// 	auto *self_func = const_cast<PyNativeFunction *>(this);
// 	return PyNativeFunction::create(
// 		m_name,
// 		[self_func, instance](PyTuple *args, PyDict *kwargs) -> PyResult<PyObject *> {
// 			std::vector<Value> new_args;
// 			new_args.reserve((args ? args->size() : 0) + 1);
// 			new_args.push_back(instance);
// 			if (args) {
// 				new_args.insert(new_args.end(), args->elements().begin(), args->elements().end());
// 			}
// 			auto bound_args = PyTuple::create(new_args);
// 			if (bound_args.is_err()) { return Err(bound_args.unwrap_err()); }
// 			return self_func->__call__(bound_args.unwrap(), kwargs);
// 		},
// 		self_func,// GC: 追踪原函数
// 		instance// GC: 追踪绑定实例
// 	);
// }

PyResult<PyObject *> PyNativeFunction::__get__(PyObject *instance, PyObject * /*owner*/) const
{
	// Python 3.9 描述符协议:
	//   Class.method      → instance=None  → 返回未绑定函数
	//   obj.method         → instance=obj   → 返回绑定方法
	if (!instance || instance == py_none()) { return Ok(const_cast<PyNativeFunction *>(this)); }

	// [修复极度致命的性能结节]：
	// 不要去创建新的 PyNativeFunction + std::function(malloc) 闭包！
	// std::function 内部状态分配极慢，且阻碍了 GC。
	// 直接生成轻巧的 PyBoundMethod 进行实例绑定，立减亿级堆分配！
	return PyBoundMethod::create(instance, const_cast<PyNativeFunction *>(this));
}

void PyNativeFunction::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	if (m_self) { visitor.visit(*m_self); }
	if (m_closure) { visitor.visit(*m_closure); }
	if (m_module_ref) { visitor.visit(*m_module_ref); }
	for (auto *obj : m_captures) { visitor.visit(*obj); }
}

PyType *PyNativeFunction::static_type() const { return types::native_function(); }

namespace {
	std::once_flag native_function_flag;

	std::unique_ptr<TypePrototype> register_native_function()
	{
		return std::move(klass<PyNativeFunction>("builtin_function_or_method").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyNativeFunction::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(native_function_flag, []() { type = register_native_function(); });
		return std::move(type);
	};
}

template<> PyFunction *as(PyObject *node)
{
	if (node->type() == types::function()) { return static_cast<PyFunction *>(node); }
	return nullptr;
}

template<> const PyFunction *as(const PyObject *node)
{
	if (node->type() == types::function()) { return static_cast<const PyFunction *>(node); }
	return nullptr;
}

template<> PyNativeFunction *as(PyObject *node)
{
	if (node->type() == types::native_function()) { return static_cast<PyNativeFunction *>(node); }
	return nullptr;
}

template<> const PyNativeFunction *as(const PyObject *node)
{
	if (node->type() == types::native_function()) {
		return static_cast<const PyNativeFunction *>(node);
	}
	return nullptr;
}

}// namespace py
