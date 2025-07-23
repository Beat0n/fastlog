#ifndef FASTLOG_FORMAT_H
#define FASTLOG_FORMAT_H

#include <fmt/core.h>

#include <sstream>
#include <thread>

// --- Teach {fmt} how to format std::thread::id ---

template <>
struct fmt::formatter<std::thread::id> {
  // parse function is fine as it is.
  constexpr auto parse(format_parse_context &ctx) const {
    return ctx.begin();
  }

  // The core formatting function.
  // It must be const, as {fmt} will call it on a const formatter object.
  template <typename FormatContext>
  auto format(const std::thread::id &id, FormatContext &ctx) const {
    std::stringstream ss;
    ss << id;
    const std::string str = ss.str();
    return std::copy(str.begin(), str.end(), ctx.out());
  }
};

#endif  // FASTLOG_FORMAT_H