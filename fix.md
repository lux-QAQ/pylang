经过对 PyObject.cpp、PyType.cpp、PyProperty.cpp 的深入交叉分析，我发现以下问题：

---

## 🔴 Critical Bug 1: `PyObject::new_` — `resize` vs `reserve`

**仍未修复！** 这是当前 Properties 测试失败的直接原因。

````cpp
// ...existing code...
	// pop out type from args
	std::vector<Value> new_args;
	// [FIX] resize → reserve
	new_args.reserve(args->size() - 1);
	new_args.insert(new_args.end(), args->elements().begin() + 1, args->elements().end());
// ...existing code...
````

---

## 🔴 Critical Bug 2: `PyProperty::__new__` — 错误的参数索引

`fdel` 和 `doc` 都使用了错误的索引 `[1]`。

````cpp
// ...existing code...
	auto fdel = [&]() -> PyResult<PyObject *> {
		if (args && args->size() >= 3) {
			// [FIX] args[1] → args[2]
			return PyObject::from(args->elements()[2]);
		} else if (kwargs) {
			if (auto it = kwargs->map().find(String{ "fdel" }); it != kwargs->map().end()) {
				return PyObject::from(it->second);
			}
		}
		return Ok(py_none());
	}();
	if (fdel.is_err()) { return fdel; }

	auto doc = [&]() -> PyResult<PyObject *> {
		// [FIX] size >= 3 → size >= 4, args[1] → args[3]
		if (args && args->size() >= 4) {
			return PyObject::from(args->elements()[3]);
		} else if (kwargs) {
			if (auto it = kwargs->map().find(String{ "doc" }); it != kwargs->map().end()) {
				return PyObject::from(it->second);
			}
		}
		return Ok(py_none());
	}();
// ...existing code...
````

---

## 🔴 Critical Bug 3: `PyProperty::deleter` — 错用 `m_setter`

````cpp
// ...existing code...
PyResult<PyObject *> PyProperty::deleter(PyTuple *args, PyDict *kwargs) const
{
	ASSERT(!kwargs || kwargs->map().empty());
	ASSERT(args);
	ASSERT(args->size() == 1);

	auto deleter_ = PyObject::from(args->elements()[0]);

	if (deleter_.is_err()) return deleter_;

	// [FIX] 第一个参数应是 m_getter，不是 m_setter
	return PyProperty::create(m_getter == py_none() ? py_none() : m_getter,
		m_setter == py_none() ? py_none() : m_setter,
		deleter_.unwrap(),
		m_property_name);
}
// ...existing code...
````

---

## 🟡 Important Bug 4: `PyObject::init` — `self` 未 prepend 到 args

`PyType::__call__` 调用 `obj->init(args, kwargs)` 时，`args` 是原始构造参数（不含 `self`）。`PyObject::init` 通过 `type()->lookup("__init__")` 找到的是原始 `PyFunction`（不经过描述符协议），所以不会自动绑定 `self`。

AOT 编译的 `__init__` 期望 `args = (self, ...)` 但实际收到 `args = (...)`。

**修复 `PyObject::init`**：

````cpp
// ...existing code...
PyResult<int32_t> PyObject::init(PyTuple *args, PyDict *kwargs)
{
	// Python 3.9 语义: 优先在 MRO 字典中动态查找 __init__
	auto init_str_ = PyString::create("__init__");
	if (init_str_.is_err()) { return Err(init_str_.unwrap_err()); }
	auto *init_str = init_str_.unwrap();

	auto descriptor_ = type()->lookup(init_str);
	if (descriptor_.has_value() && descriptor_->is_ok()) {
		auto *descriptor = descriptor_->unwrap();

		// [FIX] 尝试描述符协议: __get__(self, type) → BoundMethod
		// 这样调用时 self 会自动绑定
		if (descriptor->type_prototype().__get__.has_value()) {
			auto bound = descriptor->get(this, type());
			if (bound.is_ok()) {
				auto result = bound.unwrap()->call(args, kwargs);
				if (result.is_err()) { return Err(result.unwrap_err()); }
				// __init__ 应返回 None，我们返回 0 表示成功
				return Ok(0);
			}
		}

		// Fallback: 手动 prepend self 到 args
		std::vector<Value> init_args;
		init_args.reserve(1 + (args ? args->size() : 0));
		init_args.push_back(this);
		if (args) {
			init_args.insert(init_args.end(),
				args->elements().begin(),
				args->elements().end());
		}
		auto init_tuple = PyTuple::create(init_args);
		if (init_tuple.is_err()) { return Err(init_tuple.unwrap_err()); }

		auto result = descriptor->call(init_tuple.unwrap(), kwargs);
		if (result.is_err()) { return Err(result.unwrap_err()); }
		return Ok(0);
	}

	// 回退原生 C++ slot
	if (type_prototype().__init__.has_value()) {
		return call_slot(*type_prototype().__init__, this, args, kwargs);
	}

	return Ok(0);
}
// ...existing code...
````

