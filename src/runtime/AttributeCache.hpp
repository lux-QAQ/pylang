#pragma once
#include "runtime/PyDict.hpp"
#include "runtime/PyModule.hpp"
#include "runtime/PyObject.hpp"
#include <cstdint>

namespace py {

// 哨兵：表示属性确定不存在
#define PYLANG_ATTR_ABSENT reinterpret_cast<py::PyObject *>(-1)

struct GlobalCache
{
    static constexpr size_t TABLE_SIZE = 256 *3; // 全局查找槽位
    struct Entry
    {
        PyModule *cached_mod;
        const char *name_ptr;
        PyObject *value;
        uint64_t mod_ver;
        uint64_t builtin_ver;
    };

    static inline size_t hash(PyModule *mod, const char *name) {
        return (reinterpret_cast<uintptr_t>(mod) ^ reinterpret_cast<uintptr_t>(name)) % TABLE_SIZE;
    }

    static PyObject *lookup(PyModule *mod, const char *name, PyDict *globals, PyDict *builtins)
    {
        static thread_local Entry table[TABLE_SIZE] = {};
        Entry &e = table[hash(mod, name)];
        
        if (e.cached_mod == mod && e.name_ptr == name) {
            if (globals->version() == e.mod_ver
                && (!builtins || builtins->version() == e.builtin_ver)) {
                return e.value;
            }
        }
        return nullptr;
    }

    static void update(PyModule *mod, const char *name, PyObject *val, uint64_t g_ver, uint64_t b_ver)
    {
        static thread_local Entry table[TABLE_SIZE];
        table[hash(mod, name)] = { mod, name, val, g_ver, b_ver };
    }
};

struct AttrCache
{
    static constexpr size_t TABLE_SIZE = 256 *3; // 类属性槽位
    struct Entry {
        PyType *type;
        const char *name;
        PyObject *attr;
        uint64_t version;
    };

    static inline size_t hash(PyType *type, const char *name) {
        // 指针偏移对齐，通过位移提高熵值
        return ((reinterpret_cast<uintptr_t>(type) >> 3) ^ reinterpret_cast<uintptr_t>(name)) % TABLE_SIZE;
    }

    static PyObject *lookup(PyType *type, const char *name, uint64_t gv)
    {
        static thread_local Entry table[TABLE_SIZE] = {};
        Entry &e = table[hash(type, name)];
        if (e.type == type && e.name == name && e.version == gv) {
            return e.attr;
        }
        return nullptr;
    }

    static void update(PyType *type, const char *name, PyObject *attr, uint64_t gv)
    {
        static thread_local Entry table[TABLE_SIZE];
        table[hash(type, name)] = { type, name, attr, gv };
    }
};

}// namespace py