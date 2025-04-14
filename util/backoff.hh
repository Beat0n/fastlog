#ifndef FLOG_backoff_HH
#define FLOG_backoff_HH

#include <chrono>
#include <thread>

namespace flog {

// Performs exponential backoff in spin loops.
// Backing off in spin loops reduces contention and improves overall
// performance.
class backoff {
 public:
  // 初始退避值（最小等待时间）
  static constexpr int MIN_DELAY = 1;        // 1 微秒
  static constexpr int MAX_DELAY = 1024;     // 最大等待时间
  static constexpr int YIELD_THRESHOLD = 8;  // 超过 8 次后使用 yield

  // 构造函数
  backoff() : delay(MIN_DELAY), spin_count(0) {}

  // 执行自旋并进行退避
  void snooze() {
    if (spin_count < YIELD_THRESHOLD) {
      // 自旋一段时间后尝试退避
      spin_count++;
      cpu_relax();  // 使用空指令降低 CPU 负载
    } else {
      // 超过 YIELD_THRESHOLD 后，主动让出 CPU
      std::this_thread::yield();
    }

    // 退避增加，指数退避（2 的幂次）
    delay = std::min(delay * 2, MAX_DELAY);
    std::this_thread::sleep_for(std::chrono::microseconds(delay));
  }

  // 重置退避计数
  void reset() {
    delay = MIN_DELAY;
    spin_count = 0;
  }

 private:
  int delay;       // 当前退避时间（微秒）
  int spin_count;  // 自旋次数

  // 空转一小段时间，避免 CPU 过热
  inline void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64)
    asm volatile("pause" ::: "memory");
#elif defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("yield" ::: "memory");
#else
    std::this_thread::yield();
#endif
  }
};

}  // namespace flog

#endif