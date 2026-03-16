## 深度分析：`len()` 的正确修复方式

### 核心发现：你的 `TypePrototype::create` 模板已经解决了这个问题

阅读 PyObject.hpp 中 `TypePrototype::create` 的模板代码，关键在这里：

```cpp
if constexpr (HasLength<Type>) {
    if (!type_prototype->mapping_type_protocol.has_value()) {
        type_prototype->mapping_type_protocol =
            MappingTypePrototype{ .__len__ = ... };
    } else {
        type_prototype->mapping_type_protocol->__len__ = ...;
    }
}
```

**`HasLength<T>` 检测到 `__len__()` 方法后，始终注册到 `mapping_type_protocol->__len__`。** 从来不会注册到 `sequence_type_protocol->__len__`。

这意味着：**原有的 `BuiltinsModule::len()` 调用 `as_mapping()` 在架构上就是正确的。** 你的 dict 视图类型之所以报错，仅仅是因为它们缺少 `__len__()` C++ 方法声明，导致 `HasLength<PyDictKeys>` 为 false，`mapping_type_protocol` 没被创建。

### CPython 3.9 的 `PyObject_Size` 实际实现

```c
// Objects/abstract.c — CPython 3.9
Py_ssize_t PyObject_Size(PyObject *o)
{
    // 1. 先尝试 tp_as_mapping->mp_length
    m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_length)
        return m->mp_length(o);

    // 2. 再尝试 tp_as_sequence->sq_length
    m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_length)
        return m->sq_length(o);

    // 3. TypeError
    return type_error("object of type '%.200s' has no len()", o);
}
```

**CPython 的顺序是 mapping 优先 → sequence 回退。** 你的原始代码 `as_mapping()` 恰好就是这个顺序。

### 最终方案：最小改动，不破坏架构

**`BuiltinsModule::len()` 不需要大改**，只需要补一个 sequence fallback 即可完整覆盖 CPython 语义。`PyObject::len()` 可以保留作为抽象 API 层，但要修正优先级顺序：

````cpp
// ...existing code...
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
// ...existing code...
````

````cpp
// ...existing code...
PyResult<PyObject *> len(const PyTuple *args, const PyDict *kwargs)
{
	if (args->size() != 1) {
		return Err(type_error("len() takes exactly one argument ({} given)", args->size()));
	}
	if (kwargs && !kwargs->map().empty()) {
		return Err(type_error("len() takes no keyword arguments"));
	}

	return PyObject::from(args->elements()[0]).and_then([](PyObject *o) -> PyResult<PyObject *> {
		return o->len().and_then([](size_t length) -> PyResult<PyObject *> {
			return PyInteger::create(static_cast<int64_t>(length));
		});
	});
}
// ...existing code...
````

### 为什么这个方案不会破坏架构

| 层级 | 职责 | 类比 CPython |
|------|------|-------------|
| `PyObject::len()` | 抽象对象 API，协议分发 | `PyObject_Size()` in `abstract.c` |
| `BuiltinsModule::len()` | 参数校验 + 委托 | `builtin_len()` in `bltinmodule.c` |
| `TypePrototype::create` | 自动注册 `__len__` 到 `mapping_type_protocol` | `type_new()` 中的 slot 填充 |
| `PyDictKeys::__len__()` | 具体类型实现 | `dictkeys_len()` |

**不需要为每种类硬编码语义**——`TypePrototype::create` 的 `if constexpr` 模板已经自动处理了注册。你只需要确保每个需要 `len()` 的类型声明了 `__len__()` 方法即可。

现在重新编译测试：

```bash
cd /home/lux/code/language/python-cpp/build/debug
cmake --build . -j $(nproc)
rm -f /tmp/pylang_runtime_cache.o /tmp/pylang_*
./src/pyc ../../test.py
/tmp/test
```