#pragma once

#include "internal/io/io.h"

#include <cstddef>
#include <stdexcept>
#include <string>

using namespace std;

// minimal version of Go's bufio.Reader
namespace bufio {

// BufferFullError is thrown when a line grows past the reader's limit without a delimiter
class BufferFullError : public runtime_error {
public:
    BufferFullError() : runtime_error("bufio: buffer full") {}
};

// Reader wraps an io::Reader so we can read whole lines / exact byte counts
// instead of whatever tiny chunks the network hands us
class Reader {
public:
    static constexpr size_t defaultMaxLineSize = 64 * 1024;

    explicit Reader(io::Reader& rd, size_t maxLineSize = defaultMaxLineSize);

    // ReadBytes reads until the first occurrence of delim, returning the data including delim
    string ReadBytes(char delim);

    // ReadFull reads exactly n bytes (like Go's io.ReadFull)
    string ReadFull(size_t n);

private:
    // fill pulls another chunk from the underlying reader; returns false at end of stream
    bool fill();
    size_t buffered() const { return buf.size() - r; }
    void consume(size_t n);

    io::Reader& rd;
    size_t maxLineSize;
    string buf;   // bytes read from rd but not yet handed out
    size_t r = 0; // read position inside buf
};

} // namespace bufio

