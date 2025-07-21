#include "fastlog/formatters/pattern_formatter.h"
#include <fmt/chrono.h> // For fmt's time formatting
#include <functional>
#include <map>

namespace fastlog {
namespace formatters {

namespace { // Anonymous namespace for implementation details

// --- Flag Formatter Implementations ---

// Formats the year (e.g., 2023)
class YearFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:%Y}", msg.timestamp);
  }
};

// Formats the month (01-12)
class MonthFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:%m}", msg.timestamp);
  }
};

// Formats the day (01-31)
class DayFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:%d}", msg.timestamp);
  }
};

class HourFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:%H}", msg.timestamp);
  }
};

class MinuteFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:%M}", msg.timestamp);
  }
};

class SecondFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:%S}", msg.timestamp);
  }
};

// Formats microseconds
class MicrosecondsFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  msg.timestamp.time_since_epoch()) %
              1000000;
    fmt::format_to(std::back_inserter(dest), "{:06}", us.count());
  }
};

// Formats the log level
class LevelFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    dest.append(level2string(msg.level));
  }
};

// Formats the thread ID
class ThreadIdFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    fmt::format_to(std::back_inserter(dest), "{}", msg.thread_id);
  }
};

// Formats the log message payload
class PayloadFormatter : public FlagFormatter {
public:
  void format(const LogMessage &msg, std::string &dest) override {
    dest.append(msg.payload);
  }
};

// Formats a literal string from the pattern
class LiteralFormatter : public FlagFormatter {
public:
  explicit LiteralFormatter(std::string literal)
      : _literal(std::move(literal)) {}
  void format(const LogMessage &, std::string &dest) override {
    dest.append(_literal);
  }

private:
  std::string _literal;
};

} // anonymous namespace

PatternFormatter::PatternFormatter(std::string pattern)
    : _pattern(std::move(pattern)) {
  compile_pattern(_pattern);
}

void PatternFormatter::format(const LogMessage &msg, std::string &dest) {
  for (const auto &formatter : _flag_formatters) {
    formatter->format(msg, dest);
  }
}

void PatternFormatter::compile_pattern(const std::string &pattern) {
  // A simple map from pattern flag to a factory function that creates the
  // formatter.
  const std::map<char, std::function<std::unique_ptr<FlagFormatter>()>>
      flag_map = {
          {'Y', []() { return std::make_unique<YearFormatter>(); }},
          {'m', []() { return std::make_unique<MonthFormatter>(); }},
          {'d', []() { return std::make_unique<DayFormatter>(); }},
          {'H', []() { return std::make_unique<HourFormatter>(); }},
          {'M', []() { return std::make_unique<MinuteFormatter>(); }},
          {'S', []() { return std::make_unique<SecondFormatter>(); }},
          {'f', []() { return std::make_unique<MicrosecondsFormatter>(); }},
          {'l', []() { return std::make_unique<LevelFormatter>(); }},
          {'t', []() { return std::make_unique<ThreadIdFormatter>(); }},
          {'v', []() { return std::make_unique<PayloadFormatter>(); }}};

  std::string literal_buffer;
  for (size_t i = 0; i < pattern.length(); ++i) {
    if (pattern[i] == '%') {
      if (!literal_buffer.empty()) {
        _flag_formatters.push_back(
            std::make_unique<LiteralFormatter>(std::move(literal_buffer)));
        literal_buffer.clear();
      }
      if (i + 1 < pattern.length()) {
        char flag = pattern[i + 1];
        if (flag == '%') { // Handle "%%"
          literal_buffer += '%';
        } else {
          auto it = flag_map.find(flag);
          if (it != flag_map.end()) {
            _flag_formatters.push_back(it->second());
          }
        }
        i++; // Skip the flag character
      }
    } else {
      literal_buffer += pattern[i];
    }
  }

  if (!literal_buffer.empty()) {
    _flag_formatters.push_back(
        std::make_unique<LiteralFormatter>(std::move(literal_buffer)));
  }
}

} // namespace formatters
} // namespace fastlog