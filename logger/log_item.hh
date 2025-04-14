#ifndef FLOG_LOG_ITEM_HH
#define FLOG_LOG_ITEM_HH

#include <chrono>
#include <string>
#include <thread>

#include "log_level.hh"

namespace flog {

struct LogItem {
  LogLevel level;
  std::chrono::system_clock::time_point time;
  std::string thread_id;
  std::string file;
  int line;
  std::string msg;

  LogItem(LogLevel level, std::chrono::system_clock::time_point time,
          std::thread::id tid, std::string file, int line, std::string msg)
      : level(level), time(time), thread_id(tid2str(tid)), file(file), line(line),
        msg(msg) {}

  std::string tid2str(const std::thread::id& tid) const {
    std::ostringstream oss;
    oss << tid;
    return oss.str();
  }
};

} // namespace flog

#endif