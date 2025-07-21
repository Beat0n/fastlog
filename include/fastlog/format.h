#ifndef FASTLOG_FORMAT_H
#define FASTLOG_FORMAT_H

#include <fmt/core.h>
#include <sstream>
#include <thread>

// --- Teach {fmt} how to format std::thread::id ---

template <> struct fmt::formatter<std::thread::id> {
  // parse function is fine as it is.
  constexpr auto parse(format_parse_context &ctx)
      const { // It's good practice to make parse const too
    return ctx.begin();
  }

  // The core formatting function.
  // It must be const, as {fmt} will call it on a const formatter object.
  template <typename FormatContext>
  auto format(const std::thread::id &id,
              FormatContext &ctx) const { // <-- 添加 const
    std::stringstream ss;
    ss << id;

    // Output the resulting string to the formatting context.
    // The original implementation had a small inefficiency here.
    // We can write directly to the context's output iterator.
    return std::copy(ss.str().begin(), ss.str().end(), ctx.out());
  }
};

#endif // FASTLOG_FORMAT_H