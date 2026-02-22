#include "runtime/PyModule.hpp"

namespace py {

// 重构后: 所有模块工厂函数统一为无参数签名
// builtins/sys 内部通过 RuntimeContext 获取所需信息
PyModule *builtins_module();
PyModule *codecs_module();
PyModule *collections_module();
PyModule *errno_module();
PyModule *imp_module();
PyModule *io_module();
PyModule *math_module();
PyModule *marshal_module();
PyModule *posix_module();
PyModule *thread_module();
PyModule *weakref_module();
PyModule *warnings_module();
PyModule *itertools_module();
PyModule *signal_module();
PyModule *sre_module();
PyModule *struct_module();
PyModule *sys_module();
PyModule *time_module();
}// namespace py
