#include <fastlog/formatters/pattern_formatter.h>
#include <fastlog/logger.h>
#include <fastlog/sinks/rotating_file_sink.h>
#include <fastlog/sinks/stdout_sink.h>
#include <unistd.h>
#include <memory>

void some_function() { FASTLOG_DEBUG("This is a debug message from a specific function."); }

int main() {
  // --- Setup a console sink for demonstration ---
  fastlog::details::add_sink<fastlog::sinks::StdoutSink>();
  auto fancy_formatter = std::make_unique<fastlog::formatters::PatternFormatter>("%^[%Y-%M-%D %H:%M:%S] [%T] [%F:%l %f] %v%$");
  fastlog::details::set_formatter(std::move(fancy_formatter));
  // std::cout << "--- Example 1: Log Level Control ---\n";

  fastlog::details::set_level(fastlog::LogLevel::Info);
  // std::cout << "Log level set to INFO. TRACE and DEBUG messages will be ignored.\n";

  FASTLOG_TRACE("This trace message will NOT be seen.");
  FASTLOG_DEBUG("This debug message will also NOT be seen.");
  FASTLOG_INFO("This info message WILL be seen.");
  FASTLOG_WARN("This warn message WILL be seen.");

  fastlog::details::set_level(fastlog::LogLevel::Trace);
  // std::cout << "\nLog level set back to TRACE. All messages will be shown.\n";
  FASTLOG_TRACE("This trace message will now be visible.");

  // std::cout << "\n--- Example 2: Enhanced Formatter ---\n";

  // Create a new formatter with color and function name
  // %^ starts color, %$ ends it. %C is the function name.
  // auto fancy_formatter = std::make_unique<fastlog_formatters::PatternFormatter>("%^[%Y-%m-%d %H:%M:%S] [%l] [%t] [%C] %v%$");
  // fastlog_set_formatter(std::move(fancy_formatter));

  FASTLOG_INFO("This is an info message in green.");
  FASTLOG_WARN("A warning message in yellow.");
  FASTLOG_ERROR("An error message in red.");

  some_function();

  // The logger will shut down automatically on exit.
  return 0;
}