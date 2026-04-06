#include "PyObject.hpp"
#include "shape/Shape.hpp"

#include "AttributeError.hpp"
#include "NotImplemented.hpp"
#include "NotImplementedError.hpp"
#include "PyBool.hpp"
#include "PyBytes.hpp"
#include "PyDict.hpp"
#include "PyEllipsis.hpp"
#include "PyFloat.hpp"
#include "PyFunction.hpp"
#include "PyGenericAlias.hpp"
#include "PyInteger.hpp"
#include "PyIterator.hpp"
#include "PyNone.hpp"
#include "PyNumber.hpp"
#include "PySlotWrapper.hpp"
#include "PyStaticMethod.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"
#include "PyType.hpp"
#include "StopIteration.hpp"
#include "TypeError.hpp"
#include "Value.hpp"
#include "taggered_pointer/RtValue.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"


#include <variant>

namespace py {


bool descriptor_is_data(const PyObject *obj)
{
	// FIXME: temporary hack to get object.__new__ working, but requires __set__ to be
	// implemented
	//        should be:
	//        obj->type()->underlying_type().__set__.has_value()
	return !as<PyStaticMethod>(obj) && !as<PySlotWrapper>(obj);
}

namespace {
	bool is_method_descriptor(PyType *obj_type)
	{
		return obj_type == types::method_wrapper() || obj_type == types::function()
			   || obj_type == types::classmethod_descriptor();
	}


	// 辅助函数：当 Slot 缺失时，尝试动态查找 Python 方法
	PyResult<PyObject *>
		binary_op_fallback(const PyObject *lhs, const PyObject *rhs, const char *op_name)
	{
		auto name = PyString::create(op_name);
		if (name.is_err()) return Err(name.unwrap_err());

		// 查找属性 (getattr(lhs, "__add__"))，这会自动处理方法绑定
		auto [method, found] = lhs->lookup_attribute(name.unwrap());

		if (found == LookupAttrResult::FOUND && method.is_ok()) {
			// 构造参数 tuple: (rhs,)
			// 使用变参模板版本 PyTuple::create(Args...)，避免 vector 构造歧义
			auto args = PyTuple::create(const_cast<PyObject *>(rhs));
			if (args.is_err()) return Err(args.unwrap_err());

			return method.unwrap()->call(args.unwrap(), nullptr);
		}

		return Ok(not_implemented());
	}
}// namespace

void TypePrototype::visit_graph(::Cell::Visitor &visitor)
{
	if (__class__) { visitor.visit(*__class__); }
	if (__dict__) { visitor.visit(*__dict__); }
	if (__base__) { visitor.visit(*__base__); }
	for (auto *b : __bases__) { visitor.visit(*b); }
}

// size_t RtValueHash::operator()(const RtValue &value) const
// {
//     auto val = value.hash();
//     ASSERT(val.is_ok());
//     return val.unwrap();
// }

// bool RtValueEq::operator()(const RtValue &lhs, const RtValue &rhs) const
// {
//     auto result = equals(lhs, rhs).and_then([](const auto &result) {
//         return truthy(result);
//     });
//     if (result.is_err()) { return false; }
//     return result.unwrap();
// }

namespace {
	inline bool may_break_reflexive_equality(const PyObject *obj) noexcept
	{
		PyType *type = obj->type();
		return type == types::float_() || type == types::complex();
	}

	template<typename T> auto to_object(T &&value)
	{
		if constexpr (std::is_base_of_v<PyObject,
						  typename std::remove_pointer_t<typename std::remove_cvref_t<T>>>) {
			return value;
		} else {
			return PyObject::from(value).unwrap();
		}
	}

	template<typename SlotFunctionType,
		typename ResultType = typename SlotFunctionType::result_type>
	ResultType call_slot(const std::variant<SlotFunctionType, PyObject *> &slot,
		PyObject *self,
		PyTuple *args,
		PyDict *kwargs)
		requires std::is_same_v<typename ResultType::OkType, PyObject *>
	{
		if (std::holds_alternative<SlotFunctionType>(slot)) {
			return std::get<SlotFunctionType>(slot)(self, args, kwargs);
		} else if (std::holds_alternative<PyObject *>(slot)) {
			std::vector<Value> args_;
			args_.reserve(args->size() + 1);
			args_.push_back(self);
			args_.insert(args_.end(), args->elements().begin(), args->elements().end());
			return PyTuple::create(args_).and_then([&slot, kwargs](PyTuple *args) -> ResultType {
				return std::get<PyObject *>(slot)->call(args, kwargs);
			});
		} else {
			TODO();
		}
	}

	template<typename SlotFunctionType,
		typename ResultType = typename SlotFunctionType::result_type,
		typename... Args>
	ResultType call_slot(const std::variant<SlotFunctionType, PyObject *> &slot, Args &&...args_)
		requires std::is_same_v<typename ResultType::OkType, PyObject *>
	{
		if (std::holds_alternative<SlotFunctionType>(slot)) {
			return std::get<SlotFunctionType>(slot)(std::forward<Args>(args_)...);
		} else if (std::holds_alternative<PyObject *>(slot)) {
			// FIXME: this const_cast is needed since in Python land there is no concept of
			//		  PyObject constness (right?). But for the internal calls handled above
			//		  which are resolved in the C++ runtime, we want to enforce constness
			//		  so we end up with the awkward line below. But how could we do better?
			auto args = PyTuple::create(const_cast<PyObject *>(
				static_cast<const PyObject *>(to_object(std::forward<Args>(args_))))...);
			if (args.is_err()) { return Err(args.unwrap_err()); }
			PyDict *kwargs = nullptr;
			return std::get<PyObject *>(slot)->call(args.unwrap(), kwargs);
		} else {
			TODO();
		}
	}

	template<typename SlotFunctionType,
		typename ResultType = typename SlotFunctionType::result_type,
		typename... Args>
	ResultType call_slot(const std::variant<SlotFunctionType, PyObject *> &slot,
		std::string_view conversion_error_message,
		Args &&...args_)
		requires(!std::is_same_v<typename ResultType::OkType, PyObject *>)
	{
		if (std::holds_alternative<SlotFunctionType>(slot)) {
			return std::get<SlotFunctionType>(slot)(std::forward<Args>(args_)...);
		} else if (std::holds_alternative<PyObject *>(slot)) {
			auto *callable = std::get<PyObject *>(slot);
			ASSERT(callable);
			// FIXME: this const_cast is needed since in Python land there is no concept of
			//		  PyObject constness (right?). But for the internal calls handled above
			//		  which are resolved in the C++ runtime, we want to enforce constness
			//		  so we end up with the awkward line below. But how could we do better?
			auto args = PyTuple::create(const_cast<PyObject *>(
				static_cast<const PyObject *>(to_object(std::forward<Args>(args_))))...);
			if (args.is_err()) { return Err(args.unwrap_err()); }
			PyDict *kwargs = nullptr;
			if constexpr (std::is_same_v<typename ResultType::OkType, bool>) {
				auto result = callable->call(args.unwrap(), kwargs);
				if (result.is_err()) return Err(result.unwrap_err());
				if (!as<PyBool>(result.unwrap())) {
					return Err(type_error(std::string(conversion_error_message)));
				}
				return Ok(as<PyBool>(result.unwrap())->value());
			} else if constexpr (std::is_integral_v<typename ResultType::OkType>) {
				auto result = callable->call(args.unwrap(), kwargs);
				if (result.is_err()) return Err(result.unwrap_err());
				if (!as<PyInteger>(result.unwrap())) {
					return Err(type_error(std::string(conversion_error_message)));
				}
				// return Ok(as<PyInteger>(result.unwrap())->as_i64());
				return Ok(static_cast<int32_t>(as<PyInteger>(result.unwrap())->as_i64()));
			} else if constexpr (std::is_same_v<typename ResultType::OkType, std::monostate>) {
				auto result = callable->call(args.unwrap(), kwargs);
				if (result.is_err()) {
					return Err(result.unwrap_err());
				} else {
					return Ok(std::monostate{});
				}
			} else {
				[]<bool flag = false>() { static_assert(flag, "unsupported return type"); }();
			}
		} else {
			TODO();
		}
	}
}// namespace


PyResult<size_t> PyMappingWrapper::len()
{
	ASSERT(m_object->type_prototype().mapping_type_protocol.has_value());
	if (m_object->type_prototype().mapping_type_protocol->__len__.has_value()) {
		return call_slot(*m_object->type_prototype().mapping_type_protocol->__len__,
			"object cannot be interpreted as an integer",
			m_object);
	}

	return Err(type_error("object of type '{}' has no len()", m_object->type()->name()));
}

PyResult<PyObject *> PyMappingWrapper::getitem(PyObject *name)
{
	ASSERT(m_object->type_prototype().mapping_type_protocol.has_value());
	if (m_object->type_prototype().mapping_type_protocol->__getitem__.has_value()) {
		return call_slot(
			*m_object->type_prototype().mapping_type_protocol->__getitem__, m_object, name);
	}

	return Err(type_error("object of type '{}' is not subscriptable", m_object->type()->name()));
}

PyResult<std::monostate> PyMappingWrapper::setitem(PyObject *name, PyObject *value)
{
	ASSERT(m_object->type_prototype().mapping_type_protocol.has_value());
	if (m_object->type_prototype().mapping_type_protocol->__setitem__.has_value()) {
		return call_slot(*m_object->type_prototype().mapping_type_protocol->__setitem__,
			"",
			m_object,
			name,
			value);
	}

	return Err(type_error(
		"object of type '{}' does not support item assignment", m_object->type()->name()));
}

PyResult<std::monostate> PyMappingWrapper::delitem(PyObject *name)
{
	if (!m_object->type_prototype().mapping_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().mapping_type_protocol->__delitem__.has_value()) {
		return call_slot(
			*m_object->type_prototype().mapping_type_protocol->__delitem__, "", m_object, name);
	}

	return Err(
		type_error("object of type '{}' does not support item deletion", m_object->type()->name()));
}

PyResult<size_t> PySequenceWrapper::len()
{
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__len__.has_value()) {
		return call_slot(*m_object->type_prototype().sequence_type_protocol->__len__,
			"object cannot be interpreted as an integer",
			m_object);
	}

