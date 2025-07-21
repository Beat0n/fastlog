#ifndef FASTLOG_SINKS_ROTATING_FILE_SINK_H
#define FASTLOG_SINKS_ROTATING_FILE_SINK_H

#include "fastlog/sinks/file_sink.h" // We can reuse FileSink's logic
#include <cstddef>
#include <string>

namespace fastlog {
namespace sinks {

/**
 * @brief A file sink that rotates logs. When the current log file reaches
 *        a certain size, it is renamed, and a new log file is created.
 */
class RotatingFileSink : public Sink {
public:
  /**
   * @param base_filename The base name for the log file, e.g., "my_app.log".
   * @param max_file_size The maximum size in bytes for a single log file.
   * @param max_files The maximum number of rotated files to keep (e.g.,
   * "my_app.1.log", "my_app.2.log").
   */
  RotatingFileSink(std::string base_filename, size_t max_file_size,
                   size_t max_files);
  ~RotatingFileSink() override = default;

  void log(std::string_view message) override;
  void flush() override;

private:
  void rotate(); // The core rotation logic
  void open_new_file();

  std::string _base_filename;
  size_t _max_file_size;
  size_t _max_files;
  size_t _current_size = 0;

  // We can use a simple FileSink internally to handle the actual file I/O.
  // This demonstrates composition over inheritance.
  std::unique_ptr<FileSink> _file_sink;
};

} // namespace sinks
} // namespace fastlog

#endif // FASTLOG_SINKS_ROTATING_FILE_SINK_H