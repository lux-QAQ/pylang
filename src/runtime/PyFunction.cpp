#include "PyFunction.hpp"
#include "PyBoundMethod.hpp"
#include "PyCode.hpp"
#include "PyDict.hpp"
#include "PyNone.hpp"
#include "PyString.hpp"

#include "runtime/RuntimeError.hpp"
#include "executable/Program.hpp"
#include "interpreter/InterpreterCore.hpp"
#include "runtime/PyObject.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


#include <variant>

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

/// AOT 编译后的函数指针类型（无闭包）
using AOTFuncPtr = py::PyObject *(*)(py::PyTuple *, py::PyDict *);

/// AOT 编译后的函数指针类型（有闭包）
using AOTClosureFuncPtr = py::PyObject *(*)(py::PyObject *, py::PyTuple *, py::PyDict *);

PyResult<PyNativeFunction *> PyNativeFunction::create_aot(std::string name,
	void *code_ptr,
	PyObject *module,
	PyObject *defaults,
	PyObject *kwdefaults,
	PyObject *closure)
{
	PyNativeFunction *result = nullptr;

	if (closure) {
		//  闭包函数 
		// 编译后签名: PyObject*(PyObject* closure, PyTuple* args, PyDict* kwargs)
		// 通过 lambda 捕获 closure 指针，保持 FreeFunctionType（不用 MethodType hack）
		auto fn = reinterpret_cast<AOTClosureFuncPtr>(code_ptr);
		PyObject *captured_closure = closure;

		FreeFunctionType func = [fn, captured_closure](
									PyTuple *args, PyDict *kwargs) -> PyResult<PyObject *> {
			auto *r = fn(captured_closure, args, kwargs);
			if (!r) { return Err(runtime_error("compiled closure returned null")); }
			return Ok(r);
		};

		result = PYLANG_ALLOC(
			PyNativeFunction, std::move(name), FunctionType{ std::move(func) }, nullptr);
	} else {
		//  普通函数 
		// 编译后签名: PyObject*(PyTuple* args, PyDict* kwargs)
		auto fn = reinterpret_cast<AOTFuncPtr>(code_ptr);

		FreeFunctionType func = [fn](PyTuple *args, PyDict *kwargs) -> PyResult<PyObject *> {
			auto *r = fn(args, kwargs);
			if (!r) { return Err(runtime_error("compiled function returned null")); }
			return Ok(r);
		};

		result = PYLANG_ALLOC(
			PyNativeFunction, std::move(name), FunctionType{ std::move(func) }, nullptr);
	}

	if (!result) { return Err(memory_error(sizeof(PyNativeFunction))); }

	// 存储元数据（Python 语义需要）
	result->m_closure = closure ? static_cast<PyTuple *>(closure) : nullptr;
	result->m_module_ref = module;

	// GC 追踪：lambda 捕获了这些指针，必须让 GC 知道
	// 在AOT模式下是多余的,考虑删除
	if (closure) { result->m_captures.push_back(closure); }
	if (defaults) { result->m_captures.push_back(defaults); }
	if (kwdefaults) { result->m_captures.push_back(kwdefaults); }
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
	return m_code->eval(m_globals,
		ns,
		args,
		kwargs,
		m_defaults,
		m_kwonly_defaults,
		m_closure ? m_closure->elements() : std::vector<Value>{},
		m_name);
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
