#ifndef FASTLOG_LOGGER_H
#define FASTLOG_LOGGER_H

#include <source_location>

#include "fastlog/details/backend.h"
#include "fastlog/log_message.h"
#include "fastlog/sinks/sink.h"
#include "fmt/format.h"

namespace fastlog {
namespace details {

inline Backend& get_backend_instance() {
  static Backend default_backend;
  return default_backend;
}

inline void set_formatter(std::unique_ptr<formatters::Formatter> formatter) { get_backend_instance().set_formatter(std::move(formatter)); }

inline void set_level(LogLevel level) { details::g_log_level.store(level, std::memory_order_relaxed); }

inline LogLevel get_level() { return details::g_log_level.load(std::memory_order_relaxed); }

template <typename SinkType, typename... Args>
  requires std::derived_from<SinkType, sinks::Sink> && std::constructible_from<SinkType, Args...>
void add_sink(Args&&... args) {
  get_backend_instance().add_sink(std::move(std::make_unique<SinkType>(std::forward<Args>(args)...)));
}

class LogBuilder {
 public:
  // When a builder is created, it captures the log level and source location.
  LogBuilder(LogLevel level, std::source_location loc = std::source_location::current())
      : _level(level), _loc(loc), _enabled(level >= g_log_level.load(std::memory_order_relaxed)) {}

  // The destructor is where the log message is actually sent.
  // This is the core of the RAII magic.
  ~LogBuilder() {
    if (_enabled && !_payload.empty()) {
      details::Backend& backend = get_backend_instance();
      LogMessage msg{.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
                     .thread_id = std::this_thread::get_id(),
                     .location = _loc,
                     .level = _level,
                     .payload = std::move(_payload)};
      backend.log(std::move(msg));
    }
  }

  // This is the function that takes the format string and args.
  template <typename... Args>
  void log(fmt::format_string<Args...> fmt, Args&&... args) {
    if (_enabled) {
      _payload = fmt::format(fmt, std::forward<Args>(args)...);
    }
  }

  // Make the builder move-only
  LogBuilder(const LogBuilder&) = delete;
  LogBuilder& operator=(const LogBuilder&) = delete;
  LogBuilder(LogBuilder&&) = default;
  LogBuilder& operator=(LogBuilder&&) = default;

 private:
  LogLevel _level;
  std::source_location _loc;
  bool _enabled;
  std::string _payload;
};
}  // namespace details
// --- Core Logging API ---
// These functions return a temporary LogBuilder object.
// [[nodiscard]] warns the user if they write `fastlog::info();` without a log call.

[[nodiscard]] inline details::LogBuilder trace(std::source_location loc = std::source_location::current()) {
  return details::LogBuilder(LogLevel::Trace, loc);
}
[[nodiscard]] inline details::LogBuilder debug(std::source_location loc = std::source_location::current()) {
  return details::LogBuilder(LogLevel::Debug, loc);
}
[[nodiscard]] inline details::LogBuilder info(std::source_location loc = std::source_location::current()) {
  return details::LogBuilder(LogLevel::Info, loc);
}
[[nodiscard]] inline details::LogBuilder warn(std::source_location loc = std::source_location::current()) {
  return details::LogBuilder(LogLevel::Warn, loc);
}
[[nodiscard]] inline details::LogBuilder error(std::source_location loc = std::source_location::current()) {
  return details::LogBuilder(LogLevel::Error, loc);
}
[[nodiscard]] inline details::LogBuilder fatal(std::source_location loc = std::source_location::current()) {
  return details::LogBuilder(LogLevel::Fatal, loc);
}

}  // namespace fastlog

// --- Convenience Macros ---
// These macros provide a more function-like syntax, e.g., `FASTLOG_INFO(...)`.
// This is the recommended way for users to call the logger.

#define FASTLOG_TRACE(...) fastlog::trace().log(__VA_ARGS__)
#define FASTLOG_DEBUG(...) fastlog::debug().log(__VA_ARGS__)
#define FASTLOG_INFO(...) fastlog::info().log(__VA_ARGS__)
#define FASTLOG_WARN(...) fastlog::warn().log(__VA_ARGS__)
#define FASTLOG_ERROR(...) fastlog::error().log(__VA_ARGS__)
#define FASTLOG_FATAL(...) fastlog::fatal().log(__VA_ARGS__)

#endif  // FASTLOG_LOGGER_H