#ifndef FASTLOG_CONCURRENCY_MPMC_QUEUE_H
#define FASTLOG_CONCURRENCY_MPMC_QUEUE_H

#include <atomic>
#include <cstddef>
#include <memory>
#include <new> // For std::hardware_destructive_interference_size
#include <type_traits>
#include <utility>

namespace fastlog {
namespace concurrency {

// Forward declaration for the base queue if you plan to have one
// template <typename T>
// class QueueBase { ... };

template <typename T, size_t BufferSize = 8192>
class MPMCQueue /* : public QueueBase<T> */ {
  static_assert(BufferSize >= 2 && (BufferSize & (BufferSize - 1)) == 0,
                "BufferSize must be a power of 2.");

private:
  // C++17 aignment specifier, provides cache line size to prevent false sharing
  // False sharing occurs when multiple threads access different variables on
  // the same cache line, causing unnecessary cache invalidations. By aligning
  // cursors to cache lines, we ensure they don't share a cache line.
#if defined(__cpp_lib_hardware_interference_size)
  static constexpr size_t kCachelineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr size_t kCachelineSize = 64;
#endif

  // Mask for fast modulo operation
  static constexpr size_t kMask = BufferSize - 1;

  struct Slot {
    // The sequence number is used to coordinate producers and consumers.
    // A slot is available for writing if its sequence is equal to the writer's
    // position. A slot is available for reading if its sequence is one greater
    // than the reader's position.
    std::atomic<size_t> sequence;
    // Storage for the object itself. Using std::aligned_storage is a classic
    // way to handle uninitialized memory for placement new.
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;

    // Note: No constructor or destructor for Slot. We manage memory manually.
  };

  // Correctly align cursors to prevent false sharing between them
  alignas(
      kCachelineSize) std::atomic<size_t> _head; // To be modified by producers
  alignas(
      kCachelineSize) std::atomic<size_t> _tail; // To be modified by consumers

  // The buffer itself. Using unique_ptr for automatic and exception-safe memory
  // management.
  std::unique_ptr<Slot[]> _buffer;

public:
  MPMCQueue() : _head(0), _tail(0) {
    // Allocate buffer using make_unique for exception safety
    _buffer = std::make_unique<Slot[]>(BufferSize);
    // Initialize sequence numbers for each slot
    for (size_t i = 0; i < BufferSize; ++i) {
      _buffer[i].sequence.store(i, std::memory_order_relaxed);
    }
  }

  ~MPMCQueue() {
    // To destruct remaining elements, we pop them one by one.
    // The `pop` operation itself handles calling the destructor of the
    // element inside the queue's buffer.
    // We don't need to do anything with the popped value.
    while (true) {
      // We still need a temporary variable to call pop, but its state
      // doesn't matter before the call. Let's create it inside the loop.
      // For types that are not default constructible, this is still a problem.
      //
      // The *correct* approach is to create a new `try_pop()` or similar
      // that doesn't require an output parameter, or to implement a `clear()`
      // method that directly manipulates the tail pointer and calls
      // destructors.

      // Let's go with a more direct and correct `clear()` method logic inside
      // the destructor. This avoids any constructor requirements on T.

      size_t tail = _tail.load(std::memory_order_relaxed);
      Slot *slot = &_buffer[tail & kMask];
      size_t seq = slot->sequence.load(std::memory_order_acquire);
      intptr_t diff = (intptr_t)seq - (intptr_t)(tail + 1);

      if (diff == 0) {
        // This slot contains an element. Destruct it.
        T *data_ptr = std::launder(reinterpret_cast<T *>(&slot->storage));
        std::destroy_at(data_ptr);

        // Mark the slot as empty for future producers.
        slot->sequence.store(tail + BufferSize, std::memory_order_release);

        // Advance the tail pointer. No CAS needed as we are in the destructor
        // and no other consumers should be running.
        _tail.store(tail + 1, std::memory_order_relaxed);
      } else {
        // If diff != 0, it means the queue is empty.
        break;
      }
    }
  }

