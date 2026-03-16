#include "interpreter/InterpreterCore.hpp"
#include "vm/VM.hpp"
#ifdef PYLANG_USE_ARENA
#include "memory/ArenaManager.hpp"
#include "runtime/builtinTypeInit.hpp"
#endif
#ifdef PYLANG_USE_Boehm_GC
#include <gc.h>
#endif

#include "gtest/gtest.h"

#include <cxxopts.hpp>

class PythonVMEnvironment : public ::testing::Environment
{
	char **m_argv [[maybe_unused]];

  public:
	PythonVMEnvironment(char **argv) : m_argv(argv) {}
	~PythonVMEnvironment() override {}

	void SetUp() override
	{
#ifdef PYLANG_USE_ARENA
		py::ArenaManager::initialize();

#ifdef PYLANG_USE_Boehm_GC
		GC_INIT();
		GC_allow_register_threads();
#endif
#else
		[[maybe_unused]] auto &vm = VirtualMachine::the();
		vm.heap().set_start_stack_pointer(bit_cast<uintptr_t *>(m_argv));
#endif
		py::initialize_types();
	}

	void TearDown() override
	{
#ifdef PYLANG_USE_ARENA
		py::ArenaManager::shutdown();
#endif
	}
};

int main(int argc, char **argv)
{


	::testing::InitGoogleTest(&argc, argv);

	(void)AddGlobalTestEnvironment(new PythonVMEnvironment(argv));

	cxxopts::Options options("python-testing", "The C++ Python interpreter tests");

	// clang-format off
	options.add_options()
		("d,debug", "Enable debug logging", cxxopts::value<bool>()->default_value("false"))
		("trace", "Enable trace logging", cxxopts::value<bool>()->default_value("false"))
		("h,help", "Print usage");

	auto result = options.parse(argc, argv);

	if (result.count("help")) {
		std::cout << options.help() << std::endl;
		return EXIT_SUCCESS;
	}

	const bool debug = result["debug"].as<bool>();
	const bool trace = result["trace"].as<bool>();
	if (debug) { spdlog::set_level(spdlog::level::debug); }
	if (trace) { spdlog::set_level(spdlog::level::trace); }

	auto ret =RUN_ALL_TESTS();



return ret;
}