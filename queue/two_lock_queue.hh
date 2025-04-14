#ifndef FLOG_TWOLOCKQUEUE_HH
#define FLOG_TWOLOCKQUEUE_HH

#include <mutex>

#include "base_queue.hh"

namespace flog {

// todo: unsafe when queue is empty (undefined behavior)
template <typename T> class TwoLockQueue : public QueueBase<T> {
private:
  struct Node {
    T data;
    Node *next;

    Node() : next(nullptr) {}
    explicit Node(const T &d) : data(d), next(nullptr) {}
    explicit Node(T &&d) : data(std::move(d)), next(nullptr) {}
  };

private:
  std::mutex mu_tail_;
  std::mutex mu_head_;
  Node *head_;
  Node *tail_;
  Node *dummy_;

public:
  TwoLockQueue() {
    dummy_ = new Node;
    head_ = dummy_;
    tail_ = dummy_;
  }
  ~TwoLockQueue() override {
    while (head_) {
      auto tmp = head_->next;
      delete head_;
      head_ = tmp;
    }
  }

  template <typename U>
    requires std::is_same_v<T, std::decay_t<U>>
  bool push(U &&item) {
    auto new_node = new Node(std::forward<U>(item));
    std::lock_guard<std::mutex> tail_lock(mu_tail_);
    tail_->next = new_node;
    tail_ = new_node;
    return true;
  }

  bool pop(T &result) {
    std::unique_lock<std::mutex> lock(mu_head_);
    auto old_dummy = head_;   // old_dummy 指向当前 dummy 节点
    dummy_ = old_dummy->next; // 更新 dummy_ 到下一个节点
    if (dummy_ == nullptr) {  // 如果队列为空
      return false;
    }
    if constexpr (std::is_copy_constructible<T>::value) {
      result = dummy_->data;
    } else {
      result = std::move(dummy_->data);
    }
    head_ = dummy_; // head_ 更新到新的 dummy
    lock.unlock();
    delete old_dummy; // 释放旧的 dummy
    return true;
  }
};

} // namespace flog

#endif