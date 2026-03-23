#pragma once

#include "MemoryError.hpp"
#include "PyObject.hpp"
#include "PyTuple.hpp"
#include "executable/Program.hpp"
#include "runtime/compat.hpp"


namespace py {
class PyFunction : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	PyString *m_name{ nullptr };
	PyObject *m_doc{ nullptr };
	PyCode *m_code{ nullptr };
	PyObject *m_globals{ nullptr };
	PyDict *m_dict{ nullptr };
	const std::vector<Value> m_defaults;
	const std::vector<Value> m_kwonly_defaults;
	PyTuple *m_closure{ nullptr };
	PyObject *m_module{ nullptr };
	PyString *m_qualname{ nullptr };

	PyFunction(PyType *);

  public:
	PyFunction(std::vector<Value> defaults,
		std::vector<Value> kwonly_defaults,
		PyCode *code,
		PyTuple *closure,
		PyObject *globals);

	const PyCode *code() const { return m_code; }
	PyCode *code() { return m_code; }

	std::string to_string() const override;
	const std::vector<Value> &defaults() const { return m_defaults; }
	const std::vector<Value> &kwonly_defaults() const { return m_kwonly_defaults; }

	PyResult<PyObject *> call_with_frame(PyObject *ns, PyTuple *args, PyDict *kwargs) const;

	PyResult<PyObject *> __call__(PyTuple *args, PyDict *kwargs);
	PyString *function_name() const { return m_name; }

	PyObject *globals() const { return m_globals; }

	/// 获取闭包 cell 元组（AOT 编译器和 get_closure 导出使用）
	PyTuple *closure() const { return m_closure; }

	PyResult<PyObject *> __repr__() const;
	PyResult<PyObject *> __get__(PyObject *instance, PyObject *owner) const;

	void visit_graph(Visitor &) override;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};


class PyNativeFunction : public PyBaseObject
{

	/// AOT 编译后的函数指针类型（无闭包）
	/// 签名: PyObject*(PyObject* module, PyTuple* args, PyDict* kwargs)
	using AOTFuncPtr = py::PyObject *(*)(py::PyObject *, py::PyTuple *, py::PyDict *);

	/// AOT 编译后的函数指针类型（有闭包）
	/// 签名: PyObject*(PyObject* module, PyObject* closure, PyTuple* args, PyDict* kwargs)
	using AOTClosureFuncPtr = py::PyObject *(*)(py::PyObject *,
		py::PyObject *,
		py::PyTuple *,
		py::PyDict *);

	// 签名: PyObject*(PyObject* module, PyObject* closure, const Value* args, int32_t argc, PyDict*
	// kwargs)
	using AOTRawFuncPtr =
		py::PyObject *(*)(py::PyObject *, py::PyObject *, const py::Value *, int32_t, py::PyDict *);
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;
	using FreeFunctionSignature = PyResult<PyObject *>(PyTuple *, PyDict *);
	using MethodSignature = PyResult<PyObject *>(PyObject *, PyTuple *, PyDict *);
	using FreeFunctionType = std::function<FreeFunctionSignature>;
	using MethodType = std::function<MethodSignature>;
	using FunctionType = std::variant<FreeFunctionType, MethodType>;

	using FreeFunctionPointerType = typename std::add_pointer_t<FreeFunctionSignature>;
	using MethodPointerType = typename std::add_pointer_t<MethodSignature>;

	std::string m_name;
	FunctionType m_function;
	PyObject *m_self{ nullptr };
	AOTRawFuncPtr m_aot_ptr{ nullptr };// 显式存储 AOT 原始指针
	std::vector<PyObject *> m_captures;

	// ---- AOT 编译器支持 (Phase 3.2) ----
	PyTuple *m_closure{ nullptr };// 闭包 cell 元组
	PyObject *m_module_ref{ nullptr };// __module__ 所属模块

	PyNativeFunction(PyType *);

	PyNativeFunction(std::string &&name, FunctionType &&function);

