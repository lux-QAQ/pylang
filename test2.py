# 这个简单的例子就足以触发 GC_MALLOC_ATOMIC 导致的 UAF 崩溃
# 原因分析见下方

test = str(123123213231232334124414141*123123213231232334124414141*123123213231232334124414141)
print(test)

# === 为什么这两行就会崩？===
#
# 执行流程:
#   1. str(123) 调用 PyString.__new__ → 需要查找 builtins 中的 "str" 类型
#   2. rt_load_global(module, "str") → 访问 PyModule 的 __dict__ (PyDict)
#   3. PyDict 是通过 Arena::allocate<PyDict>() 分配的
#   4. PyDict 没有 GCLayout 特化 → 走 fallback → GC_MALLOC_ATOMIC
#   5. PyDict 内部持有 PyObject* 指针（键值对引用其他对象）
#   6. GC 标记阶段：因为 ATOMIC，GC 不扫描 PyDict 内部
#   7. PyDict 引用的 PyType("str"), PyFunction("print") 等对象
#      看起来"没人引用" → 被 GC 回收！
#   8. 下次访问 str 类型或调用 print 时 → 访问已释放内存 → 段错误
#
# 更具体的崩溃链:
#
#   PyModule (builtins)
#     └─ m_attributes: PyDict*  ← ATOMIC 分配，GC 不扫描！
#          ├─ "str"   → PyType*  ← GC 认为没人引用，回收！
#          ├─ "print" → PyNativeFunction*  ← 同上，回收！
#          ├─ "int"   → PyType*  ← 同上
#          └─ ...
#
#   当 GC 触发后:
#     str(123)  → 访问已回收的 PyType("str") → SEGFAULT / ASAN: use-after-free
#     print(x)  → 访问已回收的 PyNativeFunction → SEGFAULT
#
# === 更隐蔽的场景 ===
#
# 即使上面侥幸没崩（GC 还没触发），以下代码必崩:

# 制造足够的内存压力迫使 GC 触发
big_list = []
for i in range(100000):
    big_list.append(str(i))  # 大量分配，触发 GC

# GC 触发后，PyList 内部的 PyObject* 数组因为 ATOMIC 不被扫描
# big_list 引用的 10 万个 PyString 全部被回收
print(big_list[0])   # ← use-after-free, 必崩
print(big_list[-1])  # ← 同上

# === 结论 ===
# GC_MALLOC_ATOMIC 作为 fallback 是致命的。
# 任何包含 PyObject* 指针的类（PyDict, PyList, PyModule, PyFunction,
# PyType, PyFrame, PyCode, PyGenerator...）都必须被 GC 扫描。
# 
# 正确做法: fallback 必须是 GC_MALLOC（保守扫描），
# 然后逐步为高频类添加 GCLayout 特化来精确优化。