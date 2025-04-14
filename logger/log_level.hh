#ifndef FLOG_LOG_LEVEL_HH
#define FLOG_LOG_LEVEL_HH

#include <string>

namespace flog {

enum class LogLevel {
  Unknown,
  Debug,
  Info,
  Warn,
  Error,
  Fatal,
};

inline std::string level2string(LogLevel level) {
  static std::pair<LogLevel, const char *> enum_names[6]={
#define ENUM_NAME(name) std::make_pair(LogLevel::name, #name)
      ENUM_NAME(Unknown), ENUM_NAME(Debug), ENUM_NAME(Info),
      ENUM_NAME(Warn),    ENUM_NAME(Error), ENUM_NAME(Fatal),
  };

  return std::string(enum_names[static_cast<size_t>(level)].second);
}
} // namespace flog

#endif // FLOG_LOG_LEVEL_HH