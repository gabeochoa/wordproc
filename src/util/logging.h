#pragma once

// Use Afterhours logging as the backend
#include "../../vendor/afterhours/src/logging.h"

namespace logging {

enum class Level {
  Debug,
  Info,
  Warning,
  Error
};

// Current logging level (can be changed at runtime)
inline Level g_level = Level::Info;

inline void setLevel(Level level) {
  g_level = level;
}

// Simple timing helper for profiling
struct ScopedTimer {
  const char* name;
  double startTime;
  
  ScopedTimer(const char* n);
  ~ScopedTimer();
};

} // namespace logging

// Convenience macros that delegate to Afterhours logging
// Note: Debug/trace level is not output by default in Afterhours
#define LOG_DEBUG(...) log_trace(__VA_ARGS__)
#define LOG_INFO(...) log_info(__VA_ARGS__)
#define LOG_WARNING(...) log_warn(__VA_ARGS__)
#define LOG_ERROR(...) log_error(__VA_ARGS__)
#define SCOPED_TIMER(name) logging::ScopedTimer _timer_##__LINE__(name)
