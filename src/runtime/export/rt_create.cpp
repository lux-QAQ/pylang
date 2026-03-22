#include "rt_common.hpp"

#include "runtime/PyBytes.hpp"
#include "runtime/PyComplex.hpp"
#include "runtime/PyDict.hpp"
#include "runtime/PyFloat.hpp"
#include "runtime/PyInteger.hpp"
#include "runtime/PyList.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PySet.hpp"
#include "runtime/PySlice.hpp"
#include "runtime/PyString.hpp"
#include "runtime/PyTuple.hpp"
#include "runtime/TypeError.hpp"
#include "runtime/taggered_pointer/RtValue.hpp"
#include "runtime/types/builtin.hpp"

// =============================================================================
// Tier 0: 对象创建
// =============================================================================

PYLANG_EXPORT_CREATE("string_from_cstr", "obj", "str,i64")
py::PyObject *rt_string_from_cstr(const char *data, int64_t length)
{
	std::string str(data, static_cast<size_t>(length));
	return rt_unwrap(py::PyString::create(str));
}

PYLANG_EXPORT_CREATE("build_tuple", "obj", "i32,ptr")
py::PyObject *rt_build_tuple(int32_t count, py::PyObject **items)
{
	std::vector<py::Value> elements;
	elements.reserve(static_cast<size_t>(count));
	for (int32_t i = 0; i < count; ++i) { elements.push_back(py::ensure_box(items[i])); }
	return rt_unwrap(py::PyTuple::create(elements));
}

// =============================================================================
// Tier 1: 更多对象创建
// =============================================================================

PYLANG_EXPORT_CREATE("integer_from_i64", "obj", "i64")
py::PyObject *rt_integer_from_i64(int64_t value)
{
	return py::RtValue::from_int_or_box(value).as_pyobject_raw();
}

PYLANG_EXPORT_CREATE("float_from_f64", "obj", "f64")
py::PyObject *rt_float_from_f64(double value) { return rt_unwrap(py::PyFloat::create(value)); }

PYLANG_EXPORT_CREATE("build_list", "obj", "i32,ptr")
py::PyObject *rt_build_list(int32_t count, py::PyObject **items)
{
	std::vector<py::Value> elements;
	elements.reserve(static_cast<size_t>(count));
	for (int32_t i = 0; i < count; ++i) { elements.push_back(py::ensure_box(items[i])); }
	return rt_unwrap(py::PyList::create(elements));
}

PYLANG_EXPORT_CREATE("integer_from_string", "obj", "str")
py::PyObject *rt_integer_from_string(const char *str)
{
	mpz_class value(str, 10);
	return rt_unwrap(py::PyInteger::create(std::move(value)));
}

// =============================================================================
// Tier 3: 更多容器创建
// =============================================================================

PYLANG_EXPORT_CREATE("build_dict", "obj", "i32,ptr,ptr")
py::PyObject *rt_build_dict(int32_t count, py::PyObject **keys, py::PyObject **values)
{
	if (count == 0) { return rt_unwrap(py::PyDict::create()); }
	auto dict = py::PyDict::create();
	if (dict.is_err()) { rt_raise(dict.unwrap_err()); }

	auto *dict_obj = dict.unwrap();
	for (int32_t i = 0; i < count; ++i) {
		dict_obj->insert(py::ensure_box(keys[i]), py::ensure_box(values[i]));
	}

	return dict_obj;
}

PYLANG_EXPORT_CREATE("build_set", "obj", "i32,ptr")
py::PyObject *rt_build_set(int32_t count, py::PyObject **items)
{
	py::PySet::SetType elements;
	elements.reserve(static_cast<size_t>(count));

	for (int32_t i = 0; i < count; ++i) { elements.insert(py::ensure_box(items[i])); }

	return rt_unwrap(py::PySet::create(std::move(elements)));
}

PYLANG_EXPORT_CREATE("build_slice", "obj", "obj,obj,obj")
py::PyObject *rt_build_slice(py::PyObject *start, py::PyObject *stop, py::PyObject *step)
{
	return rt_unwrap(
		py::PySlice::create(py::ensure_box(start), py::ensure_box(stop), py::ensure_box(step)));
}

// =============================================================================
// Tier 1: 字节字面量 / 复数字面量
// =============================================================================

PYLANG_EXPORT_CREATE("bytes_from_buffer", "obj", "str,i64")
py::PyObject *rt_bytes_from_buffer(const char *data, int64_t length)
{
	std::vector<std::byte> bytes(static_cast<size_t>(length));
	std::memcpy(bytes.data(), data, static_cast<size_t>(length));
	return rt_unwrap(py::PyBytes::create(py::Bytes{ std::move(bytes) }));
}

PYLANG_EXPORT_CREATE("complex_from_doubles", "obj", "f64,f64")
py::PyObject *rt_complex_from_doubles(double real, double imag)
{
	return rt_unwrap(py::PyComplex::create(real, imag));
}