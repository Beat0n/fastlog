#ifndef FLOG_LOCKQUEUE_HH
#define FLOG_LOCKQUEUE_HH

#include <list>
#include <mutex>

#include "base_queue.hh"

namespace flog {

template <typename T> class LockQueue : public QueueBase<T> {
private:
  std::mutex mu_;
  std::list<T> items_;

public:
  LockQueue() = default;
  ~LockQueue() override = default;

  LockQueue(LockQueue &&other) noexcept {
    std::unique_lock<std::mutex> lock(other.mu_);
    items_ = std::move(other.items_);
  }
  LockQueue &operator=(LockQueue &&other) noexcept {
    if (this != &other) {
      std::unique_lock<std::mutex> lock(other.mu_);
      items_ = std::move(other.items_);
    }
    return *this;
  }

  template <typename U>
    requires std::is_same_v<T, std::decay_t<U>>
  bool push(U &&item) {
    std::unique_lock<std::mutex> lock(mu_);
    items_.push_back(std::forward<U>(item));
    return true;
  }

  bool pop(T &result) {
    std::unique_lock<std::mutex> lock(mu_);
    if (items_.empty()) {
      return false;
    }
    if constexpr (std::is_copy_constructible<T>::value) {
      result = items_.front();
    } else {
      result = std::move(items_.front());
    }
    items_.pop_front();
    return true;
  }
};

} // namespace flog

#endif