#pragma once

#include <atomic>
#include <string>
#include <unordered_map>

namespace py {

class KeyVersionTracker
{
  public:
    uint64_t get(const std::string &key) const
    {
        auto it = m_versions.find(key);
        return it != m_versions.end() ? it->second.load(std::memory_order_acquire) : 0;
    }

    void bump(const std::string &key)
    {
        m_versions[key].fetch_add(1, std::memory_order_release);
    }

  private:
    std::unordered_map<std::string, std::atomic<uint64_t>> m_versions;
};

}// namespace py