#include "fastlog/details/backend.h"

#include <unistd.h>

#include <atomic>
#include <thread>
#include <utility>

#include "fastlog/concurrency/backoff.h"
#include "fastlog/formatters/pattern_formatter.h"

namespace fastlog {
namespace details {

Backend::Backend() : _active(true), _queue(std::make_unique<concurrency::MPMCQueue<LogMessage>>()), _format_buffer() {
  // Set a default formatter. This requires the full definition of
  // PatternFormatter.
  _formatter = std::make_unique<formatters::PatternFormatter>("%^[%Y-%M-%D %H:%M:%S] [%T] [%F:%l %f] %v");
  std::atomic_signal_fence(std::memory_order_seq_cst);
  _worker_thread = std::thread([this]() { this->worker_loop(); });
}

// The destructor needs the full definition of Formatter to destroy _formatter.
Backend::~Backend() {
  _active.store(false, std::memory_order_release);
  if (_worker_thread.joinable()) {
    _worker_thread.join();
  }
}

void Backend::add_sink(std::unique_ptr<sinks::Sink> sink) {
  if (sink) {
    // In a real multi-threaded scenario where sinks could be added
    // concurrently, this would need a mutex. For our design, it's called
    // during initial setup, so it's fine.
    _sinks.push_back(std::move(sink));
  }
}

// This needs the full definition of Formatter for the unique_ptr parameter.
void Backend::set_formatter(std::unique_ptr<formatters::Formatter> formatter) {
  if (formatter) {
    _formatter = std::move(formatter);
  }
}

void Backend::log(LogMessage &&message) {
  // Attempt to push to the queue. In a high-load scenario, we might
  // choose to drop the message if the queue is full.
  _queue->push(std::move(message));
}

void Backend::worker_loop() {
  concurrency::Backoff backoff;

  while (_active.load(std::memory_order_acquire)) {
    LogMessage msg;
    if (_queue->pop(msg)) {
      backoff.reset();  // Success, reset backoff

      // Format and sink the message.
      _format_buffer.clear();  // Reuse the buffer to avoid allocations
      // This call needs the full definition of Formatter for the virtual
      // dispatch.
      _formatter->format(msg, _format_buffer);

      for (auto &sink : _sinks) {
        sink->log(_format_buffer);
      }
    } else {
      // Queue is empty, use our backoff strategy to wait.
      backoff.snooze();
    }
  }

  // After the loop terminates, process any remaining items in the queue.
  LogMessage remaining_msg;
  while (_queue->pop(remaining_msg)) {
    _format_buffer.clear();
    _formatter->format(remaining_msg, _format_buffer);
    for (auto &sink : _sinks) {
      sink->log(_format_buffer);
    }
  }

  // Finally, flush all sinks to ensure data is persisted.
  for (auto &sink : _sinks) {
    sink->flush();
  }
}

}  // namespace details
}  // namespace fastlog