#ifndef FASTLOG_CONCURRENCY_SPSC_QUEUE_H
#define FASTLOG_CONCURRENCY_SPSC_QUEUE_H
#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
namespace fastlog {
namespace concurrency {
/**
@class SPSCQueue
@brief A lock-free Single-Producer, Single-Consumer queue.
This implementation is based on a circular buffer and uses cached cursors
to minimize atomic operations, making it extremely fast.
The producer thread can push, and the consumer thread can pop.
Access by more than one producer or consumer thread results in undefined
behavior.
@tparam T The type of elements in the queue.
@tparam BufferSize The size of the circular buffer. Must be a power of 2.
*/
template <typename T, size_t BufferSize = 8192> class SPSCQueue {
  static_assert(BufferSize >= 2 && (BufferSize & (BufferSize - 1)) == 0,
                "BufferSize must be a power of 2.");

private:
#if defined(__cpp_lib_hardware_interference_size)
  static constexpr size_t kCachelineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr size_t kCachelineSize = 64;
#endif
  // Mask for fast modulo operation
  static constexpr size_t kMask = BufferSize - 1;

  // The producer's cursor. Only the producer thread can write to this.
  alignas(kCachelineSize) std::atomic<size_t> _head;

  // The consumer's cached copy of the head cursor.
  alignas(kCachelineSize) size_t _cached_head;

  // The consumer's cursor. Only the consumer thread can write to this.
  alignas(kCachelineSize) std::atomic<size_t> _tail;

  // The producer's cached copy of the tail cursor.
  alignas(kCachelineSize) size_t _cached_tail;

  // The buffer itself. We use aligned_storage to manually manage object
  // lifetime. Using unique_ptr for automatic and exception-safe memory
  // management.
  std::unique_ptr<T[]> _buffer;

public:
  SPSCQueue() : _head(0), _cached_head(0), _tail(0), _cached_tail(0) {
    // Use aligned new if T has specific alignment requirements.
    // For simplicity, standard new is often fine if T is not over-aligned.
    _buffer = std::make_unique<T[]>(BufferSize);
  }
  ~SPSCQueue() {
    // Destruct any remaining elements.
    T dummy;
    while (pop(dummy)) {
    }
  }

  // Disable copy operations
  SPSCQueue(const SPSCQueue &) = delete;
  SPSCQueue &operator=(const SPSCQueue &) = delete;
  // For simplicity, we also delete move operations. A queue's identity is tied
  // to its memory, and moving it is complex. This can be implemented if needed.
  SPSCQueue(SPSCQueue &&) = delete;
  SPSCQueue &operator=(SPSCQueue &&) = delete;

  template <typename... Args>
    requires std::is_constructible_v<T, Args...>
  bool emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    const auto head = _head.load(std::memory_order_relaxed);

    if (head - _cached_tail == BufferSize) {
      // Queue might be full, check the real tail cursor.
      _cached_tail = _tail.load(std::memory_order_acquire);
      if (head - _cached_tail == BufferSize) {
        return false; // Queue is indeed full.
      }
    }

    // Construct the object in place.
    new (&_buffer[head & kMask]) T(std::forward<Args>(args)...);

    // Release memory order ensures that the construction of T happens-before
    // the head cursor is updated and made visible to the consumer.
    _head.store(head + 1, std::memory_order_release);
    return true;
  }

  bool push(const T &value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    return emplace(value);
  }

  bool push(T &&value) noexcept(std::is_nothrow_move_constructible_v<T>) {
    return emplace(std::move(value));
  }

  bool pop(T &result) noexcept(std::is_nothrow_move_assignable_v<T>) {
    const auto tail = _tail.load(std::memory_order_relaxed);

    if (tail == _cached_head) {
      // Queue might be empty, check the real head cursor.
      _cached_head = _head.load(std::memory_order_acquire);
      if (tail == _cached_head) {
        return false; // Queue is indeed empty.
      }
    }

    T &item_in_slot = _buffer[tail & kMask];
    result = std::move(item_in_slot);

    // IMPORTANT: Destruct the object in the slot.
    item_in_slot.~T();

    // Release memory order ensures that the move and destruction happen-before
    // the tail cursor is updated and made visible to the producer.
    _tail.store(tail + 1, std::memory_order_release);
    return true;
  }

  // A 'front' style pop that gives a pointer to the element.
  // The user must call `commit_pop()` after using the element.
  // This can avoid one move/copy operation.
  T *front() {
    const auto tail = _tail.load(std::memory_order_relaxed);
    if (tail == _cached_head) {
      _cached_head = _head.load(std::memory_order_acquire);
      if (tail == _cached_head) {
        return nullptr;
      }
    }
    return &_buffer[tail & kMask];
  }

  void commit_pop() {
    const auto tail = _tail.load(std::memory_order_relaxed);
    _buffer[tail & kMask].~T();
    _tail.store(tail + 1, std::memory_order_release);
  }
};
} // namespace concurrency
} // namespace fastlog
#endif // FASTLOG_CONCURRENCY_SPSC_QUEUE_H