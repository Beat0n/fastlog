#ifndef FLOG_LOGGER_HH
#define FLOG_LOGGER_HH

#include "../queue/mpmc_queue.hh"
#include "../queue/spsc_queue.hh"
#include "../util/backoff.hh"
#include "log_item.hh"
#include "log_level.hh"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace flog {

constexpr size_t BufferSize = 128;

// *************************** GlobalLogger ***************************

class GlobalLogger : public MPMCQueue<std::unique_ptr<LogItem>, BufferSize> {
  friend class LocalLogger;

private:
  using LogItemPtr = std::unique_ptr<LogItem>;
  std::atomic_bool exit_;
  std::string filename_;
  std::thread flush_worker_;
  size_t flush_freq_;

public:
  explicit GlobalLogger(const std::string &filename, size_t flush_freq = 0)
      : exit_(false), filename_(filename),
        flush_worker_([this] { flush_task(); }), flush_freq_(flush_freq) {}

  ~GlobalLogger() override {
    exit_.store(true, std::memory_order_seq_cst);
    flush_worker_.join();
  }

private:
  std::string format_time(const std::chrono::system_clock::time_point &tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                            tp.time_since_epoch()) %
                        1000;

    std::tm tm = *std::localtime(&time_t);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);

    // 添加毫秒部分
    char result[120];
    snprintf(result, sizeof(result), "%s.%03lld", buffer, milliseconds.count());
    return result;
  }

  void format(std::ofstream &out, LogItemPtr log_item) {
    out << std::format("{0} | {1:<5} | thread[{2}] | {3}:{4} | {5}\n",
                       level2string(log_item->level),
                       format_time(log_item->time), log_item->thread_id,
                       log_item->file, log_item->line, log_item->msg);
  }

  void flush_task() {
    std::ofstream out(filename_, std::ios::app);
    size_t item_count = 0;
    LogItemPtr log_item = nullptr;
    while (true) {
      backoff b;
      while (!pop(log_item)) {
        b.snooze();
        if (exit_.load(std::memory_order_seq_cst)) {
          return;
        }
      }
      format(out, std::move(log_item));
      if (++item_count >= flush_freq_) {
        item_count = 0;
        out.flush();
      }
    }
  }
};

inline auto global_logger = std::make_unique<GlobalLogger>("1.log");

// *************************** LocalLogger ***************************

class LocalLogger {
private:
  using LogItemPtr = GlobalLogger::LogItemPtr;
  bool exit_;
  std::unique_ptr<SPSCQueue<LogItemPtr, BufferSize>> local_log_buffer_;
  std::thread transfer_worker_;
  LogLevel level_;

  void transfer_task() {
    while (true) {
      LogItemPtr log_item = nullptr;
      if (local_log_buffer_->pop(log_item)) {
        backoff b;
        while (!global_logger->push(std::move(log_item))) {
          b.snooze();
        }
      } else {
        if (exit_) {
          return;
        }
      }
    }
  }

public:
  explicit LocalLogger(LogLevel level = LogLevel::Unknown)
      : exit_(false), local_log_buffer_(new SPSCQueue<LogItemPtr, BufferSize>),
        transfer_worker_([this] { transfer_task(); }), level_(level) {}

  ~LocalLogger() {
    exit_ = true;
    transfer_worker_.join();
  }

  void set_log_level(LogLevel level) { level_ = level; }

  void log(const LogLevel& level, const std::chrono::system_clock::time_point& time,
           const std::thread::id& tid, const std::string &file, const int line,
           const std::string &msg) {
    if (level >= level_) {
      LogItemPtr item(new LogItem(level, time, tid, file, line, msg));
      while (!local_log_buffer_->push(std::move(item))) {
      }
    }
  }

  bool should_log(LogLevel level) const {
    return (static_cast<int>(level) >= static_cast<int>(level_));
  }
};

inline thread_local auto local_logger = std::make_unique<LocalLogger>();

} // namespace flog

#define FLOG_LEVEL(level, msg)                                                 \
  do {                                                                         \
    if (flog::local_logger->should_log(level)) [[likely]] {                    \
      flog::local_logger->log(level, std::chrono::system_clock::now(),         \
                              std::this_thread::get_id(), __FILE__, __LINE__,  \
                              msg);                                            \
    }                                                                          \
  } while (0)
#define FLOG_DEBUG(msg) FLOG_LEVEL(flog::LogLevel::Debug, msg)
#define FLOG_INFO(msg) FLOG_LEVEL(flog::LogLevel::Info, msg)
#define FLOG_WARN(msg) FLOG_LEVEL(flog::LogLevel::Warn, msg)
#define FLOG_ERROR(msg) FLOG_LEVEL(flog::LogLevel::Error, msg)
#define FLOG_FATAL(msg) FLOG_LEVEL(flog::LogLevel::Fatal, msg)

#define SET_LOG_LEVEL(level)                                                   \
  do {                                                                         \
    flog::local_logger->set_log_level(level);                                  \
  } while (0)
#define SET_LOG_LEVEL_UNKNOWN SET_LOG_LEVEL(flog::LogLevel::Unknown)
#define SET_LOG_LEVEL_DEBUG SET_LOG_LEVEL(flog::LogLevel::Debug)
#define SET_LOG_LEVEL_INFO SET_LOG_LEVEL(flog::LogLevel::Info)
#define SET_LOG_LEVEL_WARN SET_LOG_LEVEL(flog::LogLevel::Warn)
#define SET_LOG_LEVEL_ERROR SET_LOG_LEVEL(flog::LogLevel::Error)
#define SET_LOG_LEVEL_FATAL SET_LOG_LEVEL(flog::LogLevel::Fatal)

#endif