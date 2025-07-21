#include "fastlog/sinks/file_sink.h"
#include <fstream>
#include <stdexcept>

namespace fastlog {
namespace sinks {

FileSink::FileSink(const std::string &filename, bool truncate) {
  auto open_mode = std::ios::app;
  if (truncate) {
    open_mode = std::ios::trunc;
  }
  _file_stream = std::make_unique<std::ofstream>();
  _file_stream->open(filename, open_mode);

  // Establish the class invariant: the file MUST be open.
  if (!_file_stream->is_open()) {
    throw std::runtime_error("Failed to open log file: " + filename);
  }
}

FileSink::~FileSink() {
  // Relying on the invariant that _file_stream is valid.
  flush();
}

void FileSink::log(std::string_view message) {
  // No check needed. We assume _file_stream is valid due to the invariant
  // established in the constructor. Dereferencing a null pointer here
  // would indicate a bug elsewhere (e.g., in move semantics) and should crash.
  _file_stream->write(message.data(), message.length());
  _file_stream->put('\n');
}

void FileSink::flush() {
  // No check needed. Assume valid stream.
  _file_stream->flush();
}

} // namespace sinks
} // namespace fastlog