#ifndef FASTLOG_FORMATTERS_PATTERN_FORMATTER_H
#define FASTLOG_FORMATTERS_PATTERN_FORMATTER_H

#include "fastlog/formatters/formatter.h" // Needs the full definition of the base class
#include "fastlog/log_message.h"
#include <memory>
#include <string>
#include <vector>

namespace fastlog {
namespace formatters {

// Represents a single piece of the format pattern (e.g., a timestamp, or
// literal text)
class FlagFormatter {
public:
  virtual ~FlagFormatter() = default;
  virtual void format(const LogMessage &msg, std::string &dest) = 0;
};

class PatternFormatter : public Formatter {
public:
  explicit PatternFormatter(std::string pattern);

  void format(const LogMessage &msg, std::string &dest) override;

private:
  void compile_pattern(const std::string &pattern);

  std::string _pattern;
  std::vector<std::unique_ptr<FlagFormatter>> _flag_formatters;
};

} // namespace formatters
} // namespace fastlog

#endif // FASTLOG_FORMATTERS_PATTERN_FORMATTER_H