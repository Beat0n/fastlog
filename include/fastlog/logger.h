#ifndef FASTLOG_LOGGER_H
#define FASTLOG_LOGGER_H

#include "fastlog/formatters/formatter.h"
#include "fastlog/log_message.h"
#include <memory>
#include <source_location>

#include <fmt/core.h>
#include <fmt/xchar.h>

namespace fastlog {
namespace sinks {
class Sink;
}

class Logger {
public:
  // Logger is now just a concept, its implementation details are hidden.
  // We will get it via the `default_logger()` function.
  // For simplicity, we can make its constructor private or protected
  // and control creation via a friend factory function if needed.
  // For now, let's keep it simple.

  // The core logging function. THIS IS NOW A TEMPLATE DECLARATION ONLY.
  template <typename... Args>
  void log(LogLevel level, fmt::format_string<Args...> fmt, Args &&...args,
           std::source_location loc = std::source_location::current());

  // --- Convenience methods ---
  // These remain in the header as they just call the primary `log` method.
  template <typename... Args>
  void trace(fmt::format_string<Args...> fmt, Args &&...args) {
    log(LogLevel::Trace, fmt, std::forward<Args>(args)...);
  }
  // ... (debug, info, warn, error, fatal convenience methods are the same) ...
  template <typename... Args>
  void debug(fmt::format_string<Args...> fmt, Args &&...args) {
    log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void info(fmt::format_string<Args...> fmt, Args &&...args) {
    log(LogLevel::Info, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void warn(fmt::format_string<Args...> fmt, Args &&...args) {
    log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void error(fmt::format_string<Args...> fmt, Args &&...args) {
    log(LogLevel::Error, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void fatal(fmt::format_string<Args...> fmt, Args &&...args) {
    log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
  }
};

// --- Global API ---
Logger &default_logger();

// Global convenience functions now call the global logger's methods.
template <typename... Args>
inline void trace(fmt::format_string<Args...> fmt, Args &&...args) {
  default_logger().trace(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args &&...args) {
  default_logger().debug(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args &&...args) {
  default_logger().info(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void warn(fmt::format_string<Args...> fmt, Args &&...args) {
  default_logger().warn(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args &&...args) {
  default_logger().error(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void fatal(fmt::format_string<Args...> fmt, Args &&...args) {
  default_logger().fatal(fmt, std::forward<Args>(args)...);
}


// Non-template helper function, declared here.
void add_sink_impl(std::unique_ptr<sinks::Sink> sink);

/**
 * @brief Adds a sink of a specific type to the default logger.
 *
 * This function template is constrained using C++20 concepts to ensure
 * type safety at compile time.
 *
 * @tparam SinkType The type of the sink to add. Must be derived from
 * sinks::Sink.
 * @tparam ...Args The types of the arguments for the sink's constructor.
 * @param ...args The arguments for the sink's constructor.
 */
template <typename SinkType, typename... Args>
// 2. Add the requires clause with two conditions
  requires std::derived_from<SinkType, sinks::Sink> &&
           std::constructible_from<SinkType, Args...>
void add_sink(Args &&...args) {
  add_sink_impl(std::make_unique<SinkType>(std::forward<Args>(args)...));
}

void set_formatter(std::unique_ptr<formatters::Formatter> formatter);

} // namespace fastlog
#endif // FASTLOG_LOGGER_H