	return Err(type_error("object of type '{}' has no len()", m_object->type()->name()));
}

PyResult<bool> PySequenceWrapper::contains(PyObject *value)
{
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__contains__.has_value()) {
		return call_slot(*m_object->type_prototype().sequence_type_protocol->__contains__,
			"object cannot be interpreted as a bool",
			m_object,
			value);
	}

	return Err(type_error("object of type '{}' has no contains()", m_object->type()->name()));
}

PyResult<PyObject *> PySequenceWrapper::repeat(const PyObject *value)
{
	if (!value->type()->issubclass(types::integer())) {
		return Err(
			type_error("can't multiply sequence by non-int of type '{}'", value->type()->name()));
	}
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__repeat__.has_value()) {
		return call_slot(*m_object->type_prototype().sequence_type_protocol->__repeat__,
			m_object,
			static_cast<const PyInteger &>(*value).as_size_t());
	}

	return Err(type_error("object of type '{}' has no repeat()", m_object->type()->name()));
}


PyResult<PyObject *> PySequenceWrapper::getitem(int64_t index)
{
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__getitem__.has_value()) {
		return call_slot(
			*m_object->type_prototype().sequence_type_protocol->__getitem__, m_object, index);
	}

	return Err(
		type_error("object of type '{}' does not support indexing", m_object->type()->name()));
}

PyResult<std::monostate> PySequenceWrapper::setitem(int64_t index, PyObject *value)
{
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__setitem__.has_value()) {
		return call_slot(*m_object->type_prototype().sequence_type_protocol->__setitem__,
			"",
			m_object,
			index,
			value);
	}

	return Err(type_error(
		"object of type '{}' does not support item assignment", m_object->type()->name()));
}

PyResult<std::monostate> PySequenceWrapper::delitem(int64_t index)
{
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__delitem__.has_value()) {
		return call_slot(
			*m_object->type_prototype().sequence_type_protocol->__delitem__, "", m_object, index);
	}

	return Err(
		type_error("object of type '{}' does not support item deletion", m_object->type()->name()));
}

PyResult<PyObject *> PySequenceWrapper::concat(PyObject *value)
{
	if (!m_object->type_prototype().sequence_type_protocol.has_value()) { TODO(); }
	if (m_object->type_prototype().sequence_type_protocol->__concat__.has_value()) {
		return call_slot(
			*m_object->type_prototype().sequence_type_protocol->__concat__, m_object, value);
	}

	return Err(type_error("unsupported operand type(s) for +: \'{}\' and \'{}\'",
		m_object->type()->name(),
		value->type()->name()));
}

template<> PyResult<PyObject *> PyObject::from(PyObject *const &value)
{
	ASSERT(value);
	return Ok(value);
}

// template<> PyResult<PyObject *> PyObject::from(const Number &value)
// {
// 	return PyNumber::create(value);
// }


template<> PyResult<PyObject *> PyObject::from(const int64_t &value)
{
	return PyInteger::create(value);
}

template<> PyResult<PyObject *> PyObject::from(const size_t &value)
{
	return PyInteger::create(static_cast<int64_t>(value));
}

template<> PyResult<PyObject *> PyObject::from(const Value &value) { return Ok(value.box()); }

PyObject::PyObject(const TypePrototype &type) : Cell()
{
	m_bits_type = types::lookup_type_by_prototype(&type);
	if (!m_bits_type) {
		spdlog::error("bootstrap type missing: {}", type.__name__);
		ASSERT(m_bits_type && "PyType must be initialized before object creation.");
	}
}

// 实现 vector 装箱
PyResult<PyObject *> PyObject::from(const std::vector<PyObject *> &value)
{
	auto tuple = PyTuple::create(value);
	if (tuple.is_err()) return Err(tuple.unwrap_err());
	return Ok(tuple.unwrap());
}

PyObject::PyObject(PyType *type) : Cell(), m_bits_type(type) { /* ASSERT(type); */ }

// const TypePrototype &PyObject::type_prototype() const { return type()->underlying_type(); }

const TypePrototype &PyObject::type_prototype() const { return m_bits_type->underlying_type(); }


// PyBaseObject::PyBaseObject(const TypePrototype &type) : PyObject(type) {}


// void PyObject::visit_graph(Visitor &visitor)
// {
//
// 	for (size_t i = 0, offset = type()->underlying_type().basicsize; i < type()->__slots__.size();
// 		++i, offset += sizeof(PyObject *)) {
// 		auto *slot = *bit_cast<PyObject **>(bit_cast<uint8_t *>(this) + offset);
// 		if (slot) { visitor.visit(*slot); }
// 	}
// 	if (std::holds_alternative<PyType *>(m_type)) {
// 		if (auto *t = std::get<PyType *>(m_type)) { visitor.visit(*t); }
// 	} else {
// 		const_cast<TypePrototype &>(
// 			std::get<std::reference_wrapper<const TypePrototype>>(m_type).get())
// 			.visit_graph(visitor);
// 	}
// }

void PyObject::visit_graph(Visitor &visitor)
{
	// 1. 访问属性字典

	// 2. 访问类型对象本身 (这是对象生存的核心)
	if (m_bits_type) {
		visitor.visit(*m_bits_type);

		// 3. 访问 __slots__ 区域 (根据类型定义的 basicsize 偏移量扫描)
		const size_t basicsize = m_bits_type->underlying_type().basicsize;
		const auto &slots = m_bits_type->__slots__;

		if (!slots.empty()) {
			char *raw_ptr = reinterpret_cast<char *>(this);
			for (size_t i = 0; i < slots.size(); ++i) {
				// 指针在内存中的位置 = 对象起始地址 + basicsize + (索引 * 指针大小)
				PyObject **slot_addr =
					reinterpret_cast<PyObject **>(raw_ptr + basicsize + (i * sizeof(PyObject *)));
				if (*slot_addr) { visitor.visit(**slot_addr); }
			}
		}
	}
}

PyResult<PyMappingWrapper> PyObject::as_mapping()
{
	if (type_prototype().mapping_type_protocol.has_value()) {
		return Ok(PyMappingWrapper(this));
	} else {
		return Err(type_error("object of type {} is not mappable", type_prototype().__name__));
	}
}

