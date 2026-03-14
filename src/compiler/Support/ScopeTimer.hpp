#pragma once

#include <spdlog/spdlog.h>

#include <chrono>
#include <string>

namespace pylang {

class ScopeTimer
{
  public:
	explicit ScopeTimer(std::string name, spdlog::level::level_enum level = spdlog::level::debug)
		: m_name(std::move(name)), m_level(level), m_start(std::chrono::steady_clock::now())
	{}

	~ScopeTimer()
	{
		auto elapsed = std::chrono::steady_clock::now() - m_start;
		auto ms = std::chrono::duration<double, std::milli>(elapsed).count();
		spdlog::log(m_level, "[timer] {}: {:.2f}ms", m_name, ms);
	}

	ScopeTimer(const ScopeTimer &) = delete;
	ScopeTimer &operator=(const ScopeTimer &) = delete;

  private:
	std::string m_name;
	spdlog::level::level_enum m_level;
	std::chrono::steady_clock::time_point m_start;
};

}// namespace pylang

#define PYLANG_TIMER_CONCAT_(a, b) a##b
#define PYLANG_TIMER_CONCAT(a, b) PYLANG_TIMER_CONCAT_(a, b)

#define PYLANG_TIMER(name) ::pylang::ScopeTimer PYLANG_TIMER_CONCAT(_pylang_timer_, __LINE__)(name)

#define PYLANG_TIMER_INFO(name) \
	::pylang::ScopeTimer PYLANG_TIMER_CONCAT(_pylang_timer_, __LINE__)(name, spdlog::level::info)