#ifndef FLOG_MPMC_QUEUE_HH
#define FLOG_MPMC_QUEUE_HH

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "base_queue.hh"

namespace flog {

template <typename T, size_t BufferSize = 1024>
class MPMCQueue : public QueueBase<T> {
  static_assert((BufferSize >= 2) && ((BufferSize & (BufferSize - 1)) == 0),
                "BufferSize must be power of 2");

private:
#if defined(__cpp_lib_hardware_interference_size)
  static constexpr size_t kCachelineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr size_t kCachelineSize = 64;
#endif
  static constexpr size_t kMask = BufferSize - 1;

  struct ItemT {
    std::atomic<size_t> sequence;
    T data;
  };

private:
  alignas(kCachelineSize) std::atomic<size_t> pop_cursor_;
  alignas(kCachelineSize) std::atomic<size_t> push_cursor_;
  alignas(kCachelineSize) ItemT *buffer_;

public:
  MPMCQueue() {
    buffer_ = new ItemT[BufferSize];
    for (size_t i = 0; i != BufferSize; i += 1) {
      buffer_[i].sequence.store(i, std::memory_order_relaxed);
    }
    pop_cursor_.store(0, std::memory_order_relaxed);
    push_cursor_.store(0, std::memory_order_relaxed);
  }

  ~MPMCQueue() override { delete[] buffer_; }

  MPMCQueue(MPMCQueue &&other) noexcept {
    pop_cursor_.store(other.pop_cursor_.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
    push_cursor_.store(other.push_cursor_.load(std::memory_order_relaxed),
                       std::memory_order_relaxed);
    delete[] buffer_;
    buffer_ = other.buffer_;
    other.buffer_ = nullptr;
  }

  MPMCQueue &operator=(MPMCQueue &&other) noexcept {
    if (this != &other) {
      pop_cursor_.store(other.pop_cursor_.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
      push_cursor_.store(other.push_cursor_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
      delete[] buffer_;
      buffer_ = other.buffer_;
      other.buffer_ = nullptr;
    }
    return *this;
  }

  template <typename U>
    requires std::is_same_v<T, std::decay_t<U>>
  bool push(U &&data) {
    ItemT *item;
    size_t pos = push_cursor_.load(std::memory_order_relaxed);
    while (true) {
      item = &buffer_[pos & kMask]; // item = &buffer_[tail_pos % BufferSize]
      size_t seq = item->sequence.load(std::memory_order_acquire);
      int64_t diff = (int64_t)seq - (int64_t)pos;
      if (diff == 0) {
        // try to occupy the pos
        if (push_cursor_.compare_exchange_weak(pos, pos + 1,
                                               std::memory_order_relaxed))
          break;
      } else if (diff < 0) { // the queue is full
        return false;
      } else { // the pos has been occupied by other thread
        pos = push_cursor_.load(std::memory_order_relaxed);
      }
    }

    new (&item->data) T(std::forward<U>(data));
    item->sequence.store(pos + 1, std::memory_order_release);
    return true;
  }

  bool pop(T &result) {
    ItemT *item;
    size_t pos = pop_cursor_.load(std::memory_order_relaxed);
    while (true) {
      item = &buffer_[pos & kMask]; // item = &buffer_[tail_pos % BufferSize]
      size_t seq = item->sequence.load(std::memory_order_acquire);
      int64_t diff = (int64_t)seq - (int64_t)(pos + 1);
      if (diff == 0) {
        if (pop_cursor_.compare_exchange_weak(pos, pos + 1,
                                              std::memory_order_relaxed))
          break;
      } else if (diff < 0) {
        return false;
      } else {
        pos = pop_cursor_.load(std::memory_order_relaxed);
      }
    }

    if constexpr (std::is_copy_constructible<T>::value) {
      result = item->data;
    } else {
      result = std::move(item->data);
    }
    item->sequence.store(pos + BufferSize, std::memory_order_release);
    return true;
  }
};

} // namespace flog

#endif