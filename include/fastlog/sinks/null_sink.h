#ifndef FASTLOG_SINKS_NULL_SINK_H
#define FASTLOG_SINKS_NULL_SINK_H

#include "fastlog/sinks/sink.h"

namespace fastlog {
namespace sinks {

/**
 * @brief A sink that discards all log messages.
 *        Useful for performance benchmarking the core logging pipeline
 *        without the overhead of I/O operations.
 */
class NullSink : public Sink {
 public:
  void log(std::string_view) override {
    // Do nothing.
  }

  void flush() override {
    // Do nothing.
  }
};

}  // namespace sinks
}  // namespace fastlog

#endif  // FASTLOG_SINKS_NULL_SINK_H