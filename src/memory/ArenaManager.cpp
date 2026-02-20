#include "ArenaManager.hpp"

namespace py {

// thread_local 定义
thread_local std::unique_ptr<Arena> ArenaManager::t_thread_arena = nullptr;

}// namespace py
