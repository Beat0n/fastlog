#include "fastlog/logger.h"
#include "fastlog/details/backend.h"
#include "fastlog/sinks/sink.h"
#include <chrono>
#include <thread>

namespace fastlog {

// --- Backend Singleton ---
static details::Backend &get_backend_instance() {
  static details::Backend instance;
  return instance;
}

// --- Logger class implementation ---
// The pointer to backend is an implementation detail now, no need in header.
// The global logger object is what users interact with.
struct LoggerImpl : public Logger {
  details::Backend *backend = nullptr;
};

// We define the template member function Logger::log here, in the .cpp file
template <typename... Args>
void Logger::log(LogLevel level, fmt::format_string<Args...> fmt,
                 Args &&...args, std::source_location loc) {
  details::Backend &backend = get_backend_instance();

  LogMessage msg{.timestamp = std::chrono::system_clock::now(),
                 .thread_id = std::this_thread::get_id(),
                 .level = level,
                 .location = loc,
                 .payload = fmt::format(fmt, std::forward<Args>(args)...)};

  backend.log(std::move(msg));
}

// --- Global API Implementation ---
Logger &default_logger() {
  static Logger instance;
  return instance;
}

void set_formatter(std::unique_ptr<formatters::Formatter> formatter) {
    get_backend_instance().set_formatter(std::move(formatter));
}

void add_sink_impl(std::unique_ptr<sinks::Sink> sink) {
  if (sink) {
    get_backend_instance().add_sink(std::move(sink));
  }
}

// --- Explicit Instantiations for Logger::log ---
// This is the key to solving the template problem. We must explicitly
// instantiate every combination of log function and argument types we want to
// support. This is cumbersome, so we'll instantiate some common ones.

// Example instantiations
template void Logger::log<const char *>(LogLevel,
                                        fmt::format_string<const char *>,
                                        const char *&&, std::source_location);
template void
Logger::log<int, const char *>(LogLevel, fmt::format_string<int, const char *>,
                               int &&, const char *&&, std::source_location);

template void Logger::log<std::string>(LogLevel,
                                       fmt::format_string<std::string>,
                                       std::string &&, std::source_location);

} // namespace fastlog