#pragma once

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
    /// "foo/bar/baz.py" (相对于 root) → "foo.bar.baz"
    static std::string path_to_module_name(const std::string &file_path,
        const std::string &root_dir);
};

}// namespace py