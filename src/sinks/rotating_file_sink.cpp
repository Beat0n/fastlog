#include "fastlog/sinks/rotating_file_sink.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <system_error> // For std::error_code in filesystem

namespace fastlog {
namespace sinks {

namespace fs = std::filesystem;

RotatingFileSink::RotatingFileSink(std::string base_filename,
                                   size_t max_file_size, size_t max_files)
    : _base_filename(std::move(base_filename)), _max_file_size(max_file_size),
      _max_files(max_files) {
  if (max_files == 0) {
    throw std::invalid_argument("max_files must be > 0 in RotatingFileSink");
  }
  if (_max_file_size == 0) {
    throw std::invalid_argument(
        "max_file_size must be > 0 in RotatingFileSink");
  }
  open_new_file();
}

void RotatingFileSink::log(std::string_view message) {
  // Check if the *next* write will exceed the size limit.
  // Also check if _current_size > 0 to avoid rotating an already empty file.
  if (_current_size > 0 &&
      (_current_size + message.length() + 1 > _max_file_size)) {
    rotate();
  }

  _file_sink->log(message);
  _current_size += message.length() + 1; // +1 for the newline character
}

void RotatingFileSink::flush() { _file_sink->flush(); }

void RotatingFileSink::open_new_file() {
  // This function must either succeed and assign a valid sink, or throw.
  _file_sink = std::make_unique<FileSink>(_base_filename, true /* truncate */);

  // Reset current size.
  std::error_code ec;
  _current_size = fs::file_size(_base_filename, ec);
  if (ec) { // If there's an error (e.g., file not found), size is 0.
    _current_size = 0;
  }
}

void RotatingFileSink::rotate() {
  // 1. Close the current file by destroying the old sink.
  //    This also flushes the file, thanks to FileSink's destructor.
  _file_sink.reset();

  // Helper lambda for robust file operations, printing errors to stderr.
  auto handle_fs_error = [](const std::error_code &ec, const std::string &msg) {
    if (ec) {
      std::cerr << "fastlog: " << msg << " - " << ec.message() << std::endl;
    }
  };

  // 2. Remove the oldest file if it exists, to make space for the new backup.
  fs::path oldest_file = _base_filename + "." + std::to_string(_max_files);
  std::error_code ec;
  fs::remove(oldest_file, ec);
  // We don't need to log an error if the file didn't exist (ec value would be
  // file_not_found).

  // 3. Rename files in reverse order: "log.2" -> "log.3", "log.1" -> "log.2"
  // etc.
  //    This avoids overwriting files during the chain rename.
  for (size_t i = _max_files - 1; i > 0; --i) {
    fs::path src = _base_filename + "." + std::to_string(i);
    fs::path dest = _base_filename + "." + std::to_string(i + 1);

    // We only try to rename if the source file actually exists.
    fs::rename(src, dest, ec);
    if (ec && ec != std::errc::no_such_file_or_directory) {
      handle_fs_error(ec, "failed to rename " + src.string() + " to " +
                              dest.string());
    }
    ec.clear(); // Clear error code for the next iteration.
  }

  // 4. Rename the main log file to the first backup: "log" -> "log.1"
  fs::path src = _base_filename;
  fs::path dest_1 = _base_filename + ".1";
  fs::rename(src, dest_1, ec);
  if (ec && ec != std::errc::no_such_file_or_directory) {
    handle_fs_error(ec, "failed to rename " + src.string() + " to " +
                            dest_1.string());
  }

  // 5. Open a new, empty file for subsequent logs.
  //    This re-establishes the class invariant.
  open_new_file();
}

} // namespace sinks
} // namespace fastlog