#ifndef FASTLOG_DETAILS_BACKEND_H
#define FASTLOG_DETAILS_BACKEND_H

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "fastlog/concurrency/mpmc_queue.h"
#include "fastlog/formatters/formatter.h"
#include "fastlog/log_message.h"
#include "fastlog/sinks/sink.h"

namespace fastlog {
namespace details {

class Backend {
 public:
  Backend();
  ~Backend();

  // Deleted copy and move operations
  Backend(const Backend &) = delete;
  Backend &operator=(const Backend &) = delete;
  Backend(Backend &&) = delete;
  Backend &operator=(Backend &&) = delete;

  void add_sink(std::unique_ptr<sinks::Sink> sink);

  // This now works because the compiler knows what
  // fastlog::formatters::Formatter is.
  void set_formatter(std::unique_ptr<formatters::Formatter> formatter);

  void log(LogMessage &&message);

 private:
  void worker_loop();

  std::thread _worker_thread;
  std::atomic<bool> _active;

  std::unique_ptr<concurrency::MPMCQueue<LogMessage>> _queue;
  std::vector<std::unique_ptr<sinks::Sink>> _sinks;

  // This also works now, as we only need the incomplete type for a unique_ptr
  // member.
  std::unique_ptr<formatters::Formatter> _formatter;

  std::string _format_buffer;  // A reusable buffer for formatting
};

}  // namespace details
}  // namespace fastlog

#endif  // FASTLOG_DETAILS_BACKEND_H