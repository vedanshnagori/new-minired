#pragma once

#include <cstddef>
#include <stdexcept>

// C++ stand-ins for the bits of Go's "io" package that miniRedis relies on
namespace io {

// Reader is anything we can pull raw bytes from (a socket, a string in tests...)
class Reader {
public:
    virtual ~Reader() = default;

    // Read reads up to n bytes into p and returns how many were read
    // returns 0 when the stream has ended (Go's io.EOF)
    virtual std::size_t Read(char* p, std::size_t n) = 0;
};

// EOFError is thrown when the stream ends before we got what we asked for
class EOFError : public std::runtime_error {
public:
    EOFError() : std::runtime_error("EOF") {}
};

} // namespace io
