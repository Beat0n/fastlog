#include "fastlog/logger.h"

namespace fastlog {
namespace details {
// Define and initialize the global log level.
// Default to Trace to show all messages initially.
std::atomic<LogLevel> g_log_level{LogLevel::Trace};
}  // namespace details
}  // namespace fastlog