#include "Arena.hpp"

namespace py {

// thread_local 定义
thread_local Arena *Arena::t_current_arena = nullptr;

}// namespace py
