#ifndef FASTLOG_SINKS_FILE_SINK_H
#define FASTLOG_SINKS_FILE_SINK_H

#include "fastlog/sinks/sink.h"
#include <memory>
#include <string>

// To forward-declare std::ofstream correctly without including the heavy
// <fstream>, we include the lightweight <iosfwd> header. This is the
// standard-compliant way.
#include <iosfwd>

namespace fastlog {
namespace sinks {

class FileSink : public Sink {
public:
  explicit FileSink(const std::string &filename, bool truncate = false);

  // The destructor needs the full definition of std::ofstream to compile.
  // Since we are using Pimpl, the full definition is only in the .cpp file.
  // Therefore, the destructor *must* be defined in the .cpp file.
  // We declare it here, and "= default" it in the .cpp file.
  ~FileSink() override;

  // --- Deleted members ---
  // A file sink is unique and owns a file handle. It shouldn't be copied.
  // Moving it requires careful implementation of transferring file ownership,
  // which we will disable for simplicity for now.
  FileSink(const FileSink &) = delete;
  FileSink &operator=(const FileSink &) = delete;
  FileSink(FileSink &&) = delete;
  FileSink &operator=(FileSink &&) = delete;

  // --- Interface implementation ---
  void log(std::string_view message) override;
  void flush() override;

private:
  // This is the Pimpl pattern. The unique_ptr holds the pointer to the
  // implementation detail (the file stream), hiding it from the header file
  // user.
  std::unique_ptr<std::ofstream> _file_stream;
};

} // namespace sinks
} // namespace fastlog

#endif // FASTLOG_SINKS_FILE_SINK_H