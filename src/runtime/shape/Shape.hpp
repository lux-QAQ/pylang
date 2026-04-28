#pragma once

#include "runtime/forward.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace py {

class Shape
{
	Shape *m_parent{ nullptr };
	PyType *m_owner_type{ nullptr };
	PyString *m_property_name{ nullptr };
	size_t m_slot_count{ 0 };
	uint64_t m_version{ 0 };

	// O(1) 属性名 -> slot offset
	std::unordered_map<PyString *, size_t> m_offsets;
	// 共享 hidden-class 转移边
	std::unordered_map<PyString *, Shape *> m_transitions;

	Shape(PyType *owner_type, uint64_t version);
	Shape(Shape *parent, PyString *name, size_t offset, uint64_t version);

  public:
	static Shape *root(PyType *owner_type);

	size_t slot_count() const { return m_slot_count; }
	uint64_t version() const { return m_version; }
	PyType *owner_type() const { return m_owner_type; }
	const std::unordered_map<PyString *, size_t> &offsets() const { return m_offsets; }

	Shape *transition(PyString *name);
	std::optional<size_t> lookup(PyString *name) const;
};

}// namespace py