	// TODO: fix tracking of lambda captures
	template<typename... Args>
	PyNativeFunction(std::string &&name, FunctionType &&function, PyObject *self, Args &&...args)
		: PyNativeFunction(std::move(name), std::move(function))
	{
		m_self = self;
		m_captures = std::vector<PyObject *>{ std::forward<Args>(args)... };
	}

  public:
	template<typename... Args>
	static PyResult<PyNativeFunction *>
		create(std::string name, FreeFunctionType function, Args &&...args)
	{
		auto *result = PYLANG_ALLOC(PyNativeFunction,
			std::move(name),
			std::move(function),
			nullptr,
			std::forward<Args>(args)...);
		if (!result) { return Err(memory_error(sizeof(PyNativeFunction))); }
		return Ok(result);
	}

	template<typename... Args>
	static PyResult<PyNativeFunction *>
		create(std::string name, MethodType function, PyObject *self, Args &&...args)
	{

		auto *result = PYLANG_ALLOC(PyNativeFunction,
			std::move(name),
			std::move(function),
			self,
			std::forward<Args>(args)...);

		if (!result) { return Err(memory_error(sizeof(PyNativeFunction))); }
		return Ok(result);
	}
	PyResult<PyObject *> call_raw(std::span<const Value> args, PyDict *kwargs) override;
	// AOT 编译器工厂
	/// 从原生函数指针创建 Python 可调用对象
	///
	/// 闭包实现: closure 通过 lambda 捕获传给 AOT 函数，
	///   is_function() 返回 true（不是 method hack）
	///
	/// @param name       函数名（__name__）
	/// @param code_ptr   编译后的函数指针
	/// @param module     所属模块（__module__），可为 null
	/// @param defaults   默认值 PyTuple*，可为 null
	/// @param kwdefaults 关键字默认值 PyDict*，可为 null
	/// @param closure    闭包 cell 元组 PyTuple*，可为 null
	static PyResult<PyNativeFunction *> create_aot(std::string name,
		void *code_ptr,
		PyObject *module,
		PyObject *defaults,
		PyObject *kwdefaults,
		PyObject *closure);

	/// 获取闭包 cell 元组（AOT 和解释器统一接口）
	PyTuple *closure() const { return m_closure; }

	/// 获取所属模块
	PyObject *module_ref() const { return m_module_ref; }

	PyResult<PyObject *> operator()(PyTuple *args, PyDict *kwargs)
	{
		ASSERT(is_function());
		return std::get<FreeFunctionType>(m_function)(args, kwargs);
	}

	PyResult<PyObject *> operator()(PyObject *self, PyTuple *args, PyDict *kwargs)
	{
		ASSERT(is_method());
		return std::get<MethodType>(m_function)(self, args, kwargs);
	}

	bool is_function() const { return std::holds_alternative<FreeFunctionType>(m_function); }

	bool is_method() const { return std::holds_alternative<MethodType>(m_function); }

	std::optional<FreeFunctionPointerType> free_function_pointer()
	{
		if (!is_function()) { return std::nullopt; }
		ASSERT(std::get<FreeFunctionType>(m_function).target<FreeFunctionPointerType>());
		return *std::get<FreeFunctionType>(m_function).target<FreeFunctionPointerType>();
	}

	std::optional<MethodPointerType> method_pointer()
	{
		if (!is_method()) { return std::nullopt; }
		ASSERT(std::get<MethodType>(m_function).target<MethodPointerType>());
		return *std::get<MethodType>(m_function).target<MethodPointerType>();
	}

	std::string to_string() const override;

	const std::string &name() const { return m_name; }
	PyResult<PyObject *> __call__(PyTuple *args, PyDict *kwargs);
	PyResult<PyObject *> __repr__() const;

	// 添加描述符协议支持，供类方法绑定 (Method Binding) 使用
	PyResult<PyObject *> __get__(PyObject *instance, PyObject *owner) const;

	void visit_graph(Visitor &) override;

	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	// PyType *static_type() const override;;
};

}// namespace py
