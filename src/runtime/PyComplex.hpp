#pragma once

#include "forward.hpp"
#include "runtime/PyObject.hpp"
#include "runtime/PyType.hpp"
#include "runtime/Value.hpp"

#include <complex>

namespace py {

class PyComplex : public PyBaseObject
{
#ifndef PYLANG_USE_ARENA
	friend class ::Heap;
#endif
	friend class ::py::Arena;

	std::complex<BigIntType> m_complex;

  protected:
	PyComplex(PyType *);
	PyComplex(TypePrototype &type, std::complex<BigIntType> complex);
	PyComplex(PyType *type, std::complex<BigIntType> complex);

	// 直接从两个 double 构造（用于字面量快速路径）
	PyComplex(PyType *type, double real, double imag);

  public:
	static std::function<std::unique_ptr<TypePrototype>()> type_factory();
	PyType *static_type() const override;

	/// AOT 快速路径：直接从两个 double 创建
	static PyResult<PyComplex *> create(double real, double imag);
};

}// namespace py
