#include "ClassBuilder.hpp"

#include "PyCell.hpp"
#include "PyDict.hpp"
#include "PyNone.hpp"
#include "PyString.hpp"
#include "PyTuple.hpp"
#include "PyType.hpp"
#include "TypeError.hpp"
#include "types/builtin.hpp"

namespace py {

PyResult<PyObject *>
	build_class_aot(PyObject *body_fn, const std::string &name, PyTuple *bases, PyDict *kwargs)
{
	// =================================================================
	// Step 1: 确定 metaclass
	//
	// 逻辑与 BuiltinsModule::build_class 完全一致:
	//   1. kwargs 中显式指定 metaclass=Meta
	//   2. type(bases[0])
	//   3. 默认 type
	// =================================================================
	bool metaclass_is_class = false;
	PyObject *metaclass = py_none();

	if (kwargs && !kwargs->map().empty()) {
		auto it = kwargs->map().find(RtValue::from_ptr(PyString::create("metaclass").unwrap()));
		if (it != kwargs->map().end()) {
			auto mc = PyObject::from(it->second);
			if (mc.is_err()) { return mc; }
			metaclass = mc.unwrap();
			if (metaclass->type()->issubclass(types::type())) { metaclass_is_class = true; }
		}
	}

	if (metaclass == py_none()) {
		if (bases->size() == 0) {
			metaclass = types::type();
		} else {
			auto base0 = PyObject::from(bases->elements()[0]);
			if (base0.is_err()) { return base0; }
			metaclass = base0.unwrap()->type();
		}
		metaclass_is_class = true;
	}

	// =================================================================
	// Step 2: 计算 most-derived metaclass
	//
	// 复用 PyType::calculate_metaclass — 与 build_class 完全一致
	// 这保证了多继承中 metaclass 冲突检测的正确性
	// =================================================================
	if (metaclass_is_class) {
		std::vector<PyType *> bases_vector;
		bases_vector.reserve(bases->size());
		for (size_t i = 0; i < bases->size(); ++i) {
			auto base_i = PyObject::from(bases->elements()[i]);
			if (base_i.is_err()) { return base_i; }
			if (auto *b = as<PyType>(base_i.unwrap())) {
				bases_vector.push_back(b);
			} else {
				return Err(type_error("bases must be types"));
			}
		}
		auto winner = PyType::calculate_metaclass(static_cast<PyType *>(metaclass), bases_vector);
		if (winner.is_err()) { return Err(winner.unwrap_err()); }
		metaclass = const_cast<PyType *>(winner.unwrap());
	}

	// =================================================================
	// Step 3: 创建 namespace
	//
	// 与 build_class 完全一致:
	//   - type 作为 metaclass → 直接创建空 dict
	//   - 自定义 metaclass → 调用 __prepare__(name, bases, **kwargs)
	// =================================================================
	auto class_name_ = PyString::create(name);
	if (class_name_.is_err()) { return Err(class_name_.unwrap_err()); }
	auto *class_name = class_name_.unwrap();

	auto ns_ = [metaclass, class_name, bases, kwargs]() -> PyResult<PyObject *> {
		if (metaclass == types::type()) {
			return PyDict::create();
		} else {
			auto prepare = PyString::create("__prepare__");
			if (prepare.is_err()) { return Err(prepare.unwrap_err()); }

			// 过滤掉 metaclass= 关键字
			PyDict *new_kwargs = nullptr;
			if (kwargs && !kwargs->map().empty()) {
				auto prepare_kwargs = kwargs->map();
				prepare_kwargs.erase(RtValue::from_ptr(PyString::create("metaclass").unwrap()));
				if (!prepare_kwargs.empty()) {
					auto kw = PyDict::create(prepare_kwargs);
					if (kw.is_err()) { return Err(kw.unwrap_err()); }
					new_kwargs = kw.unwrap();
				}
			}

			auto result = metaclass->lookup_attribute(prepare.unwrap());
			if (std::get<0>(result).is_ok() && std::get<1>(result) == LookupAttrResult::FOUND) {
				auto prepare_fn = std::get<0>(result).unwrap();
				auto args = PyTuple::create(class_name, bases);
				if (args.is_err()) { return Err(args.unwrap_err()); }
				return prepare_fn->call(args.unwrap(), new_kwargs);
			} else {
				return PyDict::create();
			}
		}
	}();

	if (ns_.is_err()) { return ns_; }
	auto *ns = ns_.unwrap();

	// =================================================================
	// Step 4: 预设 namespace 特殊属性
	// =================================================================
	if (auto *ns_dict = as<PyDict>(ns)) {
		ns_dict->insert(RtValue::from_ptr(PyString::create("__name__").unwrap()), class_name);
		ns_dict->insert(RtValue::from_ptr(PyString::create("__qualname__").unwrap()), class_name);
	}

	// =================================================================
	// Step 5: 调用类体函数
	//
	// *** 这是与 build_class 唯一的实质区别 ***
	//
	// build_class:     callable->call_with_frame(ns, empty_args, empty_kwargs)
	//                  → 帧 locals = ns → STORE_NAME 写入 ns
	//
	// build_class_aot: body_fn->call(PyTuple(ns), nullptr)
	//                  → ns 作为 args[0] → dict_setitem_str 写入 ns
	// =================================================================
	auto body_args = PyTuple::create(ns);
	if (body_args.is_err()) { return Err(body_args.unwrap_err()); }

	auto body_result = body_fn->call(body_args.unwrap(), nullptr);
	if (body_result.is_err()) { return body_result; }

	// =================================================================
	// Step 6: 创建类对象
	//
	// metaclass(name, bases, ns) → PyType::__call__ → __new__ →
	//   build_type → best_base + compute_bases + mro_() + ready()
	//
	// 完整支持 C3 MRO、多继承、描述器协议
	// =================================================================
	auto call_args = PyTuple::create(class_name, bases, ns);
	if (call_args.is_err()) { return Err(call_args.unwrap_err()); }

	auto cls = metaclass->call(call_args.unwrap(), nullptr);
	if (cls.is_err()) { return cls; }

	// =================================================================
	// Step 7: 处理 __classcell__
	//
	// 与 build_class 完全一致:
	//   如果类体创建了 __class__ cell 并存入 ns["__classcell__"]
	//   将 cell 内容设为新创建的类对象
	//   这使 super() 无参调用能正确引用 __class__
	// =================================================================
	if (auto *ns_dict = as<PyDict>(ns)) {
		auto *classcell_key = py::PyString::create("__classcell__").unwrap();
		if (auto it = ns_dict->get(classcell_key, nullptr); it.is_ok() && it.unwrap() != nullptr) {
			auto cell = it.unwrap();
			if (auto *cell_obj = as<PyCell>(cell)) {
				cell_obj->set_cell(py::RtValue::from_ptr(cls.unwrap()));
			}
		}
	}

	return cls;
}

}// namespace py