#include "PyModule.hpp"
#include "MemoryError.hpp"
#include "PyDict.hpp"
#include "PyList.hpp"
#include "PyString.hpp"
#include "ValueError.hpp"
#include "executable/Program.hpp"
#include "executable/bytecode/codegen/BytecodeGenerator.hpp"
#include "interpreter/InterpreterCore.hpp"
#include "modules/Modules.hpp"
#include "parser/Parser.hpp"
#include "runtime/AttributeError.hpp"
#include "runtime/NameError.hpp"
#include "runtime/RuntimeContext.hpp"
#include "types/api.hpp"
#include "types/builtin.hpp"

#include "runtime/compat.hpp"
#include <filesystem>

namespace py {

// 全局版本计数器 — 从 1 开始（0 表示"从未初始化"）
std::atomic<uint64_t> PyModule::s_global_version{ 1 };

PyModule::PyModule(PyType *type) : PyBaseObject(type) {}

PyModule::PyModule(PyDict *symbol_table, PyString *module_name, PyObject *doc)
	: PyBaseObject(types::BuiltinTypes::the().module()), m_module_name(module_name), m_doc(doc)
{
	m_package = PyString::create("").unwrap();
	m_loader = py_none();
	m_spec = py_none();

	// m_dict = symbol_table;
	m_dict = symbol_table;
	m_dict->insert(RtValue::from_ptr(PyString::create("__name__").unwrap()), m_module_name);
	m_dict->insert(RtValue::from_ptr(PyString::create("__doc__").unwrap()), m_doc);
	m_dict->insert(RtValue::from_ptr(PyString::create("__package__").unwrap()), m_package);
	m_dict->insert(RtValue::from_ptr(PyString::create("__loader__").unwrap()), m_loader);
	m_dict->insert(RtValue::from_ptr(PyString::create("__spec__").unwrap()), m_spec);
}

PyResult<PyObject *> PyModule::__repr__() const
{
	if (RuntimeContext::has_current() && RuntimeContext::current().has_interpreter()) {
		auto *importlib = RuntimeContext::current().interpreter()->importlib();
		if (importlib) {
			auto module_repr = importlib->get_method(PyString::create("_module_repr").unwrap());
			return module_repr.and_then([this](PyObject *obj) {
				return obj->call(PyTuple::create(const_cast<PyModule *>(this)).unwrap(), nullptr);
			});
		}
	}
	return PyString::create(fmt::format("<module {}>", m_module_name->value()));
}

PyResult<PyObject *> PyModule::__new__(const PyType *type, PyTuple *args, PyDict *kwargs)
{
	ASSERT(type == types::module());
	ASSERT(!kwargs || kwargs->map().empty());

	auto symbol_table = PyDict::create();
	if (symbol_table.is_err()) return symbol_table;

	auto *name = args->size() > 0 ? PyObject::from(args->elements()[0]).unwrap() : nullptr;
	auto *doc = args->size() > 1 ? PyObject::from(args->elements()[1]).unwrap() : py_none();
	if (!name) { TODO(); }
	if (!as<PyString>(name)) { TODO(); }

	return PyModule::create(symbol_table.unwrap(), as<PyString>(name), doc);
}

PyResult<int32_t> PyModule::__init__(PyTuple *args, PyDict *kwargs)
{
	ASSERT(args);
	ASSERT(!kwargs || kwargs->map().empty());

	auto *name = args->size() > 0 ? PyObject::from(args->elements()[0]).unwrap() : nullptr;
	auto *doc = args->size() > 1 ? PyObject::from(args->elements()[1]).unwrap() : py_none();
	if (!name) { TODO(); }
	if (!as<PyString>(name)) { TODO(); }

	m_module_name = as<PyString>(name);
	m_doc = doc;

	auto attr = PyDict::create();
	if (attr.is_err()) return Err(attr.unwrap_err());
	m_dict = attr.unwrap();
	m_dict = m_dict;

	// 使用 String key，与构造函数一致
	m_dict->insert(RtValue::from_ptr(PyString::create("__name__").unwrap()), m_module_name);
	m_dict->insert(RtValue::from_ptr(PyString::create("__doc__").unwrap()), m_doc);

	return Ok(0);
}

/// 添加符号并递增版本号
void PyModule::add_symbol(PyString *key, const Value &value)
{
	m_dict->insert(RtValue::from_ptr(key), value);
	// 粗粒度: 全局 dict version 递增
	m_dict_version = s_global_version.fetch_add(1, std::memory_order_relaxed);
	// 细粒度: 该 key 的 version 递增
	m_key_versions.bump(key->value());
}

namespace {
	bool is_initializing(PyObject *spec)
	{
		auto _initializing_str = PyString::create("_initializing");
		ASSERT(_initializing_str.is_ok());
		auto value = spec->get_attribute(_initializing_str.unwrap());
		if (value.is_err()) { return false; }
		if (RuntimeContext::has_current()) {
			return RuntimeContext::current().is_true(value.unwrap());
		}
		auto is_true = value.unwrap()->true_();
		if (is_true.is_ok()) { return is_true.unwrap(); }
		return false;
	}
}// namespace

PyResult<PyObject *> PyModule::__getattribute__(PyObject *attribute) const
{
	auto attr = PyObject::__getattribute__(attribute);
	if (attr.is_ok() || attr.unwrap_err()->type() != AttributeError::class_type()) { return attr; }

	if (m_dict) {
		if (auto it = m_dict->map().find(RtValue::from_ptr(attribute)); it != m_dict->map().end()) {
			return PyObject::from(it->second);
		}
	}

	if (auto it = m_dict->map().find(RtValue::from_ptr(PyString::create("__getattr__").unwrap()));
		it != m_dict->map().end()) {
		auto getattr = PyObject::from(it->second);
		ASSERT(getattr.is_ok());
		auto args = PyTuple::create(attribute);
		ASSERT(args.is_ok());
		return getattr.unwrap()->call(args.unwrap(), nullptr);
	} else if (auto it =
				   m_dict->map().find(RtValue::from_ptr(PyString::create("__name__").unwrap()));
		it != m_dict->map().end()) {
		auto module_name = PyObject::from(it->second);
		ASSERT(module_name.is_ok());
		if (auto name = as<PyString>(module_name.unwrap())) {
			if (auto it =
					m_dict->map().find(RtValue::from_ptr(PyString::create("__spec__").unwrap()));
				it != m_dict->map().end()) {
				auto spec = PyObject::from(it->second);
				ASSERT(spec.is_ok());
				if (is_initializing(spec.unwrap())) {
					return Err(
						attribute_error("partially initialized "
										"module '{}' has no attribute '{}' "
										"(most likely due to a circular import)",
							name->to_string(),
							attribute->to_string()));
				} else {
					return Err(attribute_error("module '{}' has no attribute '{}'",
						name->to_string(),
						attribute->to_string()));
				}
			}
		}
	}

	return Err(attribute_error("module has no attribute '{}'", attribute->to_string()));
}

void PyModule::visit_graph(Visitor &visitor)
{
	PyObject::visit_graph(visitor);
	if (m_module_name) visitor.visit(*m_module_name);
	if (m_doc) visitor.visit(*m_doc);
	if (m_package) visitor.visit(*m_package);
	if (m_loader) visitor.visit(*m_loader);
	if (m_spec) visitor.visit(*m_spec);
	if (m_dict) visitor.visit(*m_dict);
	if (m_module_context) { visitor.visit(*m_module_context); }
	if (m_program) { m_program->visit_functions(visitor); }
}

std::string PyModule::to_string() const
{
	return fmt::format("<module '{}'>", m_module_name->to_string());
}

PyResult<PyModule *> PyModule::create(PyDict *symbol_table, PyString *module_name, PyObject *doc)
{
	auto *result = PYLANG_ALLOC(PyModule, symbol_table, module_name, doc);
	if (!result) { return Err(memory_error(sizeof(PyModule))); }
	return Ok(result);
}

// PyResult<PyObject *> PyModule::find_symbol_cstr(const char *name) const
// {
//     if (!m_dict) {
//         return Err(name_error("name '{}' is not defined", name));
//     }
//     // 直接用 String key 查找，避免 PyString 堆分配
//     String key{ std::string(name) };
//     auto it = m_dict->map().find(key);
//     if (it != m_dict->map().end()) {
//         return PyObject::from(it->second);
//     }
//     return Err(name_error("name '{}' is not defined", name));
// }

PyResult<PyObject *> PyModule::find_symbol_cstr(const char *name) const
{
	if (!m_dict) {
		return Err(nullptr);// 返回空错误，不分配异常对象
	}
	// [性能优化] 使用 intern 代替 create，intern 有单字符无锁缓存，
	// 且避免 PyResult 包装开销。intern 保证返回驻留对象，指针稳定。
	auto *key = PyString::intern(name);
	if (!key) { return Err(nullptr); }
	auto it = m_dict->map().find(RtValue::from_ptr(key));
	if (it != m_dict->map().end()) { return PyObject::from(it->second); }
	return Err(nullptr);// 没找到直接返回 Err，由调用者决定是否抛出 NameError
}

/*
PyType *PyModule::static_type() const { return types::module(); }
*/

void PyModule::set_program(std::shared_ptr<Program> program) { m_program = std::move(program); }

const std::shared_ptr<Program> &PyModule::program() const { return m_program; }

namespace {

