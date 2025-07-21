#ifndef FASTLOG_FORMATTERS_FORMATTER_H
#define FASTLOG_FORMATTERS_FORMATTER_H

#include "fastlog/log_message.h"
#include <string>

namespace fastlog {
namespace formatters {

class Formatter {
public:
  virtual ~Formatter() = default;

  /**
   * @brief Formats a LogMessage into a destination string.
   *
   * @param msg The log message to format.
   * @param dest The destination string to append the formatted output to.
   *             Appending to a string avoids repeated allocations.
   */
  virtual void format(const LogMessage &msg, std::string &dest) = 0;
};

} // namespace formatters
} // namespace fastlog

#endif // FASTLOG_FORMATTERS_FORMATTER_H