---

## 🟡 Important Bug 5: `richcompare` 缺少动态 fallback

`eq`, `lt`, `gt`, `le`, `ge`, `ne` 都只检查 C++ slot，不检查用户定义的 `__eq__` 等。对于 AOT 编译的类（如定义了 `__eq__`），比较操作会失败。

````cpp
// ...existing code...
PyResult<PyObject *> PyObject::eq(const PyObject *other) const
{
	if (this == other) { return Ok(py_true()); }
	bool checked_reverse_op = false;
	if (type() != other->type() && other->type()->issubclass(type())
		&& other->type()->underlying_type().__eq__.has_value()) {
		auto result = call_slot(*other->type()->underlying_type().__eq__, other, this);
		if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }
		checked_reverse_op = true;
	}
	if (type_prototype().__eq__.has_value()) {
		auto result = call_slot(*type_prototype().__eq__, this, other);
		if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }
	}

	// [FIX] Dynamic fallback for user-defined __eq__
	auto fb = binary_op_fallback(this, other, "__eq__");
	if (fb.is_ok() && fb.unwrap() != not_implemented()) { return fb; }

	if (!checked_reverse_op && other->type()->underlying_type().__eq__.has_value()) {
		auto result = call_slot(*other->type()->underlying_type().__eq__, other, this);
		if (result.is_ok() && result.unwrap() != not_implemented()) { return result; }
	}
	return Ok(this == other ? py_true() : py_false());
}
// ...existing code...
````

对 `lt`, `le`, `gt`, `ge`, `ne` 同理添加 `binary_op_fallback`。

---

## 🟡 Important Bug 6: `repr`/`str`/`iter`/`next`/`hash` 缺少动态 fallback

与二元运算符同理，这些方法也需要 fallback 到动态查找：

````cpp
// ...existing code...
PyResult<PyString *> PyObject::repr() const
{
	if (type_prototype().__repr__.has_value()) {
		return call_slot(*type_prototype().__repr__, this).and_then(
			[](PyObject *obj) -> PyResult<PyString *> {
				if (auto *s = as<PyString>(obj)) { return Ok(s); }
				return Err(type_error("__repr__ returned non-string"));
			});
	}

	// [FIX] Dynamic fallback
	auto name = PyString::create("__repr__");
	if (name.is_ok()) {
		auto [method, found] = lookup_attribute(name.unwrap());
		if (found == LookupAttrResult::FOUND && method.is_ok()) {
			auto res = method.unwrap()->call(PyTuple::create().unwrap(), nullptr);
			if (res.is_ok()) {
				if (auto *s = as<PyString>(res.unwrap())) { return Ok(s); }
				return Err(type_error("__repr__ returned non-string"));
			}
			return Err(res.unwrap_err());
		}
	}

	return __repr__();
}
// ...existing code...
````

---

## 汇总

| 优先级 | 位置 | 问题 | 影响 |
|--------|------|------|------|
| 🔴 | `PyObject::new_` | `resize` → `reserve` | 所有带参构造崩溃 |
| 🔴 | `PyProperty::__new__` | `fdel`/`doc` 索引错误 | property 构造参数错位 |
| 🔴 | `PyProperty::deleter` | `m_setter` → `m_getter` | deleter 装饰器丢失 getter |
| 🟡 | `PyObject::init` | `self` 未 prepend | AOT 类 `__init__` 崩溃 |
| 🟡 | `eq/lt/gt/...` | 缺动态 fallback | 用户定义比较操作符失效 |
| 🟡 | `repr/str/iter/next/hash` | 缺动态 fallback | 用户定义魔术方法失效 |

**建议修复顺序**: Bug 1 → Bug 4 → Bug 2 → Bug 3 → Bug 5 → Bug 6