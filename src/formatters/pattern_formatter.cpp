#include "fastlog/formatters/pattern_formatter.h"

#include <fmt/chrono.h>

#include <functional>
#include <map>
#include <string>

#include "fastlog/format.h"

namespace fastlog {
namespace formatters {

namespace {  // Anonymous namespace for implementation details

// ANSI escape codes for colors
constexpr const char *COLOR_RESET = "\x1b[0m";
constexpr const char *COLOR_RED = "\x1b[31m";
constexpr const char *COLOR_GREEN = "\x1b[32m";
constexpr const char *COLOR_YELLOW = "\x1b[33m";
constexpr const char *COLOR_BLUE = "\x1b[34m";
constexpr const char *COLOR_MAGENTA = "\x1b[35m";
constexpr const char *COLOR_CYAN = "\x1b[36m";
constexpr const char *COLOR_BOLD_RED = "\x1b[1;31m";

// Map LogLevel to color codes
constexpr const char *level2color(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return COLOR_MAGENTA;
    case LogLevel::Debug:
      return COLOR_CYAN;
    case LogLevel::Info:
      return COLOR_GREEN;
    case LogLevel::Warn:
      return COLOR_YELLOW;
    case LogLevel::Error:
      return COLOR_RED;
    case LogLevel::Fatal:
      return COLOR_BOLD_RED;
    default:
      return COLOR_RESET;
  }
}
// --- Flag Formatter Implementations ---

// Formats the year (e.g., 2023)
class YearFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{:%Y}", fmt::localtime(msg.timestamp)); }
};

// Formats the month (01-12)
class MonthFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{:%m}", fmt::localtime(msg.timestamp)); }
};

// Formats the day (01-31)
class DayFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{:%d}", fmt::localtime(msg.timestamp)); }
};

class HourFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{:%H}", fmt::localtime(msg.timestamp)); }
};

class MinuteFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{:%M}", fmt::localtime(msg.timestamp)); }
};

class SecondFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{:%S}", fmt::localtime(msg.timestamp)); }
};

// Formats the log level
class LevelFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { dest.append(level2string(msg.level)); }
};

// Formats the thread ID
class ThreadIdFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { fmt::format_to(std::back_inserter(dest), "{}", msg.thread_id); }
};

// Formats the log message payload
class PayloadFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { dest.append(msg.payload); }
};

// Formats a literal string from the pattern
class LiteralFormatter : public FlagFormatter {
 public:
  explicit LiteralFormatter(std::string literal) : _literal(std::move(literal)) {}
  void format(const LogMessage &, std::string &dest) const override { dest.append(_literal); }

 private:
  std::string _literal;
};

// Formats the function name
class FunctionNameFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { dest.append(msg.location.function_name()); }
};

class FilenameFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { dest.append(msg.location.file_name()); }
};

class LineFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { dest.append(std::to_string(msg.location.line())); }
};

// Appends the start-color code for the message's log level
class ColorStartFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &msg, std::string &dest) const override { dest.append(level2color(msg.level)); }
};

// Appends the color-reset code
class ColorEndFormatter : public FlagFormatter {
 public:
  void format(const LogMessage &, std::string &dest) const override { dest.append(COLOR_RESET); }
};

}  // anonymous namespace

PatternFormatter::PatternFormatter(std::string pattern) : _pattern(std::move(pattern)) { compile_pattern(_pattern); }

void PatternFormatter::format(const LogMessage &msg, std::string &dest) const {
  for (const auto &formatter : _flag_formatters) {
    formatter->format(msg, dest);
  }
}

void PatternFormatter::compile_pattern(const std::string &pattern) {
  // A simple map from pattern flag to a factory function that creates the formatter.
  const std::map<char, std::function<std::unique_ptr<FlagFormatter>()>> flag_map = {
      {'Y', []() { return std::make_unique<YearFormatter>(); }},
      {'M', []() { return std::make_unique<MonthFormatter>(); }},
      {'D', []() { return std::make_unique<DayFormatter>(); }},
      {'H', []() { return std::make_unique<HourFormatter>(); }},
      {'M', []() { return std::make_unique<MinuteFormatter>(); }},
      {'S', []() { return std::make_unique<SecondFormatter>(); }},
      {'L', []() { return std::make_unique<LevelFormatter>(); }},
      {'T', []() { return std::make_unique<ThreadIdFormatter>(); }},
      {'F', []() { return std::make_unique<FilenameFormatter>(); }},      // F for "filename"
      {'l', []() { return std::make_unique<LineFormatter>(); }},          // l for "line"
      {'f', []() { return std::make_unique<FunctionNameFormatter>(); }},  // f for "Function"
      {'v', []() { return std::make_unique<PayloadFormatter>(); }},
      {'^', []() { return std::make_unique<ColorStartFormatter>(); }},  // ^ for Start Color
      {'$', []() { return std::make_unique<ColorEndFormatter>(); }}     // $ for End Color
  };

  // The rest of your parsing loop remains exactly the same.
  // It will automatically handle the new flags because they are now in the map.
  std::string literal_buffer;
  for (size_t i = 0; i < pattern.length(); ++i) {
    if (pattern[i] == '%') {
      if (!literal_buffer.empty()) {
        _flag_formatters.push_back(std::make_unique<LiteralFormatter>(std::move(literal_buffer)));
        literal_buffer.clear();
      }
      if (i + 1 < pattern.length()) {
        char flag = pattern[i + 1];
        if (flag == '%') {  // Handle "%%"
          literal_buffer += '%';
        } else {
          auto it = flag_map.find(flag);
          if (it != flag_map.end()) {
            _flag_formatters.push_back(it->second());
          }
        }
        i++;  // Skip the flag character
      }
    } else {
      literal_buffer += pattern[i];
    }
  }

  if (!literal_buffer.empty()) {
    _flag_formatters.push_back(std::make_unique<LiteralFormatter>(std::move(literal_buffer)));
  }
}

}  // namespace formatters
}  // namespace fastlog