PyResult<PySequenceWrapper> PyObject::as_sequence()
{
	if (type_prototype().sequence_type_protocol.has_value()) {
		return Ok(PySequenceWrapper(this));
	} else {
		return Err(type_error("object of type {} is not a sequence", type_prototype().__name__));
	}
}

PyResult<PyBufferProcs> PyObject::as_buffer()
{
	if (type_prototype().as_buffer.has_value()) {
		return Ok(*type_prototype().as_buffer);
	} else {
		return Err(
			type_error("a bytes-like object is required, not '{}'", type_prototype().__name__));
	}
}

PyResult<std::monostate> PyObject::get_buffer(PyBuffer &buffer, int flags)
{
	return as_buffer().and_then(
		[&buffer, flags, this](const PyBufferProcs &bc) -> PyResult<std::monostate> {
			(void)buffer;
			(void)flags;
			(void)this;
			(void)bc;
			return Err(not_implemented_error("get_buffer not implemented!"));
			// return bc.getbuffer(this, buffer, flags);
		});
}

PyResult<PyObject *> PyObject::__repr__() const
{
	return PyString::create(fmt::format(
		"<{} object at {}>", type_prototype().__name__, static_cast<const void *>(this)));
}

PyResult<PyObject *> PyObject::__str__()
{
	if (!type()->underlying_type().__repr__.has_value()) { return PyObject::__repr__(); }
	return repr();
}

PyResult<PyObject *> PyObject::richcompare(const PyObject *other, RichCompare op) const
{
	constexpr std::array opstr{ "<", "<=", "==", "!=", ">", ">=" };

	switch (op) {
	case RichCompare::Py_EQ: {
		if (auto result = eq(other); result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		} else if (auto result = other->eq(this);
			result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		}
	} break;
	case RichCompare::Py_GE: {
		if (auto result = ge(other); result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		} else if (auto result = other->le(this);
			result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		}
	} break;
	case RichCompare::Py_GT: {
		if (auto result = gt(other); result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		} else if (auto result = other->lt(this);
			result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		}
	} break;
	case RichCompare::Py_LE: {
		if (auto result = le(other); result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		} else if (auto result = other->ge(this);
			result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		}
	} break;
	case RichCompare::Py_LT: {
		if (auto result = lt(other); result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		} else if (auto result = other->gt(this);
			result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		}
	} break;
	case RichCompare::Py_NE: {
		if (auto result = ne(other); result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		} else if (auto result = other->ne(this);
			result.is_ok() && (result.unwrap() != not_implemented())) {
			return result;
		}
	} break;
	}

	switch (op) {
	case RichCompare::Py_EQ: {
		return Ok(this == other ? py_true() : py_false());
	} break;
	case RichCompare::Py_NE: {
		return Ok(this != other ? py_true() : py_false());
	} break;
	default: {
		// op not supported
		return Err(type_error("'{}' not supported between instances of '{}' and '{}'",
			opstr[static_cast<size_t>(op)],
			type_prototype().__name__,
			other->type_prototype().__name__));
	}
	}
}

