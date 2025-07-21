#ifndef FASTLOG_CONCURRENCY_BACKOFF_H
#define FASTLOG_CONCURRENCY_BACKOFF_H

#include <algorithm>
#include <chrono>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h> // For _mm_pause
#endif

namespace fastlog {
namespace concurrency {

/**
 * @class Backoff
 * @brief Implements a staged exponential backoff strategy for spin-wait loops.
 *
 * This class helps reduce contention and CPU usage in high-contention scenarios
 * by progressively increasing the wait time through three stages:
 * 1. Spinning (using CPU-specific pause instructions).
 * 2. Yielding (letting other threads run).
 * 3. Sleeping (putting the thread to sleep for an exponentially increasing
 * duration).
 */
class Backoff {
public:
  Backoff(int spin_limit = 10, int yield_limit = 20)
      : _spin_limit(spin_limit), _yield_limit(yield_limit), _sleep_duration(1) {
  }

  /**
   * @brief Execute one step of the backoff strategy. Call this in a spin-wait
   * loop.
   */
  void snooze() {
    if (_counter < _spin_limit) {
      // Stage 1: Spin using CPU-specific pause instructions.
      cpu_relax();
    } else if (_counter < _yield_limit) {
      // Stage 2: Yield the CPU time slice.
      std::this_thread::yield();
    } else {
      // Stage 3: Sleep for an exponentially increasing duration.
      std::this_thread::sleep_for(_sleep_duration);
      // Exponentially increase sleep time, with a cap.
      _sleep_duration = std::min(_sleep_duration * 2, kMaxSleep);
    }
    _counter++;
  }

  /**
   * @brief Reset the backoff state. Call this when the spin-wait succeeds.
   */
  void reset() {
    _counter = 0;
    _sleep_duration = std::chrono::microseconds(1);
  }

private:
  // Helper to issue a CPU-friendly pause instruction.
  static void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64)
    _mm_pause(); // More portable intrinsic than inline asm
#elif defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("yield" ::: "memory");
#else
    // Fallback for other architectures
#endif
  }

  static constexpr std::chrono::microseconds kMaxSleep{
      1000}; // Cap sleep at 1ms

  const int _spin_limit;
  const int _yield_limit;
  int _counter = 0;
  std::chrono::microseconds _sleep_duration;
};

} // namespace concurrency
} // namespace fastlog

#endif // FASTLOG_CONCURRENCY_BACKOFF_H