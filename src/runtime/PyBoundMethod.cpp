#include "PyBoundMethod.hpp"
#include "PyFunction.hpp"
#include "PyString.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"

#include "runtime/compat.hpp"

namespace py {

PyBoundMethod::PyBoundMethod(PyType *type) : PyBaseObject(type) {}

// [修改]：兼容 PyObject
PyBoundMethod::PyBoundMethod(PyObject *self, PyObject *method)
	: PyBaseObject(types::BuiltinTypes::the().bound_method()), m_self(self), m_method(method)
{}

PyResult<PyBoundMethod *> PyBoundMethod::create(PyObject *self, PyObject *method)
{
	auto *result = PYLANG_ALLOC(PyBoundMethod, self, method);
	if (!result) { return Err(memory_error(sizeof(PyBoundMethod))); }
	return Ok(result);
}

void PyBoundMethod::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	visitor.visit(*m_self);
	visitor.visit(*m_method);
}

std::string PyBoundMethod::to_string() const
{
	auto qualname_str = PyString::create("__qualname__");
	if (qualname_str.is_err()) { TODO(); }
	auto self_qualname = m_self->getattribute(qualname_str.unwrap());
	if (self_qualname.is_err()) {
		// [容错]: Native Function 可能没有 __qualname__
		return fmt::format("<bound method of '{}'>", m_method->to_string());
	}
	return fmt::format(
		"<bound method '{}' of '{}'>", m_method->to_string(), self_qualname.unwrap()->to_string());
}

PyResult<PyObject *> PyBoundMethod::__repr__() const { return PyString::create(to_string()); }

PyResult<PyObject *> PyBoundMethod::__call__(PyTuple *args, PyDict *kwargs)
{
	// first create new args tuple -> (self, *args)
	std::vector<Value> new_args_vector;
	new_args_vector.reserve(args->size() + 1);
	new_args_vector.push_back(m_self);
	for (const auto &arg : args->elements()) { new_args_vector.push_back(arg); }
	auto args_ = PyTuple::create(new_args_vector);
	if (args_.is_err()) { return args_; }
	return m_method->call(args_.unwrap(), kwargs);
}

// PyResult<PyObject *> PyBoundMethod::call_raw(std::span<Value> args, PyDict *kwargs)
// {
//     // 在栈上准备空间：[self, arg0, arg1, ...]
//     size_t total_argc = args.size() + 1;
//     Value* stack_args = static_cast<Value*>(alloca(sizeof(Value) * total_argc));

//     new (&stack_args[0]) Value(m_self);
//     for (size_t i = 0; i < args.size(); ++i) {
//         new (&stack_args[i + 1]) Value(args[i]);
//     }

//     // 穿透调用底层函数的 call_raw
//     auto result = m_method->call_raw(std::span<Value>(stack_args, total_argc), kwargs);

//     // 析构 Value (使用 std::destroy_at 避免 typedef 解析错误)
//     for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&stack_args[i]);
//     return result;
// }

PyResult<PyObject *> PyBoundMethod::call_raw(std::span<const Value> args, PyDict *kwargs)
{
	// 在栈上准备空间：[self, arg0, arg1, ...]
	size_t total_argc = args.size() + 1;
	Value *stack_args = static_cast<Value *>(alloca(sizeof(Value) * total_argc));

	new (&stack_args[0]) Value(m_self);
	for (size_t i = 0; i < args.size(); ++i) { new (&stack_args[i + 1]) Value(args[i]); }

	// 穿透调用底层函数的 call_raw
	auto result = m_method->call_raw(std::span<const Value>(stack_args, total_argc), kwargs);

	for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&stack_args[i]);
	return result;
}

/*
PyType *PyBoundMethod::static_type() const { return types::bound_method(); }
*/

namespace {

	std::once_flag bound_method_flag;

	std::unique_ptr<TypePrototype> register_bound_method()
	{
		return std::move(klass<PyBoundMethod>("bound_method").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyBoundMethod::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(bound_method_flag, []() { type = register_bound_method(); });
		return std::move(type);
	};
}

template<> PyBoundMethod *as(PyObject *node)
{
	if (node->type() == types::bound_method()) { return static_cast<PyBoundMethod *>(node); }
	return nullptr;
}

template<> const PyBoundMethod *as(const PyObject *node)
{
	if (node->type() == types::bound_method()) { return static_cast<const PyBoundMethod *>(node); }
	return nullptr;
}

}// namespace py
