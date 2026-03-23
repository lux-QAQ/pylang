#include "PyObject.hpp"

#include "AttributeError.hpp"
#include "NotImplemented.hpp"
#include "NotImplementedError.hpp"
#include "PyBool.hpp"
#include "PyBytes.hpp"
#include "PyDict.hpp"
#include "PyEllipsis.hpp"
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
#include "runtime/Value.hpp"
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
	for (auto *b : __bases__) {
		if (b) visitor.visit(*b);
	}
}

size_t ValueHash::operator()(const Value &value) const
{
	// TODO: put these hash functions somewhere global so they can be reused by the respective
	//       PyObject classes
	const auto result = std::visit(
		overloaded{ [](const Number &number) -> size_t {
					   if (std::holds_alternative<double>(number.value)) {
						   return std::hash<double>{}(std::get<double>(number.value));
					   } else {
						   if (std::get<BigIntType>(number.value) == -1) return -2;
						   ASSERT(std::get<BigIntType>(number.value).fits_ulong_p());
						   return std::get<BigIntType>(number.value).get_ui();
					   }
				   },
			[](const String &s) -> size_t { return std::hash<std::string>{}(s.s); },
			[](const Bytes &b) -> size_t {
				std::string_view sv{ bit_cast<char *>(b.b.data()), b.b.size() };
				return static_cast<int64_t>(std::hash<std::string_view>{}(sv));
			},
			[](const Ellipsis &) -> size_t { return ::bit_cast<size_t>(py_ellipsis()); },
			[](const NameConstant &c) -> size_t {
				if (std::holds_alternative<bool>(c.value)) {
					return std::get<bool>(c.value) ? 0 : 1;
				} else {
					return bit_cast<size_t>(py_none()) >> 4;
				}
			},
			[](const Tuple &t) -> size_t { return ::bit_cast<size_t>(t.elements.data()); },
			[](PyObject *obj) -> size_t {
				auto val = obj->hash();
				ASSERT(val.is_ok());
				return val.unwrap();
			} },
		value);
	return result;
}

bool ValueEq::operator()(const Value &lhs, const Value &rhs) const
{
	auto result = equals(lhs, rhs).and_then([](const auto &result) { return truthy(result); });
	if (result.is_err()) { return false; }
	return result.unwrap();
}

