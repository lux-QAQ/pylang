#pragma once

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace py {

class ModuleMangler
{
  public:
    /// 模块逻辑名 → 链接器符号名
    /// "foo.bar" → "PyInit_foo_2E_bar"
    static std::string mangle(const std::string &module_name)
    {
        std::ostringstream os;
        os << "PyInit_";
        for (char c : module_name) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                os << c;
            } else {
                os << '_' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(static_cast<unsigned char>(c)) << '_';
            }
        }
        return os.str();
    }

    /// 文件路径 → 模块逻辑名
    /// "foo/bar/baz.py" (root=".") → "foo.bar.baz"
    /// "foo/bar/__init__.py" (root=".") → "foo.bar"
    static std::string path_to_module_name(const std::string &file_path,
        const std::string &root_dir)
    {
        // 去掉 root_dir 前缀
        std::string rel = file_path;
        if (!root_dir.empty()) {
            auto prefix = root_dir;
            // 确保 prefix 以 '/' 结尾
            if (prefix.back() != '/') prefix += '/';
            if (rel.find(prefix) == 0) { rel = rel.substr(prefix.size()); }
        }

        // 去掉 .py 后缀
        const std::string py_ext = ".py";
        if (rel.size() >= py_ext.size()
            && rel.compare(rel.size() - py_ext.size(), py_ext.size(), py_ext) == 0) {
            rel = rel.substr(0, rel.size() - py_ext.size());
        }

        // 去掉 __init__ 后缀（包模式）
        const std::string init_suffix = "/__init__";
        if (rel.size() >= init_suffix.size()
            && rel.compare(rel.size() - init_suffix.size(), init_suffix.size(), init_suffix)
                   == 0) {
            rel = rel.substr(0, rel.size() - init_suffix.size());
        } else if (rel == "__init__") {
            // 根包
            rel = "";
        }

        // '/' → '.'
        std::replace(rel.begin(), rel.end(), '/', '.');

        return rel;
    }
};

}// namespace py