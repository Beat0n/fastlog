#ifndef FLOG_SPSC_QUEUE_HH
#define FLOG_SPSC_QUEUE_HH

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "base_queue.hh"

namespace flog {

template <typename T, size_t Size = 1024>
class SPSCQueue : public QueueBase<T> {
private:
  static_assert(Size != 0 && (Size & (Size - 1)) == 0,
                "Size must be power of 2");
#if defined(__cpp_lib_hardware_interference_size)
  static constexpr size_t kCachelineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr size_t kCachelineSize = 64;
#endif
  static constexpr size_t kMask = Size - 1;

  alignas(kCachelineSize) std::atomic<size_t> push_cursor_{0};
  alignas(kCachelineSize) std::atomic<size_t> pop_cursor_{0};
  alignas(kCachelineSize) size_t cache_push_cursor_{0};
  alignas(kCachelineSize) size_t cache_pop_cursor_{0};
  alignas(kCachelineSize) T *buffer_;

private:
  bool is_full(size_t push_cursor, size_t pop_cursor) const {
    return (push_cursor - pop_cursor) == Size;
  }

  bool is_empty(size_t push_cursor, size_t pop_cursor) const {
    return push_cursor == pop_cursor;
  }

public:
  SPSCQueue() : buffer_(new T[Size]) {}
  ~SPSCQueue() override { delete[] buffer_; }

  SPSCQueue(SPSCQueue &&other) noexcept {
    push_cursor_.store(other.push_cursor_.load(std::memory_order_relaxed),
                       std::memory_order_relaxed);
    pop_cursor_.store(other.pop_cursor_.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
    cache_push_cursor_ = other.cache_push_cursor_;
    cache_pop_cursor_ = other.cache_pop_cursor_;
    delete buffer_;
    buffer_ = other.buffer_;
    other.buffer_ = nullptr;
  }

  SPSCQueue &operator=(SPSCQueue &&other) noexcept {
    if (this != &other) {
      push_cursor_.store(other.push_cursor_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
      pop_cursor_.store(other.pop_cursor_.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
      cache_push_cursor_ = other.cache_push_cursor_;
      cache_pop_cursor_ = other.cache_pop_cursor_;
      delete[] buffer_;
      buffer_ = other.buffer_;
      other.buffer_ = nullptr;
    }
    return *this;
  }

  template <typename U>
    requires std::is_same_v<T, std::decay_t<U>>
  bool push(U &&item) {
    const auto push_cursor = push_cursor_.load(std::memory_order_relaxed);
    if (is_full(push_cursor, cache_pop_cursor_)) {
      cache_pop_cursor_ = pop_cursor_.load(std::memory_order_acquire);
      if (is_full(push_cursor, cache_pop_cursor_)) {
        return false;
      }
    }
    new (&buffer_[push_cursor & kMask]) T(std::forward<U>(item));
    push_cursor_.store(push_cursor + 1, std::memory_order_release);
    return true;
  }

  bool pop(T &item) {
    const auto pop_cursor = pop_cursor_.load(std::memory_order_relaxed);
    if (is_empty(cache_push_cursor_, pop_cursor)) {
      cache_push_cursor_ = push_cursor_.load(std::memory_order_acquire);
      if (is_empty(cache_push_cursor_, pop_cursor)) {
        return false;
      }
    }
    if constexpr (std::is_copy_constructible<T>::value) {
      item = buffer_[pop_cursor & kMask];
    } else {
      item = std::move(buffer_[pop_cursor & kMask]);
    }
    pop_cursor_.store(pop_cursor + 1, std::memory_order_release);
    return true;
  }
};
} // namespace flog

#endif