	std::once_flag module_flag;

	std::unique_ptr<TypePrototype> register_module()
	{
		return std::move(klass<PyModule>("module").attr("__dict__", &PyModule::m_dict).type);
	}
}// namespace


PyResult<std::monostate> PyModule::__setattribute__(PyObject *attribute, PyObject *value)
{
	if (!as<PyString>(attribute)) { return Err(type_error("attribute name must be string")); }
	if (m_dict) {
		m_dict->insert(RtValue::from_ptr(attribute), value);
		// invalidate cache
		m_dict_version = s_global_version.fetch_add(1, std::memory_order_relaxed);
		if (auto *s = as<PyString>(attribute)) m_key_versions.bump(s->value());
	}
	return Ok(std::monostate{});
}

PyResult<std::monostate> PyModule::__delattribute__(PyObject *attribute)
{
	if (!as<PyString>(attribute)) { return Err(type_error("attribute name must be string")); }
	if (m_dict) {
		if (auto it = m_dict->map().find(RtValue::from_ptr(attribute)); it != m_dict->map().end()) {
			m_dict->remove(attribute);
			m_dict_version = s_global_version.fetch_add(1, std::memory_order_relaxed);
			if (auto *s = as<PyString>(attribute)) m_key_versions.bump(s->value());
			return Ok(std::monostate{});
		}
	}
	return Err(attribute_error("can't delete attribute"));
}

std::function<std::unique_ptr<TypePrototype>()> PyModule::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(module_flag, []() { type = register_module(); });
		return std::move(type);
	};
}

template<> PyModule *as(PyObject *obj)
{
	if (obj->type() == types::module()) { return static_cast<PyModule *>(obj); }
	return nullptr;
}


template<> const PyModule *as(const PyObject *obj)
{
	if (obj->type() == types::module()) { return static_cast<const PyModule *>(obj); }
	return nullptr;
}
}// namespace py
