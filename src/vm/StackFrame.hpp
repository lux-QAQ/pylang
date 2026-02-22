#pragma once

#include "executable/FunctionBlock.hpp"
#include "forward.hpp"
#include "runtime/Value.hpp"
#include "utilities.hpp"

#include <memory>
#include <optional>
#include <span>
#include <stack>
#include <vector>

class VirtualMachine;

using Registers = std::vector<py::Value>;

struct State
{
	enum class CleanupLogic {
		CATCH_EXCEPTION,
		WITH_EXIT,
	};
	std::stack<std::optional<std::pair<CleanupLogic, InstructionVector::const_iterator>>> cleanup{
		{ std::nullopt }
	};
};

struct StackFrame : NonCopyable
{
  private:
	StackFrame() = default;

	StackFrame(size_t register_count,
		size_t locals_count,
		size_t stack_size,
		InstructionVector::const_iterator return_address,
		VirtualMachine *);

	StackFrame(StackFrame &&);

  public:
	template<typename... Args> static std::unique_ptr<StackFrame> create(Args &&...args)
	{
		return std::unique_ptr<StackFrame>(new StackFrame{ std::forward<Args>(args)... });
	}

	Registers registers;
	std::vector<py::Value> locals_storage;
	std::span<py::Value> locals;
	InstructionVector::const_iterator return_address;
	InstructionVector::const_iterator last_instruction_pointer;
	std::vector<py::Value>::const_iterator base_pointer;
	std::vector<py::Value>::iterator stack_pointer;
	VirtualMachine *vm{ nullptr };
	std::unique_ptr<State> state;

	~StackFrame();

	StackFrame clone() const;

	StackFrame &restore();
	void leave();
};