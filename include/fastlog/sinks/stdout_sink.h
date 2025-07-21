#ifndef FASTLOG_SINKS_STDOUT_SINK_H
#define FASTLOG_SINKS_STDOUT_SINK_H

#include "fastlog/sinks/sink.h"
#include <iostream>

namespace fastlog {
namespace sinks {

/**
 * @brief A sink that writes log messages to the standard output (stdout).
 * @note This sink is NOT thread-safe by itself. It is designed to be called
 *       from a single thread, like the fastlog's backend worker thread.
 *       Synchronization must be handled by the caller if used in a
 * multi-threaded context directly.
 */
class StdoutSink : public Sink {
public:
  void log(std::string_view message) override {
    // No lock needed, assuming single-threaded access as per design.
    std::cout.write(message.data(), message.length());
    std::cout.put('\n');
  }

  void flush() override {
    // No lock needed.
    std::cout.flush();
  }
};

} // namespace sinks
} // namespace fastlog

#endif // FASTLOG_SINKS_STDOUT_SINK_H