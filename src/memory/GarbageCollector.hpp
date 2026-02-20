#pragma once

#include "utilities.hpp"

#include <bitset>
#include <stack>
#include <string>

// 前置声明: Arena 类, 供 friend class ::py::Arena 使用
namespace py { class Arena; }

// =============================================================================
// Arena 模式下, GarbageCollected / Cell / MarkSweepGC 全部禁用。
// PyObject 不再继承 Cell, 而是独立的基类。
// =============================================================================

#ifndef PYLANG_USE_ARENA

class GarbageCollected
{
  public:
	enum class Color {
		WHITE,
		GREY,
		BLACK,
	};

	bool black() const { return m_state.all(); }

	bool grey() const { return m_state[1] == 1; }

	bool white() const { return m_state.none(); }

	void mark(Color color)
	{
		if (color == Color::WHITE) {
			m_state.reset();
		} else if (color == Color::GREY) {
			m_state[0] = 0;
			m_state[1] = 1;
		} else {
			m_state.set();
		}
	}

  private:
	std::bitset<2> m_state{ 0b00 };
};

class Cell
	: NonCopyable
	, NonMoveable
{
  public:
	struct Visitor
	{
		virtual ~Visitor() = default;
		virtual void visit(Cell &) = 0;
	};

  public:
	Cell() = default;
	virtual ~Cell() = default;

	virtual std::string to_string() const = 0;
	virtual void visit_graph(Visitor &) = 0;

	virtual bool is_pyobject() const { return false; }
};


class Heap;

class GarbageCollector
{
  public:
	virtual ~GarbageCollector() = default;
	virtual void run(Heap &) const = 0;
	virtual void resume() = 0;
	virtual void pause() = 0;
	virtual bool is_active() const = 0;

	void remove_weakref(Heap &heap, uint8_t *obj) const;

	void set_frequency(size_t new_frequency) { m_frequency = new_frequency; }

  protected:
	size_t m_frequency;
};

class MarkSweepGC : public GarbageCollector
{
  public:
	MarkSweepGC();
	void run(Heap &) const override;
	void resume() override;
	void pause() override;
	bool is_active() const override;

	std::stack<Cell *> collect_roots(const Heap &) const;
	void mark_all_cell_unreachable(Heap &) const;
	void mark_all_live_objects(Heap &, std::stack<Cell *> &&) const;
	void sweep(Heap &heap) const;

  private:
	mutable uint8_t *m_stack_bottom{ nullptr };
	mutable size_t m_iterations_since_last_sweep{ 0 };
	bool m_pause{ false };
};

#else // PYLANG_USE_ARENA

// =============================================================================
// Arena 模式: Cell 替代品
// =============================================================================
// PyObject 仍然需要 to_string() 接口, 但不需要 visit_graph / GC 标记。
// 提供一个最小化的 Cell 替代基类, 保持 NonCopyable/NonMoveable 语义。

class Cell
	: NonCopyable
	, NonMoveable
{
  public:
	struct Visitor
	{
		virtual ~Visitor() = default;
		virtual void visit(Cell &) = 0;
	};

  public:
	Cell() = default;
	virtual ~Cell() = default;

	virtual std::string to_string() const = 0;

	// Arena 模式下 visit_graph 为空操作
	virtual void visit_graph(Visitor &) {}

	virtual bool is_pyobject() const { return false; }
};

#endif // PYLANG_USE_ARENA