namespace {

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

template<> PyResult<PyObject *> PyObject::from(const Number &value)
{
	return PyNumber::create(value);
}

template<> PyResult<PyObject *> PyObject::from(const int64_t &value)
{
	return PyNumber::create(Number{ value });
}

template<> PyResult<PyObject *> PyObject::from(const size_t &value)
{
	return PyNumber::create(Number{ value });
}

template<> PyResult<PyObject *> PyObject::from(const String &value)
{
	return PyString::create(value.s);
}

template<> PyResult<PyObject *> PyObject::from(const Bytes &value)
{
	return PyBytes::create(value);
}

template<> PyResult<PyObject *> PyObject::from(const Ellipsis &) { return Ok(py_ellipsis()); }

template<> PyResult<PyObject *> PyObject::from(const NameConstant &value)
{
	if (std::holds_alternative<NoneType>(value.value)) {
		return Ok(py_none());
	} else {
		const bool bool_value = std::get<bool>(value.value);
		return Ok(bool_value ? py_true() : py_false());
	}
}

template<> PyResult<PyObject *> PyObject::from(const Tuple &tuple)
{
	return PyTuple::create(tuple.elements);
}

template<> PyResult<PyObject *> PyObject::from(const Value &value)
{
	return std::visit([](const auto &v) { return PyObject::from(v); }, value);
}

PyObject::PyObject(const TypePrototype &type) : Cell()
{
	m_bits_type = types::lookup_type_by_prototype(&type);
	if (!m_bits_type) {
		spdlog::error("bootstrap type missing: {}", type.__name__);
		ASSERT(m_bits_type && "PyType must be initialized before object creation.");
	}
}

PyObject::PyObject(PyType *type) : Cell(), m_bits_type(type) { /* ASSERT(type); */ }

// const TypePrototype &PyObject::type_prototype() const { return type()->underlying_type(); }

const TypePrototype &PyObject::type_prototype() const { return m_bits_type->underlying_type(); }


PyBaseObject::PyBaseObject(const TypePrototype &type) : PyObject(type) {}


// void PyObject::visit_graph(Visitor &visitor)
// {
// 	if (m_attributes) { visitor.visit(*m_attributes); }
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
	if (m_attributes) { visitor.visit(*m_attributes); }

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
	if (this == other) { return Ok(py_true()); }
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
	return Ok(this == other ? py_true() : py_false());
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
	if (this == other) { return Ok(py_false()); }
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
	return Ok(this != other ? py_true() : py_false());
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

PyResult<PyObject *> PyObject::call(PyTuple *args, PyDict *kwargs)
{
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
		// 注意：这是 __radd__ 的逻辑，如果 lhs 没有 __add__，尝试 rhs 的 __radd__
		// 但目前的实现似乎是尝试 rhs 的 __add__ (可能不完全符合 Python 语义，但先保持原状)
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
// PyResult<PyObject *> PyObject::call_raw(std::span<Value> args, PyDict *kwargs)
// {
//     // 默认实现：回退到打包为 Tuple
//     auto tuple_res = PyTuple::create(py::GCVector<Value>(args.begin(), args.end()));
//     if (tuple_res.is_err()) { return Err(tuple_res.unwrap_err()); }

//     return call(tuple_res.unwrap(), kwargs);
// }

PyResult<PyObject *> PyObject::call_raw(std::span<const Value> args, PyDict *kwargs)
{
	// [极致优化]：Node() 路径。如果子类（如 PySlotWrapper）覆盖了 call_raw，
	// 这里就不应该再进行默认的 PyTuple 打包。

	if (args.empty() && (!kwargs || kwargs->map().empty())) {
		return call(PyTuple::create().unwrap(), kwargs);
	}

	// 只有必须打包时，才使用 GCVector
	py::GCVector<Value> elements;
	elements.reserve(args.size());
	for (auto &v : args) elements.push_back(v);

	return PyTuple::create(std::move(elements)).and_then([this, kwargs](PyTuple *t) {
		return this->call(t, kwargs);
	});
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
	return init_raw(args->elements(), kwargs);
}

PyResult<PyObject *> PyType::call_raw(std::span<const Value> args, PyDict *kwargs)
{
	// 1. 特殊处理 type(x)
	if (this == types::type() && args.size() == 1 && (!kwargs || kwargs->map().empty())) {
		return Ok(PyObject::from(args[0]).unwrap()->type());
	}

	// 2. [性能优化]：如果是无参构造且无关键字参数，使用空元组单例避免分配
	PyTuple *tuple_args = nullptr;
	if (args.empty() && (!kwargs || kwargs->map().empty())) {
		tuple_args = PyTuple::create().unwrap();
	} else {
		auto tuple_res = PyTuple::create(py::GCVector<Value>(args.begin(), args.end()));
		if (tuple_res.is_err()) return Err(tuple_res.unwrap_err());
		tuple_args = tuple_res.unwrap();
	}

	auto obj_res = new_(tuple_args, kwargs);
	if (obj_res.is_err()) return obj_res;
	auto *obj = obj_res.unwrap();

	// 3. 调用 obj.__init__
	if (obj->type()->issubclass(const_cast<PyType *>(this))) {
		auto init_res = obj->init_raw(args, kwargs);
		if (init_res.is_err()) return Err(init_res.unwrap_err());
	}

	return Ok(obj);
}

// PyResult<int32_t> PyObject::init_raw(std::span<const Value> args, PyDict *kwargs)
// {
// 	auto *init_str = PyString::intern("__init__");
// 	auto [res, found] = type()->lookup_attribute(init_str);

// 	if (found == LookupAttrResult::FOUND) {
// 		if (res.is_err()) return Err(res.unwrap_err());
// 		auto *init_method = res.unwrap();

// 		// 准备参数：[self, ...args]
// 		size_t total_argc = args.size() + 1;
// 		Value *stack_args = static_cast<Value *>(alloca(sizeof(Value) * total_argc));

// 		new (&stack_args[0]) Value(this);
// 		for (size_t i = 0; i < args.size(); ++i) { new (&stack_args[i + 1]) Value(args[i]); }

// 		auto call_res =
// 			init_method->call_raw(std::span<const Value>(stack_args, total_argc), kwargs);

// 		for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&stack_args[i]);

// 		if (call_res.is_err()) return Err(call_res.unwrap_err());
// 		return Ok(0);
// 	}

// 	// 回退到 C++ slot
// 	if (type_prototype().__init__.has_value()) {
// 		// 注意：Slot 接口目前仍需要 Tuple，但绝大多数 AOT 类不走这里
// 		auto args_tuple = PyTuple::create(py::GCVector<Value>(args.begin(), args.end()));
// 		if (args_tuple.is_err()) return Err(args_tuple.unwrap_err());
// 		return call_slot(*type_prototype().__init__, "", this, args_tuple.unwrap(), kwargs);
// 	}

// 	return Ok(0);
// }
PyResult<int32_t> PyObject::init_raw(std::span<const Value> args, PyDict *kwargs)
{
	auto *init_str = PyString::intern("__init__");
	auto [res, found] = type()->lookup_attribute(init_str);

	if (found == LookupAttrResult::FOUND) {
		auto *init_method = res.unwrap();
		size_t total_argc = args.size() + 1;
		// 使用 alloca 在栈上分配，不触发 GC 分配
		Value *stack_args = static_cast<Value *>(alloca(sizeof(Value) * total_argc));
		new (&stack_args[0]) Value(this);
		for (size_t i = 0; i < args.size(); ++i) new (&stack_args[i + 1]) Value(args[i]);

		auto call_res =
			init_method->call_raw(std::span<const Value>(stack_args, total_argc), kwargs);

		for (size_t i = 0; i < total_argc; ++i) std::destroy_at(&stack_args[i]);

		// 显式转换返回类型
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

	if (m_attributes) {
		const auto &dict = m_attributes->map();
		if (auto it = dict.find(name); it != dict.end()) { return PyObject::from(it->second); }
		// FIXME: we should abort here if PyDict returns an exception that is not an
		// AttributeError
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

	if (m_attributes) {
		const auto &dict = m_attributes->map();
		if (auto it = dict.find(name); it != dict.end()) { return PyObject::from(it->second); }
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
	if (as<PyType>(this)) { PyType::inc_global_version(); }

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

	// [核心优化]：延迟分配 m_attributes
	if (!m_attributes) {
		// 检查是否在描述符中被标记为只读（如非数据描述符但无 __set__）
		if (descriptor_.has_value() && descriptor_->is_ok()) {
			// 如果是只读描述符且没有 __set__，在没有 __dict__ 的情况下通常不允许设置
			// 但对于普通实例，通常直接进入 __dict__。这里保持 Python 默认行为：
			// 如果不是数据描述符，则尝试写入实例字典。
		}

		auto dict_res = PyDict::create();
		if (dict_res.is_err()) return Err(dict_res.unwrap_err());
		m_attributes = dict_res.unwrap();
	}

	return m_attributes->__setitem__(attribute, value);
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

	if (!m_attributes) {
		if (descriptor_.has_value() && descriptor_->is_ok()) {
			return Err(attribute_error(
				"'{}' object attribute '{}' is read-only", type()->name(), attribute->to_string()));
		} else {
			return Err(attribute_error(
				"'{}' object has no attribute '{}'", type()->name(), attribute->to_string()));
		}
	}

	m_attributes->remove(attribute);

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
						// [修复]: 显式转为 PyObject* 并包裹
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
