#ifndef FASTLOG_SINKS_SINK_H
#define FASTLOG_SINKS_SINK_H

#include <string_view>

namespace fastlog {
namespace sinks {

class Sink {
public:
    virtual ~Sink() = default;

    // The core method for a sink to log a pre-formatted message.
    virtual void log(std::string_view message) = 0;

    // A method to ensure all buffered data is written to the destination.
    virtual void flush() = 0;
};

} // namespace sinks
} // namespace fastlog

#endif // FASTLOG_SINKS_SINK_H