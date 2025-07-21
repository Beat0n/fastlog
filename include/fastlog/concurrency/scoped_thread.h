#ifndef FASTLOG_CONCURRENCY_SCOPED_THREAD_H
#define FASTLOG_CONCURRENCY_SCOPED_THREAD_H

#include <thread>
#include <utility>

namespace fastlog {
namespace concurrency {

/**
 * @class ScopedThread
 * @brief A RAII-style wrapper for std::thread that automatically joins on
 * destruction. This mimics the basic behavior of C++20's std::jthread.
 */
class ScopedThread {
public:
  // Default constructor for an empty thread object
  ScopedThread() noexcept = default;

  // Constructor that takes a function and its arguments to run in a new thread
  template <typename Function, typename... Args>
  // 2. Add the requires clause
    requires std::invocable<std::decay_t<Function>, std::decay_t<Args>...>
  explicit ScopedThread(Function &&f, Args &&...args)
      : _thread(std::forward<Function>(f), std::forward<Args>(args)...) {}

  // Destructor that joins the thread if it's joinable
  ~ScopedThread() {
    if (_thread.joinable()) {
      _thread.join();
    }
  }

  // --- Rule of Five ---
  // A thread object is not copyable
  ScopedThread(const ScopedThread &) = delete;
  ScopedThread &operator=(const ScopedThread &) = delete;

  // It is movable
  ScopedThread(ScopedThread &&other) noexcept = default;
  ScopedThread &operator=(ScopedThread &&other) noexcept {
    // If this thread is still running, join it before assigning the new one.
    if (_thread.joinable()) {
      _thread.join();
    }
    _thread = std::move(other._thread);
    return *this;
  }

  // --- std::thread interface pass-through ---

  bool joinable() const noexcept { return _thread.joinable(); }

  void join() { _thread.join(); }

  void detach() { _thread.detach(); }

  std::thread::id get_id() const noexcept { return _thread.get_id(); }

  std::thread &native_handle() { return _thread; }

private:
  std::thread _thread;
};

} // namespace concurrency
} // namespace fastlog

#endif // FASTLOG_CONCURRENCY_SCOPED_THREAD_H