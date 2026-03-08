rt_common.hpp  ← 公共头文件（错误处理宏）
    ↓
rt_error.cpp   ← rt_raise（所有包装函数的错误出口）
    ↓
rt_singleton.cpp ← 最简单，无依赖
    ↓
rt_lifecycle.cpp ← rt_init（运行时初始化）
    ↓
rt_create.cpp  ← string_from_cstr, build_tuple（Tier 0 必需）
    ↓
rt_attr.cpp    ← getattr, load_global
    ↓
rt_func.cpp    ← call
    ↓
rt_module.cpp  ← import