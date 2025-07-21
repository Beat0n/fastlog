#ifndef FASTLOG_DETAILS_BACKEND_H
#define FASTLOG_DETAILS_BACKEND_H

#include "fastlog/concurrency/mpmc_queue.h"
#include "fastlog/concurrency/scoped_thread.h"
#include "fastlog/log_message.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

// --- Correct Forward Declaration ---
// We only need to know that these classes exist to use a unique_ptr to them.
// The full definition is only needed in the .cpp file.
namespace fastlog {
namespace sinks {
class Sink;
}
namespace formatters {
class Formatter;
}
} // namespace fastlog

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

  concurrency::ScopedThread _worker_thread;
  std::atomic<bool> _active;

  std::unique_ptr<concurrency::MPMCQueue<LogMessage>> _queue;
  std::vector<std::unique_ptr<sinks::Sink>> _sinks;

  // This also works now, as we only need the incomplete type for a unique_ptr
  // member.
  std::unique_ptr<formatters::Formatter> _formatter;

  std::string _format_buffer; // A reusable buffer for formatting
};

} // namespace details
} // namespace fastlog

#endif // FASTLOG_DETAILS_BACKEND_H