PyResult<PyObject *> PyObject::eq(const PyObject *other) const
{
	if (this == other && !may_break_reflexive_equality(this)) { return Ok(py_true()); }
	bool checked_reverse_op = false;
	if (type() != other->type() && other->type()->issubclass(type())
		&& other->type()->underlying_type().__eq__.has_value()) {
		checked_reverse_op = true;
		if (auto result = call_slot(*other->type()->underlying_type().__eq__, other, this);
			result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	if (type_prototype().__eq__.has_value()) {
		if (auto result = call_slot(*type_prototype().__eq__, this, other);
			result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	if (!checked_reverse_op && other->type()->underlying_type().__eq__.has_value()) {
		if (auto result = call_slot(*other->type()->underlying_type().__eq__, other, this);
			result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	if (this == other) { return Ok(py_true()); }
	return Ok(py_false());
}

PyResult<PyObject *> PyObject::ge(const PyObject *other) const
{
	if (type_prototype().__ge__.has_value()) {
		return call_slot(*type_prototype().__ge__, this, other);
	}
	return Ok(not_implemented());
}

PyResult<PyObject *> PyObject::gt(const PyObject *other) const
{
	if (type_prototype().__gt__.has_value()) {
		return call_slot(*type_prototype().__gt__, this, other);
	}
	return Ok(not_implemented());
}

PyResult<PyObject *> PyObject::le(const PyObject *other) const
{
	if (type_prototype().__le__.has_value()) {
		return call_slot(*type_prototype().__le__, this, other);
	}
	return Ok(not_implemented());
}

PyResult<PyObject *> PyObject::lt(const PyObject *other) const
{
	if (type_prototype().__lt__.has_value()) {
		return call_slot(*type_prototype().__lt__, this, other);
	}
	return Ok(not_implemented());
}

PyResult<PyObject *> PyObject::ne(const PyObject *other) const
{
	if (this == other && !may_break_reflexive_equality(this)) { return Ok(py_false()); }
	bool checked_reverse_op = false;
	if (type() != other->type() && other->type()->issubclass(type())
		&& other->type()->underlying_type().__ne__.has_value()) {
		checked_reverse_op = true;
		if (auto result = call_slot(*other->type()->underlying_type().__ne__, other, this);
			result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	if (type_prototype().__ne__.has_value()) {
		if (auto result = call_slot(*type_prototype().__ne__, this, other);
			result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	if (!checked_reverse_op && other->type()->underlying_type().__ne__.has_value()) {
		if (auto result = call_slot(*other->type()->underlying_type().__ne__, other, this);
			result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	if (this == other) { return Ok(py_false()); }
	return Ok(py_true());
}

PyResult<PyObject *> PyObject::getattribute(PyObject *attribute) const
{
	if (type_prototype().__getattribute__.has_value()) {
		return call_slot(*type_prototype().__getattribute__, this, attribute);
	}
	TODO();
}

PyResult<std::monostate> PyObject::setattribute(PyObject *attribute, PyObject *value)
{
	if (!as<PyString>(attribute)) {
		return Err(
			type_error("attribute name must be string, not '{}'", attribute->type()->to_string()));
	}

	if (auto setattr_method = type()->underlying_type().__setattribute__;
		setattr_method.has_value()) {
		return call_slot(*setattr_method, "", this, attribute, value);
	}
	if (type()->underlying_type().__getattribute__.has_value()) {
		return Err(type_error("'{}' object has only read-only attributes (assign to {})",
			type()->name(),
			attribute->to_string()));
	} else {
		return Err(type_error("'{}' object has no attributes (assign to {})",
			type()->name(),
			attribute->to_string()));
	}
}

PyResult<std::monostate> PyObject::delattribute(PyObject *attribute)
{
	if (!as<PyString>(attribute)) {
		return Err(
			type_error("attribute name must be string, not '{}'", attribute->type()->to_string()));
	}

	if (auto delattr_method = type()->underlying_type().__delattribute__;
		delattr_method.has_value()) {
		return call_slot(*delattr_method, "", this, attribute);
	}
	if (type()->underlying_type().__delattribute__.has_value()) {
		return Err(type_error("'{}' object has only read-only attributes (assign to {})",
			type()->name(),
			attribute->to_string()));
	} else {
		return Err(type_error("'{}' object has no attributes (assign to {})",
			type()->name(),
			attribute->to_string()));
	}
}

PyResult<PyString *> PyObject::repr() const
{
	if (type_prototype().__repr__.has_value()) {
		return call_slot(*type_prototype().__repr__, this)
			.and_then([](PyObject *str) -> PyResult<PyString *> {
				if (!as<PyString>(str)) {
					return Err(
						type_error("__repr__ returned non-string (type {})", str->type()->name()));
				}
				return Ok(as<PyString>(str));
			});
	} else {
		return PyObject::__repr__().and_then([](PyObject *obj) -> PyResult<PyString *> {
			ASSERT(as<PyString>(obj));
			return Ok(as<PyString>(obj));
		});
	}
}

PyResult<PyString *> PyObject::str()
{
	if (type_prototype().__str__.has_value()) {
		return call_slot(*type_prototype().__str__, this)
			.and_then([](PyObject *str) -> PyResult<PyString *> {
				if (!as<PyString>(str)) {
					return Err(
						type_error("__str__ returned non-string (type {})", str->type()->name()));
				}
				return Ok(as<PyString>(str));
			});
	}
	return repr();
}

PyResult<int64_t> PyObject::hash() const
{
	if (type_prototype().__hash__.has_value()) {
		return call_slot(
			*type_prototype().__hash__, "__hash__ method should return an integer", this);
	} else {
		return __hash__();
	}
}

// PyResult<PyObject *> PyObject::call(PyTuple *args, PyDict *kwargs)
// {
//     // 临时加入这一行防御与追踪：
//     spdlog::error("DEBUG: calling object type: {}", type()->name());

//     if (type_prototype().__call__.has_value()) {
//         return call_slot(*type_prototype().__call__, this, args, kwargs);
//     }
//     return Err(type_error("'{}' object is not callable", type_prototype().__name__));
// }

PyResult<PyObject *> PyObject::call(PyTuple *args, PyDict *kwargs)
{
	// [核心修复]：直接放行底层核心 Callable，绝对不允许它们绕回槽查表！
	// 否则 slot_wrapper 查 slot_wrapper 的 __call__ 会形成黑洞死循环
	if (auto *slot_wrapper = py::as<PySlotWrapper>(this)) {
		return slot_wrapper->__call__(args, kwargs);
	}
	if (auto *function = py::as<PyFunction>(this)) { return function->__call__(args, kwargs); }
	if (auto *native = py::as<PyNativeFunction>(this)) { return native->__call__(args, kwargs); }
	// [修复]：增加 PyType 的旁路放行。动态创建类型（或者类实例化）时，
	// PyType 没有走 Python 侧插槽，而是由 C++ 底层 call_raw 承接！
	if (auto *type_obj = py::as<PyType>(this)) {
		return type_obj->call_fast_ptrs(
			reinterpret_cast<PyObject **>(const_cast<py::Value *>(args->elements().data())),
			args->size(),
			kwargs);
	}

	if (type_prototype().__call__.has_value()) {
		return call_slot(*type_prototype().__call__, this, args, kwargs);
	}
	return Err(type_error("'{}' object is not callable", type_prototype().__name__));
}


PyResult<PyObject *> PyObject::add(const PyObject *other) const
{
	if (type_prototype().__add__.has_value()) {
		return call_slot(*type_prototype().__add__, this, other);
	} else if (other->type_prototype().__add__.has_value()) {
		return call_slot(*other->type_prototype().__add__, other, this);
	} else if (auto seq = const_cast<PyObject *>(this)->as_sequence(); seq.is_ok()) {
		return seq.unwrap().concat(const_cast<PyObject *>(other));
	}

	// Fallback: 动态查找 __add__
	auto result = binary_op_fallback(this, other, "__add__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for +: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::subtract(const PyObject *other) const
{
	if (type_prototype().__sub__.has_value()) {
		return call_slot(*type_prototype().__sub__, this, other);
	}

	// Fallback: 动态查找 __sub__
	auto result = binary_op_fallback(this, other, "__sub__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for -: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::multiply(const PyObject *other) const
{
	if (type_prototype().__mul__.has_value()) {
		auto result = call_slot(*type_prototype().__mul__, this, other);
		if (result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}
	// Fallback: 动态查找 __mul__
	{
		auto result = binary_op_fallback(this, other, "__mul__");
		if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }
	}

	if (other->type_prototype().__mul__.has_value()) {
		auto result = call_slot(*other->type_prototype().__mul__, other, this);
		if (result.is_err() || (result.is_ok() && result.unwrap() != not_implemented())) {
			return result;
		}
	}

	if (auto sequence = const_cast<PyObject *>(this)->as_sequence();
		sequence.is_ok() && type_prototype().sequence_type_protocol->__repeat__.has_value()) {
		return sequence.unwrap().repeat(other);
	}

	if (auto sequence = const_cast<PyObject *>(other)->as_sequence();
		sequence.is_ok()
		&& other->type_prototype().sequence_type_protocol->__repeat__.has_value()) {
		return sequence.unwrap().repeat(this);
	}

	return Err(type_error("unsupported operand type(s) for *: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::pow(const PyObject *other, const PyObject *modulo) const
{
	if (type_prototype().__pow__.has_value()) {
		return call_slot(*type_prototype().__pow__, this, other, modulo);
	}

	// Fallback: 动态查找 __pow__ (仅当 modulo 为 None 时尝试简单 fallback)
	if (modulo == py_none()) {
		auto result = binary_op_fallback(this, other, "__pow__");
		if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }
	}

	return Err(type_error("unsupported operand type(s) for **: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::truediv(PyObject *other)
{
	if (type_prototype().__truediv__.has_value()) {
		return call_slot(*type_prototype().__truediv__, this, other);
	}
	// Fallback: 动态查找 __truediv__
	auto result = binary_op_fallback(this, other, "__truediv__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for //: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::floordiv(PyObject *other)
{
	if (type_prototype().__floordiv__.has_value()) {
		return call_slot(*type_prototype().__floordiv__, this, other);
	}
	// Fallback: 动态查找 __floordiv__
	auto result = binary_op_fallback(this, other, "__floordiv__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for //: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::lshift(const PyObject *other) const
{
	if (type_prototype().__lshift__.has_value()) {
		return call_slot(*type_prototype().__lshift__, this, other);
	}
	// Fallback: 动态查找 __lshift__
	auto result = binary_op_fallback(this, other, "__lshift__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for <<: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::rshift(const PyObject *other) const
{
	if (type_prototype().__rshift__.has_value()) {
		return call_slot(*type_prototype().__rshift__, this, other);
	}
	// Fallback: 动态查找 __rshift__
	auto result = binary_op_fallback(this, other, "__rshift__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for >>: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::modulo(const PyObject *other) const
{
	if (type_prototype().__mod__.has_value()) {
		return call_slot(*type_prototype().__mod__, this, other);
	}
	// Fallback: 动态查找 __mod__
	auto result = binary_op_fallback(this, other, "__mod__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for %: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::divmod(PyObject *other)
{
	if (type_prototype().__divmod__.has_value()) {
		return call_slot(*type_prototype().__divmod__, this, other);
	}
	// Fallback: 动态查找 __divmod__
	auto result = binary_op_fallback(this, other, "__divmod__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for divmod(): \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::and_(PyObject *other)
{
	if (type_prototype().__and__.has_value()) {
		return call_slot(*type_prototype().__and__, this, other);
	} else if (other->type_prototype().__and__.has_value()) {
		return call_slot(*other->type_prototype().__and__, other, this);
	}
	// Fallback: 动态查找 __and__
	auto result = binary_op_fallback(this, other, "__and__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for &: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::or_(PyObject *other)
{
	if (type_prototype().__or__.has_value()) {
		return call_slot(*type_prototype().__or__, this, other);
	} else if (other->type_prototype().__or__.has_value()) {
		return call_slot(*other->type_prototype().__or__, other, this);
	}
	// Fallback: 动态查找 __or__
	auto result = binary_op_fallback(this, other, "__or__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for |: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<PyObject *> PyObject::xor_(PyObject *other)
{
	if (type_prototype().__xor__.has_value()) {
		return call_slot(*type_prototype().__xor__, this, other);
	} else if (other->type_prototype().__xor__.has_value()) {
		return call_slot(*other->type_prototype().__xor__, other, this);
	}
	// Fallback: 动态查找 __xor__
	auto result = binary_op_fallback(this, other, "__xor__");
	if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }

	return Err(type_error("unsupported operand type(s) for ^: \'{}\' and \'{}\'",
		type_prototype().__name__,
		other->type_prototype().__name__));
}

PyResult<bool> PyObject::contains(PyObject *value)
{
	if (auto seq = as_sequence(); seq.is_ok()) {
		if (type_prototype().sequence_type_protocol->__contains__.has_value()) {
			return seq.unwrap().contains(value);
		}
	}
	auto it = iter();
	if (it.is_err()) { return Err(it.unwrap_err()); }

	auto n = it.unwrap()->next();

	while (n.is_ok()) {
		if (auto r = value->eq(n.unwrap()); r.is_ok()) {
			// 修改：使用 RuntimeContext 的 is_true 方法
			if (RuntimeContext::has_current()) {
				if (RuntimeContext::current().is_true(r.unwrap())) { return Ok(true); }
			} else {
				// 降级
				if (r.unwrap() != py_false() && r.unwrap() != py_none()) { return Ok(true); }
			}
		} else if (r.is_err()) {
			return Err(r.unwrap_err());
		}
		n = it.unwrap()->next();
	}

	if (n.is_err() && (n.unwrap_err()->type() != stop_iteration()->type())) {
		return Err(n.unwrap_err());
	}
	return Ok(false);
}

PyResult<std::monostate> PyObject::delete_item(PyObject *key)
{
	if (auto mapping = as_mapping(); mapping.is_ok()) {
		if (type_prototype().mapping_type_protocol->__delitem__.has_value()) {
			return mapping.unwrap().delitem(key);
		}
	}

	// TODO: support sequence deletion

	return Err(type_error("'{}' object does not support item deletion", type_prototype().__name__));
}

// [Fix] 一元操作符 fallback
PyResult<PyObject *> PyObject::abs() const
{
	if (type_prototype().__abs__.has_value()) { return call_slot(*type_prototype().__abs__, this); }

	// Fallback
	auto name = PyString::create("__abs__");
	if (name.is_ok()) {
		auto [method, found] = lookup_attribute(name.unwrap());
		if (found == LookupAttrResult::FOUND && method.is_ok()) {
			// [Fix] create({}) -> create() 调用无参版本
			return method.unwrap()->call(PyTuple::create().unwrap(), nullptr);
		}
	}

	return Err(type_error("bad operand type for abs(): '{}'", type_prototype().__name__));
}

PyResult<PyObject *> PyObject::neg() const
{
	if (type_prototype().__neg__.has_value()) { return call_slot(*type_prototype().__neg__, this); }

	// Fallback
	auto name = PyString::create("__neg__");
	if (name.is_ok()) {
		auto [method, found] = lookup_attribute(name.unwrap());
		if (found == LookupAttrResult::FOUND && method.is_ok()) {
			// [Fix] create({}) -> create() 调用无参版本
			return method.unwrap()->call(PyTuple::create().unwrap(), nullptr);
		}
	}

	return Err(type_error("bad operand type for unary -: '{}'", type_prototype().__name__));
}

PyResult<PyObject *> PyObject::pos() const
{
	if (type_prototype().__pos__.has_value()) { return call_slot(*type_prototype().__pos__, this); }

	// Fallback
	auto name = PyString::create("__pos__");
	if (name.is_ok()) {
		auto [method, found] = lookup_attribute(name.unwrap());
		if (found == LookupAttrResult::FOUND && method.is_ok()) {
			// [Fix] create({}) -> create() 调用无参版本
			return method.unwrap()->call(PyTuple::create().unwrap(), nullptr);
		}
	}

	return Err(type_error("bad operand type for unary +: '{}'", type_prototype().__name__));
}

PyResult<PyObject *> PyObject::invert() const
{
	if (type_prototype().__invert__.has_value()) {
		return call_slot(*type_prototype().__invert__, this);
	}

	// Fallback
	auto name = PyString::create("__invert__");
	if (name.is_ok()) {
		auto [method, found] = lookup_attribute(name.unwrap());
		if (found == LookupAttrResult::FOUND && method.is_ok()) {
			// [Fix] create({}) -> create() 调用无参版本
			return method.unwrap()->call(PyTuple::create().unwrap(), nullptr);
		}
	}

	return Err(type_error("bad operand type for unary ~: '{}'", type_prototype().__name__));
}

PyResult<bool> PyObject::true_()
{
	if (type_prototype().__bool__.has_value()) {
		return call_slot(*type_prototype().__bool__, "__bool__ should return bool", this);
	} else if (auto mapping = as_mapping();
		mapping.is_ok() && type_prototype().mapping_type_protocol->__len__.has_value()) {
		return mapping.unwrap().len().and_then(
			[](size_t l) -> PyResult<bool> { return Ok(l > 0); });
	} else if (auto sequence = as_sequence();
		sequence.is_ok() && type_prototype().sequence_type_protocol->__len__.has_value()) {
		return sequence.unwrap().len().and_then(
			[](size_t l) -> PyResult<bool> { return Ok(l > 0); });
	} else {
		return Ok(true);
	}
}

PyResult<size_t> PyObject::len() const
{
	auto *self = const_cast<PyObject *>(this);

	// 1. 优先 mapping 协议 (与 CPython PyObject_Size 一致)
	if (type_prototype().mapping_type_protocol.has_value()
		&& type_prototype().mapping_type_protocol->__len__.has_value()) {
		return self->as_mapping().unwrap().len();
	}

	// 2. 回退 sequence 协议
	if (type_prototype().sequence_type_protocol.has_value()
		&& type_prototype().sequence_type_protocol->__len__.has_value()) {
		return self->as_sequence().unwrap().len();
	}

	return Err(type_error("object of type '{}' has no len()", type()->name()));
}

PyResult<PyObject *> PyObject::iter() const
{
	// spdlog::error("iter called on {}", type()->name());
	if (type_prototype().__iter__.has_value()) {
		return call_slot(*type_prototype().__iter__, this);
	}

	if (type_prototype().sequence_type_protocol.has_value()
		&& type_prototype().sequence_type_protocol->__getitem__.has_value()) {
		return PyIterator::create(const_cast<PyObject *>(this));
	}

	return Err(type_error("'{}' object is not iterable", type()->name()));
}

PyResult<PyObject *> PyObject::next()
{
	if (type_prototype().__next__.has_value()) {
		return call_slot(*type_prototype().__next__, this);
	}

	return Err(type_error("'{}' object is not an iterator", type()->name()));
}

PyResult<PyObject *> PyObject::get(PyObject *instance, PyObject *owner) const
{
	if (type_prototype().__get__.has_value()) {
		return call_slot(*type_prototype().__get__, this, instance, owner);
	}
	TODO();
}
//
// 消除 variant，高速直通传参方案的基类默认实现 (旧版回退兼容)
PyResult<PyObject *> PyObject::call_fast_ptrs(PyObject **args, size_t argc, PyDict *kwargs)
{
	if (argc == 0 && (!kwargs || kwargs->map().empty())) {
		static PyTuple *s_empty_tuple = nullptr;
		if (__builtin_expect(s_empty_tuple == nullptr, 0)) {
			s_empty_tuple = PyTuple::create().unwrap();
		}
		return call(s_empty_tuple, kwargs);
	}

	// 通用回退仍需构造 PyTuple，但不再强制把 tagged int 装箱成 PyInteger。
	py::GCVector<Value> tuple_args;
	tuple_args.reserve(argc);
	for (size_t i = 0; i < argc; ++i) { tuple_args.push_back(RtValue::from_ptr(args[i])); }

	return PyTuple::create(std::move(tuple_args))
		.and_then([this, kwargs](PyTuple *t) -> PyResult<PyObject *> { return call(t, kwargs); });
}

PyResult<PyObject *> PyObject::new_(PyTuple *args, PyDict *kwargs) const
{
	if (!as<PyType>(this)) {
		// FIXME: should be SystemError
		return Err(type_error("__new__() called with non-type 'self'"));
	}
	if (!args || args->size() < 1) {
		return Err(type_error("object.__new__(): not enough arguments"));
	}
	auto maybe_type_ = PyObject::from(args->elements()[0]);
	if (maybe_type_.is_err()) return maybe_type_;
	auto *maybe_type = maybe_type_.unwrap();
	if (!as<PyType>(maybe_type)) {
		return Err(type_error(
			"object.__new__(X): X is not a type object ({})", maybe_type->type()->name()));
	}
	auto *type = as<PyType>(maybe_type);
	// TODO: check type is a subtype of self (which we know here is `const PyType*`)
	//       otherwise raise TypeError("{}.__new__({}): {} is not a subtype of {}")

	// pop out type from args
	std::vector<Value> new_args;
	new_args.reserve(args->size() - 1);
	// new_args.resize(args->size() - 1);
	new_args.insert(new_args.end(), args->elements().begin() + 1, args->elements().end());
	auto args_ = PyTuple::create(new_args);
	if (args_.is_err()) return args_;
	args = args_.unwrap();

	if (type_prototype().__new__.has_value()) {
		return call_slot(*type_prototype().__new__, type, args, kwargs);
	}
	TODO();
}

PyResult<int32_t> PyObject::init(PyTuple *args, PyDict *kwargs)
{
	// 重构：复用 init_raw 逻辑
	return init_fast_ptrs(
		reinterpret_cast<PyObject **>(const_cast<py::Value *>(args->elements().data())),
		args->size(),
		kwargs);
}


PyResult<PyObject *> PyType::call_fast_ptrs(PyObject **args, size_t argc, PyDict *kwargs)
{
	// 1. 特殊处理 type(x)
	if (this == py::types::type() && argc == 1 && (!kwargs || kwargs->map().empty())) {
		return Ok(py::ensure_box(args[0])->type());
	}

	// 1.5 [性能优化] str(x) 快速路径：直接调用 x.__str__() 或 x.__repr__()，
	//     完全跳过 __new__ / PyTuple 打包。（等价于 CPython str_new 对单参数的优化）
	if (this == py::types::str() && argc == 1 && (!kwargs || kwargs->map().empty())) {
		// [性能优化] tagged int 直接转字符串，避免先 box 再 __str__
		auto rt = RtValue::from_ptr(args[0]);
		if (rt.is_tagged_int()) {
			auto s = std::to_string(rt.as_int());
			// [性能优化] 使用 create_raw 跳过 intern，避免 mutex + hashtable 开销
			// 数字字符串通常不做 dict key，只被临时迭代（如 trie 构建中 for ch in str(n)）
			return PyString::create_raw(std::move(s));
		}
		auto *obj = args[0];// 已经是真实 PyObject*（非 tagged）
		// 如果已经是 str 直接返回
		if (obj->type() == py::types::str()) { return Ok(obj); }
		// 调用 __str__
		auto str_res = obj->str();
		if (str_res.is_ok()) { return str_res; }
		// 回退到 __repr__
		return obj->repr();
	}

	// 1.6 [性能优化] int(x) 快速路径
	if (this == py::types::integer() && argc == 1 && (!kwargs || kwargs->map().empty())) {
		auto *obj = py::ensure_box(args[0]);
		if (obj->type() == py::types::integer()) { return Ok(obj); }
	}

	// 2. [性能优化] 默认 object.__new__ 的快速路径：
	//    对于 heap type（用户自定义类），__new__ 始终是 PyObject::__new__
	//    （只调 __alloc__），无需打包 PyTuple，直接 alloc + init。
	//    同时支持任意 argc（不再限于 argc == 0），大幅减少 PyTuple 分配。
	//
	//    对 Python 层覆盖 def __new__ 的类，__new__ slot 存为 PyObject* variant，
	//    此时走回退路径。
	{
		bool is_default_new = false;
		if (underlying_type().is_heaptype && underlying_type().__new__.has_value()
			&& !std::holds_alternative<PyObject *>(*underlying_type().__new__)) {
			is_default_new = true;
		}
		// 非 heaptype（内建类型 str/int/list 等）仍然使用旧的地址比较。
		// 在同一 TU 内它是可靠的。
		if (!is_default_new && !underlying_type().is_heaptype
			&& underlying_type().__new__.has_value() && argc == 0
			&& (!kwargs || kwargs->map().empty())) {
			static size_t default_new_addr = 0;
			if (__builtin_expect(default_new_addr == 0, 0)) {
				default_new_addr = get_address(*types::object()->underlying_type().__new__);
			}
			if (get_address(*underlying_type().__new__) == default_new_addr) {
				is_default_new = true;
			}
		}

		if (is_default_new) {
			auto obj_res = underlying_type().__alloc__(const_cast<PyType *>(this));
			if (obj_res.is_err()) return obj_res;
			auto *obj = obj_res.unwrap();

			// [性能优化] 预分配 slot vector 容量，避免 __init__ 中每次 setattr 的 push_back 触发
			// realloc 对于 Node(children, terminal)，slot 从 0 增长到 2，每次 push_back 都 realloc
			// 预分配后一次性到位，消除 6M 次 realloc
			if (underlying_type().is_heaptype) {
				auto expected = m_cached_slot_count.load(std::memory_order_relaxed);
				if (__builtin_expect(expected > 0, 1)) { obj->m_slots.reserve(expected); }
			}

			if (obj->type()->issubclass(const_cast<PyType *>(this))) {
				auto init_res = obj->init_fast_ptrs(args, argc, kwargs);
				if (init_res.is_err()) return Err(init_res.unwrap_err());
			}

			// [性能优化] 记录第一次 __init__ 完成后的 slot 数量，用于后续预分配
			if (underlying_type().is_heaptype
				&& m_cached_slot_count.load(std::memory_order_relaxed) == 0) {
				auto actual = obj->m_slots.size();
				if (actual > 0) { m_cached_slot_count.store(actual, std::memory_order_relaxed); }
			}

			return Ok(obj);
		}
	}

	// 3. 回退路径：仍需构造 tuple 交给旧 __new__ 协议
	PyTuple *tuple_args = nullptr;
	if (argc == 0 && (!kwargs || kwargs->map().empty())) {
		static PyTuple *s_empty_tuple = nullptr;
		if (__builtin_expect(s_empty_tuple == nullptr, 0)) {
			s_empty_tuple = PyTuple::create().unwrap();
		}
		tuple_args = s_empty_tuple;
	} else {
		py::GCVector<Value> elements;
		elements.reserve(argc);
		for (size_t i = 0; i < argc; ++i) { elements.push_back(RtValue::from_ptr(args[i])); }
		auto tuple_res = PyTuple::create(std::move(elements));
		if (tuple_res.is_err()) return Err(tuple_res.unwrap_err());
		tuple_args = tuple_res.unwrap();
	}

	auto obj_res = new_(tuple_args, kwargs);
	if (obj_res.is_err()) return obj_res;
	auto *obj = obj_res.unwrap();

	// 4. 调用 obj.__init__
	if (obj->type()->issubclass(const_cast<PyType *>(this))) {
		auto init_res = obj->init_fast_ptrs(args, argc, kwargs);
		if (init_res.is_err()) return Err(init_res.unwrap_err());
	}

	return Ok(obj);
}

//
// 高速 C ABI init
PyResult<int32_t> PyObject::init_fast_ptrs(PyObject **args, size_t argc, PyDict *kwargs)
{
	// [性能优化] 使用 PyType 上的 __init__ 缓存，避免每次构造对象都做 lookup_attribute
	// 对于 Node() 这样的 heap type，__init__ 在类创建后不会改变
	// 用 global_version 校验缓存有效性，确保在类被修改时自动失效
	auto *t = type();
	auto global_ver = PyType::global_version();
	PyObject *init_method = nullptr;

	if (__builtin_expect(
			t->m_cached_init != nullptr && t->m_cached_init_version == global_ver, 1)) {
		init_method = t->m_cached_init;
	} else {
		auto *init_str = PyString::intern("__init__");
		auto [res, found] = t->lookup_attribute(init_str);

		if (found == LookupAttrResult::FOUND) {
			init_method = res.unwrap();
			// 缓存结果用于后续调用
			t->m_cached_init = init_method;
			t->m_cached_init_version = global_ver;
		}
	}

	if (init_method) {
		size_t total_argc = argc + 1;

		PyObject *raw_args_array[16];
		PyObject **final_args = args;
		if (total_argc <= 16) {
			final_args = raw_args_array;
		} else {
			final_args = static_cast<PyObject **>(alloca(sizeof(PyObject *) * total_argc));
		}

		final_args[0] = this;
		for (size_t i = 0; i < argc; ++i) final_args[i + 1] = args[i];

		auto call_res = init_method->call_fast_ptrs(final_args, total_argc, kwargs);

		if (call_res.is_err()) return Err(call_res.unwrap_err());
		return Ok(static_cast<int32_t>(0));
	}
	return Ok(static_cast<int32_t>(0));
}

PyResult<PyObject *> PyObject::getitem(PyObject *key)
{
	if (as_mapping().is_ok() && type_prototype().mapping_type_protocol->__getitem__.has_value()) {
		return call_slot(*type_prototype().mapping_type_protocol->__getitem__, this, key);
	} else if (as_sequence().is_ok()
			   && type_prototype().sequence_type_protocol->__getitem__.has_value()) {
		if (!as<PyInteger>(key)) {
			return Err(type_error("sequence index must be integer, not '{}'", key->type()->name()));
		}
		const auto index = as<PyInteger>(key)->as_i64();
		return call_slot(*type_prototype().sequence_type_protocol->__getitem__, this, index);
	}
	// TODO: could getitem be virtual and we override the logic below in PyType?
	if (as<PyType>(this)) {
		if (this == types::type()) { return PyGenericAlias::create(this, key); }
		auto method = get_method(PyString::create("__class_getitem__").unwrap());
		if (method.is_ok()) {
			return method.unwrap()->call(PyTuple::create(key).unwrap(), PyDict::create().unwrap());
		}
	}
	return Err(type_error("'{}' object is not subscriptable", type()->name()));
}

PyResult<PyObject *> PyObject::getitem(size_t index)
{
	if (as_sequence().is_ok() && type_prototype().sequence_type_protocol->__getitem__.has_value()) {
		return call_slot(*type_prototype().sequence_type_protocol->__getitem__, this, index);
	}

	return Err(type_error("'{}' object is not subscriptable", type()->name()));
}

PyResult<std::monostate> PyObject::setitem(PyObject *key, PyObject *value)
{
	if (as_mapping().is_ok() && type_prototype().mapping_type_protocol->__setitem__.has_value()) {
		return call_slot(
			*type_prototype().mapping_type_protocol->__setitem__, "", this, key, value);
	} else if (as_sequence().is_ok()
			   && type_prototype().sequence_type_protocol->__setitem__.has_value()) {
		if (!as<PyInteger>(key)) {
			return Err(type_error("sequence index must be integer, not '{}'", key->type()->name()));
		}
		const auto index = as<PyInteger>(key)->as_i64();
		return call_slot(
			*type_prototype().sequence_type_protocol->__setitem__, "", this, index, value);
	}

	return Err(type_error("'{}' object does not support item assignment", type()->name()));
}

PyResult<std::monostate> PyObject::delitem(PyObject *key)
{
	if (as_mapping().is_ok() && type_prototype().mapping_type_protocol->__delitem__.has_value()) {
		return call_slot(*type_prototype().mapping_type_protocol->__delitem__, "", this, key);
	} else if (as_sequence().is_ok()
			   && type_prototype().sequence_type_protocol->__delitem__.has_value()) {
		if (!as<PyInteger>(key)) {
			return Err(type_error("sequence index must be integer, not '{}'", key->type()->name()));
		}
		const auto index = as<PyInteger>(key)->as_i64();
		return call_slot(*type_prototype().sequence_type_protocol->__delitem__, "", this, index);
	}

	return Err(type_error("'{}' object does not support item deletion", type()->name()));
}


PyResult<PyObject *> PyObject::__eq__(const PyObject *other) const
{
	return Ok(this == other ? py_true() : not_implemented());
}

PyResult<PyObject *> PyObject::__ne__(const PyObject *other) const
{
	if (!type_prototype().__eq__.has_value()) { return Ok(not_implemented()); }

	return call_slot(*type_prototype().__eq__, this, other).and_then([](PyObject *obj) {
		// 修改：使用 RuntimeContext 的 is_true 方法
		if (RuntimeContext::has_current()) {
			bool is_true = RuntimeContext::current().is_true(obj);
			return Ok(is_true ? py_false() : py_true());
		}
		// 降级：假设非 false/none 为真
		return Ok(obj == py_false() || obj == py_none() ? py_true() : py_false());
	});
}

PyResult<PyObject *> PyObject::__getattribute__(PyObject *attribute) const
{
	if (!as<PyString>(attribute)) {
		return Err(
			type_error("attribute name must be a string, not {}", attribute->type()->to_string()));
	}

	auto *name = as<PyString>(attribute);

	auto descriptor_ = type()->lookup(name);
	if (descriptor_.has_value() && descriptor_->is_err()) { return *descriptor_; }

	bool descriptor_has_get = false;

	if (descriptor_.has_value() && descriptor_->is_ok()) {
		auto *descriptor = descriptor_->unwrap();
		const auto &descriptor_get = descriptor->type()->underlying_type().__get__;
		if (descriptor_get.has_value()) { descriptor_has_get = true; }
		if (descriptor_get.has_value() && descriptor_is_data(descriptor->type())) {
			return descriptor->get(const_cast<PyObject *>(this), type());
		}
	}
	if (m_shape) {
		if (auto offset = m_shape->lookup(as<PyString>(name))) {
			return PyObject::from(m_slots[*offset]);
		}
	}

	if (descriptor_.has_value() && descriptor_has_get) {
		return descriptor_->unwrap()->get(const_cast<PyObject *>(this), type());
	}

	if (descriptor_.has_value() && descriptor_->is_ok()) { return *descriptor_; }

	return Err(attribute_error(
		"'{}' object has no attribute '{}'", type_prototype().__name__, name->to_string()));
}

PyResult<PyObject *> PyObject::get_attribute(PyObject *name) const
{
	if (!as<PyString>(name)) {
		return Err(
			type_error("attribute name must be a string, not {}", name->type()->to_string()));
	}
	const auto &getattribute_ = type()->underlying_type().__getattribute__;
	if (getattribute_.has_value()) { return getattribute(name); }

	return Err(
		attribute_error("'{}' object has no attribute '{}'", type()->name(), name->to_string()));
}

std::tuple<PyResult<PyObject *>, LookupAttrResult> PyObject::lookup_attribute(PyObject *name) const
{
	auto result = [&]() -> std::tuple<PyResult<PyObject *>, LookupAttrResult> {
		if (!as<PyString>(name)) {
			return { Err(type_error(
						 "attribute name must be a string, not {}", name->type()->to_string())),
				LookupAttrResult::NOT_FOUND };
		}

		const auto &getattribute_ = type()->underlying_type().__getattribute__;
		if (getattribute_.has_value()
			&& get_address(*getattribute_)
				   != get_address(*types::object()->underlying_type().__getattribute__)) {
			return { get_attribute(name), LookupAttrResult::FOUND };
		}
		// TODO: check tp_getattr? This field is deprecated in [c]python so maybe should not
		// even bother to implement it here?
		else if (getattribute_.has_value()) {
			return { call_slot(*getattribute_, this, name), LookupAttrResult::FOUND };
		} else {
			return { Ok(py_none()), LookupAttrResult::NOT_FOUND };
		}
	}();

	if (std::get<0>(result).is_err()
		&& (std::get<0>(result).unwrap_err()->type() == AttributeError::class_type())) {
		return { Ok(py_none()), LookupAttrResult::NOT_FOUND };
	} else if (std::get<0>(result).is_err()) {
		return { std::get<0>(result), LookupAttrResult::NOT_FOUND };
	} else {
		return result;
	}
}


PyResult<PyObject *> PyObject::get_method(PyObject *name) const
{
	{
		// check if the derived object uses the default __getattribute__
		const auto &getattribute_ = type()->underlying_type().__getattribute__;
		if (getattribute_.has_value()
			&& get_address(*getattribute_)
				   != get_address(*types::object()->underlying_type().__getattribute__)) {
			return get_attribute(name);
		}
	}

	auto descriptor_ = type()->lookup(name);
	if (descriptor_.has_value() && descriptor_->is_err()) { return *descriptor_; }

	bool method_found = false;

	if (descriptor_.has_value() && descriptor_->is_ok()) {
		auto *descriptor = descriptor_->unwrap();
		if (is_method_descriptor(descriptor->type())) {
			method_found = true;
		} else {
			if (descriptor->type()->underlying_type().__get__.has_value()) {
				return descriptor->get(const_cast<PyObject *>(this), type());
			}
		}
	}
	if (m_shape) {
		if (auto offset = m_shape->lookup(as<PyString>(name))) {
			return PyObject::from(m_slots[*offset]);
		}
	}

	if (descriptor_.has_value() && method_found) {
		auto result = descriptor_->unwrap()->get(const_cast<PyObject *>(this), type());
		ASSERT(result.is_ok());
		return result;
	}

	return Err(attribute_error(
		"'{}' object has no attribute '{}'", type_prototype().__name__, name->to_string()));
}

// PyResult<std::monostate> PyObject::__setattribute__(PyObject *attribute, PyObject *value)
// {
// 	if (!as<PyString>(attribute)) {
// 		return Err(
// 			type_error("attribute name must be string, not '{}'", attribute->type()->to_string()));
// 	}

// 	if (as<PyType>(this)) { PyType::inc_global_version(); }

// 	auto descriptor_ = type()->lookup(attribute);

// 	if (descriptor_.has_value() && descriptor_->is_ok()) {
// 		auto *descriptor = descriptor_->unwrap();
// 		const auto &descriptor_set = descriptor->type()->underlying_type().__set__;
// 		if (descriptor_set.has_value()) {
// 			return call_slot(*descriptor_set, "", descriptor, this, value);
// 		}
// 	}

// 	if (!m_attributes) {
// 		if (descriptor_.has_value() && descriptor_->is_ok()) {
// 			return Err(attribute_error(
// 				"'{}' object attribute '{}' is read-only", type()->name(), attribute->to_string()));
// 		} else {
// 			return Err(attribute_error(
// 				"'{}' object has no attribute '{}'", type()->name(), attribute->to_string()));
// 		}
// 	}

// 	m_attributes->insert(attribute, value);

// 	return Ok(std::monostate{});
// }

PyResult<std::monostate> PyObject::__setattribute__(PyObject *attribute, PyObject *value)
{
	if (!as<PyString>(attribute)) {
		return Err(
			type_error("attribute name must be string, not '{}'", attribute->type()->name()));
	}

	// Python 3.9 语义：如果是修改 Type 对象（类属性），必须使全局缓存失效
	if (auto *type_self = as<PyType>(this)) {
		PyType::inc_global_version();
		// 类属性存储在 TypePrototype::__dict__ 中，而非 shape/slots
		// 必须同步更新 __dict__，否则 PyType::lookup / PyType::__getattribute__ 会读到旧值
		auto *dict = type_self->underlying_type().__dict__;
		if (dict) { dict->insert(RtValue::from_ptr(attribute), RtValue::from_ptr(value)); }
		return Ok(std::monostate{});
	}

	auto descriptor_ = type()->lookup(attribute);
	if (descriptor_.has_value() && descriptor_->is_err()) { return Err(descriptor_->unwrap_err()); }

	if (descriptor_.has_value() && descriptor_->is_ok()) {
		auto *descriptor = descriptor_->unwrap();
		// 数据描述符（Data Descriptor）具有最高优先级，优先于实例字典
		if (descriptor_is_data(descriptor->type())) {
			const auto &descriptor_set = descriptor->type()->underlying_type().__set__;
			if (descriptor_set.has_value()) {
				return call_slot(*descriptor_set, "", descriptor, this, value);
			}
		}
	}
	if (!m_shape) {
		m_shape = Shape::root(type())->transition(as<PyString>(attribute));
		m_slots.push_back(value);
		return Ok(std::monostate{});
	}
	if (auto offset = m_shape->lookup(as<PyString>(attribute))) {
		m_slots[*offset] = value;
		return Ok(std::monostate{});
	}
	m_shape = m_shape->transition(as<PyString>(attribute));
	m_slots.push_back(value);
	return Ok(std::monostate{});
}


PyResult<std::monostate> PyObject::__delattribute__(PyObject *attribute)
{
	if (!as<PyString>(attribute)) {
		return Err(
			type_error("attribute name must be string, not '{}'", attribute->type()->to_string()));
	}

	auto descriptor_ = type()->lookup(attribute);

	if (descriptor_.has_value() && descriptor_->is_ok()) {
		auto *descriptor = descriptor_->unwrap();
		const auto &descriptor_delete = descriptor->type()->underlying_type().__delete__;
		if (descriptor_delete.has_value()) {
			return call_slot(*descriptor_delete, "", descriptor, this);
		}
	}
	// TODO: support delattr in shape
	return Err(attribute_error("can't delete attribute"));

	return Ok(std::monostate{});
}

PyResult<int64_t> PyObject::__hash__() const { return Ok(bit_cast<size_t>(this) >> 4); }

bool PyObject::is_callable() const { return type_prototype().__call__.has_value(); }

const std::string &PyObject::name() const { return type_prototype().__name__; }

PyResult<PyObject *> PyObject::__new__(const PyType *type, PyTuple *args, PyDict *kwargs)
{
	if ((args && !args->elements().empty()) || (kwargs && !kwargs->map().empty())) {

		if (!type->underlying_type().__new__.has_value()) {
			const auto new_fn = get_address(*type->underlying_type().__new__);
			ASSERT(new_fn);

			ASSERT(types::object()->underlying_type().__new__);
			const auto custom_new_fn = get_address(*types::object()->underlying_type().__new__);
			ASSERT(custom_new_fn);

			if (new_fn != custom_new_fn) {
				return Err(type_error(
					"object.__new__() takes exactly one argument (the type to instantiate)"));
			}
		}

		if (!type->underlying_type().__init__.has_value()) {
			ASSERT(type->underlying_type().__init__);
			const auto init_fn = get_address(*type->underlying_type().__init__);
			ASSERT(init_fn);

			ASSERT(types::object()->underlying_type().__init__);
			const auto custom_init_fn = get_address(*types::object()->underlying_type().__init__);
			if (init_fn == custom_init_fn) {
				return Err(type_error("object() takes no arguments"));
			}
		}
	}
	return type->underlying_type().__alloc__(const_cast<PyType *>(type));
}

PyResult<int32_t> PyObject::__init__(PyTuple *args, PyDict *kwargs)
{
	if ((args && !args->elements().empty()) || (kwargs && !kwargs->map().empty())) {
		if (!type()->underlying_type().__new__.has_value()) {
			ASSERT(type()->underlying_type().__new__);
			const auto new_fn = get_address(*type()->underlying_type().__new__);
			ASSERT(new_fn);

			ASSERT(types::object()->underlying_type().__new__);
			const auto custom_new_fn = get_address(*types::object()->underlying_type().__new__);
			ASSERT(custom_new_fn);

			if (new_fn == custom_new_fn) {
				return Err(type_error(
					"object.__new__() takes exactly one argument (the type to instantiate)"));
			}
		}

		if (!type()->underlying_type().__init__.has_value()) {
			ASSERT(type()->underlying_type().__init__);
			const auto init_fn = get_address(*type()->underlying_type().__init__);
			ASSERT(init_fn);

			ASSERT(types::object()->underlying_type().__init__);
			const auto custom_init_fn = get_address(*types::object()->underlying_type().__init__);
			ASSERT(custom_init_fn);

			if (init_fn != custom_init_fn) {
				return Err(type_error("object() takes no arguments"));
			}
		}
	}
	return Ok(0);
}

std::string PyObject::to_string() const
{
	return fmt::format("PyObject at {}", static_cast<const void *>(this));
}

// PyType *PyObject::type() const
// {
// 	// 直接通过地址特征判断 Tagged Pointer 类型，跳过 variant 访问
// 	uintptr_t addr = reinterpret_cast<uintptr_t>(this);
// 	if (addr & 1) {
// 		return types::integer();// Tagged Integer 永远是 int 类型
// 	}
// 	if (!addr) return nullptr;
// 	if (std::holds_alternative<std::reference_wrapper<const TypePrototype>>(m_type)) {
// 		return static_type();
// 	} else {
// 		ASSERT(std::holds_alternative<PyType *>(m_type));
// 		return std::get<PyType *>(m_type);
// 	}
// }

// PyType *PyObject::type() const { return m_bits_type; }

// PyType *PyObject::static_type() const
// {
// 	ASSERT(m_bits_type && "m_bits_type ptr is NULL !!");
// 	return m_bits_type;
// }


namespace {

	std::once_flag object_type_flag;

	std::unique_ptr<TypePrototype> register_type()
	{
		// return std::move(klass<PyObject>("object")
		// 		.property(
		// 			"__class__",
		// 			[](PyObject *self) { return Ok(self->type()); },
		// 			[](PyObject *self, PyObject *value) -> PyResult<std::monostate> {
		// 				(void)self;
		// 				(void)value;
		// 				TODO();
		// 			})
		// 		.type);

		return std::move(klass<PyObject>("object")
				.property_readonly("__class__",
					[](PyObject *self) -> PyResult<PyObject *> {
						// 显式转为 PyObject* 并包裹
						return Ok(static_cast<PyObject *>(self->type()));
					})
				.type);
	}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyObject::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(object_type_flag, []() { type = register_type(); });
		return std::move(type);
	};
}

namespace detail {
	size_t slot_count(PyType *t) { return t->__slots__.size(); }
}// namespace detail
}// namespace py
