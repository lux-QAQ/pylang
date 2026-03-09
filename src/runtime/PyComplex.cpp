#include "runtime/PyComplex.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyType.hpp"
#include "runtime/types/api.hpp"

#include "runtime/compat.hpp"

using namespace py;

PyComplex::PyComplex(PyType *type) : PyBaseObject(type) {}

PyComplex::PyComplex(TypePrototype &type, std::complex<BigIntType> complex)
	: PyBaseObject(type), m_complex(std::move(complex))
{}

PyComplex::PyComplex(PyType *type, std::complex<BigIntType> complex)
	: PyBaseObject(type), m_complex(std::move(complex))
{}

PyType *PyComplex::static_type() const { return types::complex(); }

PyComplex::PyComplex(PyType *type, double real, double imag)
    : PyBaseObject(type),
      // std::complex<BigIntType> 的 real/imag 部分用 BigIntType 表示
      // 对于字面量 1+2j，double → long → BigIntType 精度足够
      m_complex(BigIntType(static_cast<long>(real)), BigIntType(static_cast<long>(imag)))
{}

namespace {

std::once_flag complex_flag;

std::unique_ptr<TypePrototype> register_complex()
{
	return std::move(klass<PyComplex>("complex").type);
}
}// namespace

std::function<std::unique_ptr<TypePrototype>()> PyComplex::type_factory()
{
	return [] {
		static std::unique_ptr<TypePrototype> type = nullptr;
		std::call_once(complex_flag, []() { type = register_complex(); });
		return std::move(type);
	};
}


PyResult<PyComplex *> PyComplex::create(double real, double imag)
{
    // 使用带 type 的构造函数，PYLANG_ALLOC 调用 PyComplex(PyType*, double, double)
    auto *obj = PYLANG_ALLOC(PyComplex, types::complex(), real, imag);
    if (!obj) return Err(memory_error(sizeof(PyComplex)));
    return Ok(obj);
}