  // Disable copy operations
  MPMCQueue(const MPMCQueue &) = delete;
  MPMCQueue &operator=(const MPMCQueue &) = delete;

  // Correctly implemented move operations
  MPMCQueue(MPMCQueue &&other) noexcept
      : _head(other._head.load(std::memory_order_relaxed)),
        _tail(other._tail.load(std::memory_order_relaxed)),
        _buffer(std::exchange(other._buffer, nullptr)) {}

  MPMCQueue &operator=(MPMCQueue &&other) noexcept {
    if (this != &other) {
      _head.store(other._head.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
      _tail.store(other._tail.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
      _buffer = std::exchange(other._buffer, nullptr);
    }
    return *this;
  }

  template <typename... Args>
  bool emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    size_t head = _head.load(std::memory_order_relaxed);
    while (true) {
      Slot *slot = &_buffer[head & kMask];
      size_t seq = slot->sequence.load(std::memory_order_acquire);
      intptr_t diff = (intptr_t)seq - (intptr_t)head;

      if (diff == 0) {
        // This slot is available for writing. Try to claim it.
        // We use acq_rel to prevent reordering of the CAS with surrounding
        // loads/stores and to synchronize with other producers.
        if (_head.compare_exchange_weak(head, head + 1,
                                        std::memory_order_acq_rel)) {
          // Successfully claimed the slot. Construct the object in place.
          new (&slot->storage) T(std::forward<Args>(args)...);
          // The release store ensures that the construction of T happens-before
          // this sequence update is visible to any consumer.
          slot->sequence.store(head + 1, std::memory_order_release);
          return true;
        }
        // Lost the race, CAS failed. `head` is updated by CAS automatically.
        // Loop again with the new `head` value.
      } else if (diff < 0) {
        // Queue is full. The sequence number is from a previous lap around the
        // buffer.
        return false;
      } else {
        // diff > 0. Another producer is ahead of us.
        // We need to reload the _head cursor to get a more recent value.
        head = _head.load(std::memory_order_relaxed);
      }
    }
  }

  bool push(T &&value) noexcept(std::is_nothrow_move_constructible_v<T>) {
    return emplace(std::move(value));
  }

  bool push(const T &value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    return emplace(value);
  }

  bool pop(T &result) noexcept(std::is_nothrow_move_assignable_v<T> ||
                               std::is_nothrow_copy_assignable_v<T>) {
    size_t tail = _tail.load(std::memory_order_relaxed);
    while (true) {
      Slot *slot = &_buffer[tail & kMask];
      size_t seq = slot->sequence.load(std::memory_order_acquire);
      intptr_t diff = (intptr_t)seq - (intptr_t)(tail + 1);

      if (diff == 0) {
        // This slot is ready for reading. Try to claim it.
        // We use acq_rel to synchronize with other consumers.
        if (_tail.compare_exchange_weak(tail, tail + 1,
                                        std::memory_order_acq_rel)) {
          // Successfully claimed the slot.
          T *data_ptr = std::launder(reinterpret_cast<T *>(&slot->storage));

          // Move/Copy the data out.
          result = std::move(*data_ptr);

          // IMPORTANT: Destruct the object in the slot.
          std::destroy_at(data_ptr);

          // The release store marks the slot as empty and available for
          // producers. It ensures the move and destruction happen-before this
          // update.
          slot->sequence.store(tail + BufferSize, std::memory_order_release);
          return true;
        }
        // Lost the race to another consumer. Loop again.
      } else if (diff < 0) {
        // Queue is empty.
        return false;
      } else {
        // diff > 0. Another consumer is ahead of us.
        // We need to reload the _tail cursor to get a more recent value.
        tail = _tail.load(std::memory_order_relaxed);
      }
    }
  }
};

} // namespace concurrency
} // namespace fastlog

#endif // FASTLOG_CONCURRENCY_MPMC_QUEUE_H