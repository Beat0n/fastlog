#ifndef FASTLOG_LOG_MESSAGE_H
#define FASTLOG_LOG_MESSAGE_H

#include <ctime>
#include <source_location>  // C++20 for source location
#include <string>
#include <thread>

namespace fastlog {
namespace details {
/**
 * @struct noncopyable
 * @brief A helper struct that can be inherited from or included as a member
 *        to make a class non-copyable. This is a classic C++ idiom.
 */
struct noncopyable {
  noncopyable() = default;
  ~noncopyable() = default;

  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;

  noncopyable(noncopyable &&) = default;
  noncopyable &operator=(noncopyable &&) = default;
};

}  // namespace details

/**
 * @enum LogLevel
 * @brief Defines the severity levels for log messages.
 *        Ordered from least severe to most severe.
 */
enum class LogLevel : uint8_t { Trace = 0, Debug, Info, Warn, Error, Fatal };

/**
 * @brief Converts a LogLevel enum to its string representation.
 * @param level The log level to convert.
 * @return A string view representing the log level.
 */
inline constexpr std::string_view level2string(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Fatal:
      return "FATAL";
  }
  return "UNKNOWN";
}

namespace details {
// Declare the global log level as an external atomic variable.
// It will be defined in logger.cpp.
extern std::atomic<LogLevel> g_log_level;
}  // namespace details

/**
 * @struct LogMessage
 * @brief Represents a single, structured log entry.
 *
 * This struct is designed to be lightweight and efficient to move. It captures
 * all the necessary metadata for a log entry at the point of creation.
 * The formatting of this message is deferred to the backend.
 */
struct LogMessage {
 public:
  // --- Member Variables ---

  // The timestamp when the log message was created.
  std::time_t timestamp;

  // The ID of the thread that generated the log message.
  std::thread::id thread_id;

  // The severity level of the log message.
  LogLevel level;

  // C++20 source location information. Captures file, line, and function name
  // at the call site with zero overhead.
  std::source_location location;

  // The actual log message payload.
  // We use std::string to own the data. This is a good starting point.
  // For extreme performance, this could later be optimized.
  std::string payload;

  // [[no_unique_address]] is a C++20 attribute that ensures this empty
  // member does not take up any space in the LogMessage struct.
  [[no_unique_address]] details::noncopyable _nocopy;
};

}  // namespace fastlog

#endif  // FASTLOG_LOG_MESSAGE_H