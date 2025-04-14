#ifndef FLOG_BASE_QUEUE_HH
#define FLOG_BASE_QUEUE_HH

namespace flog {

template <typename T>
class QueueBase {
 public:
  QueueBase() = default;
  virtual ~QueueBase() = default;

  // virtual bool push(const T&) = 0;
  // virtual bool push(T&&) = 0;
  // virtual bool pop(T&) = 0;

  QueueBase(const QueueBase&) = delete;
  QueueBase& operator=(const QueueBase&) = delete;

  QueueBase(QueueBase&&) noexcept = default;
  QueueBase& operator=(QueueBase&&) noexcept = default;
};

}  // namespace flog

#endif