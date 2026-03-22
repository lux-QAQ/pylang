#include "PySlotWrapper.hpp"
#include "PyFunction.hpp"
#include "PyString.hpp"
#include "PyType.hpp"
#include "TypeError.hpp"
#include "interpreter/InterpreterCore.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"

#include "runtime/compat.hpp"

namespace py {

PySlotWrapper::PySlotWrapper(PyType *type) : PyBaseObject(type) {}

std::string PySlotWrapper::to_string() const
{
	return fmt::format("<slot wrapper '{}' of '{}' objects>", m_name->to_string(), m_type->name());
}

void PySlotWrapper::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	visitor.visit(*m_name);
	visitor.visit(*m_type);
}

PyResult<PyObject *> PySlotWrapper::__repr__() const { return PyString::create(to_string()); }

// PyResult<PyObject *> PySlotWrapper::__call__(PyTuple *args, PyDict *kwargs)
// {
// 	// split args tuple -> (args[0], args[1:])
// 	// since args[0] is self (right?)
// 	std::vector<Value> new_args_vector;
// 	ASSERT(args->size() > 0);
// 	new_args_vector.reserve(args->size() - 1);
// 	auto self_ = PyObject::from(args->elements()[0]);
// 	if (self_.is_err()) return self_;
// 	auto *self = self_.unwrap();
// 	for (size_t i = 1; i < args->size(); ++i) { new_args_vector.push_back(args->elements()[i]); }
// 	auto args_ = PyTuple::create(new_args_vector);
// 	if (args_.is_err()) return args_;
// 	args = args_.unwrap();

// 	return m_slot(self, args, kwargs);
// }

PyResult<PyObject *> PySlotWrapper::__call__(PyTuple *args, PyDict *kwargs)
{
    ASSERT(args->size() > 0);
    // [修复]：Value 是 std::variant，需通过 PyObject::from 获取 PyObject*
    auto self_res = PyObject::from(args->elements()[0]);
    if (self_res.is_err()) return self_res;
    auto *self = self_res.unwrap();

    // 1. 无参优化：如果是 Node() 调用（仅含 self），复用空元组单例
    if (args->size() == 1) {
        return m_slot(self, PyTuple::create().unwrap(), kwargs);
    }

    // 2. 切片优化：使用 GCVector 触发移动语义，避免 O(N) 内存拷贝
    py::GCVector<Value> new_args_vector;
    new_args_vector.reserve(args->size() - 1);
    for (size_t i = 1; i < args->size(); ++i) { 
        new_args_vector.push_back(args->elements()[i]); 
    }
    
    auto args_res = PyTuple::create(std::move(new_args_vector));
    if (args_res.is_err()) return args_res;
    
    return m_slot(self, args_res.unwrap(), kwargs);
}

// PyResult<PyObject *> PySlotWrapper::call_raw(std::span<const Value> args, PyDict *kwargs)
// {
//     if (args.empty()) return Err(type_error("slot wrapper requires self"));
//     // [修复]：同上
//     auto self_res = PyObject::from(args[0]);
//     if (self_res.is_err()) return self_res;
//     auto *self = self_res.unwrap();

//     if (args.size() == 1) { return m_slot(self, PyTuple::create().unwrap(), kwargs); }

//     py::GCVector<Value> slice_args(args.begin() + 1, args.end());
//     return PyTuple::create(std::move(slice_args))
//         .and_then([this, self, kwargs](PyTuple *t) -> PyResult<PyObject *> {
//             return m_slot(self, t, kwargs);
//         });
// }

PyResult<PyObject *> PySlotWrapper::call_raw(std::span<const Value> args, PyDict *kwargs)
{
    if (args.empty()) return Err(type_error("slot wrapper requires self"));
    
    auto self_res = PyObject::from(args[0]);
    if (self_res.is_err()) return self_res;
    auto *self = self_res.unwrap();

    // [核心优化]：拦截 Node() 这种无参调用
    if (args.size() == 1 && (!kwargs || kwargs->map().empty())) {
        // PyTuple::create() 内部已经实现了 get_empty_tuple_singleton()
        return m_slot(self, PyTuple::create().unwrap(), kwargs);
    }

    // 仅在确实有参数时，才进入昂贵的 GCVector 打包流程
    py::GCVector<Value> slice_args;
    slice_args.reserve(args.size() - 1);
    for (size_t i = 1; i < args.size(); ++i) { slice_args.push_back(args[i]); }

    return PyTuple::create(std::move(slice_args))
        .and_then([this, self, kwargs](PyTuple *t) -> PyResult<PyObject *> {
            return m_slot(self, t, kwargs);
        });
}

PyResult<PyObject *> PySlotWrapper::__get__(PyObject *instance, PyObject * /*owner*/) const
{
	if (!instance) { return Ok(const_cast<PySlotWrapper *>(this)); }
	if (!instance->type()->issubclass(m_type)) {
		return Err(
			type_error("descriptor '{}' for '{}' objects "
					   "doesn't apply to a '{}' object",
				m_name->value(),
				m_type->underlying_type().__name__,
				instance->type()->underlying_type().__name__));
	}
	// bind slot wrapper to the instance
	return PyNativeFunction::create(
		m_name->value(),
		[this, instance](PyTuple *args, PyDict *kwargs) {
			std::vector<Value> args_;
			args_.reserve(args->size() + 1);
			args_.push_back(instance);
			args_.insert(args_.end(), args->elements().begin(), args->elements().end());
			return PyTuple::create(args_).and_then([this, kwargs](auto *args) {
				return const_cast<PySlotWrapper *>(this)->__call__(args, kwargs);
			});
		},
		const_cast<PySlotWrapper *>(this),
		instance);
}

PySlotWrapper::PySlotWrapper(PyString *name, PyType *slot_type, Slot &base, FunctionType &&function)
	: PyBaseObject(types::BuiltinTypes::the().slot_wrapper()), m_name(std::move(name)),
	  m_type(slot_type), m_base(base), m_slot(std::move(function))
{}

PyResult<PySlotWrapper *>
	PySlotWrapper::create(PyString *name, PyType *slot_type, Slot &base, FunctionType &&function)
{
	ASSERT(name);
	ASSERT(slot_type);
	auto *obj = PYLANG_ALLOC(PySlotWrapper, name, slot_type, base, std::move(function));
	if (!obj) return Err(memory_error(sizeof(PySlotWrapper)));
	return Ok(obj);
}

PyType *PySlotWrapper::static_type() const { return types::slot_wrapper(); }

namespace {

	std::once_flag slot_wrapper_flag;

	std::unique_ptr<TypePrototype> register_slot_wrapper()
	{
		return std::move(klass<PySlotWrapper>("slot_wrapper").type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PySlotWrapper::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(slot_wrapper_flag, []() { type = register_slot_wrapper(); });
		return std::move(type);
	};
}

template<> PySlotWrapper *as(PyObject *obj)
{
	if (obj->type() == types::slot_wrapper()) { return static_cast<PySlotWrapper *>(obj); }
	return nullptr;
}

template<> const PySlotWrapper *as(const PyObject *obj)
{
	if (obj->type() == types::slot_wrapper()) { return static_cast<const PySlotWrapper *>(obj); }
	return nullptr;
}
}// namespace py
