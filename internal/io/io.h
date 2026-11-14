#pragma once

#include <cstddef>
#include <stdexcept>

using namespace std;

// C++ stand-ins for the bits of Go's "io" package that miniRedis relies on
namespace io {

// Reader is anything we can pull raw bytes from (a socket, a string in tests...)
class Reader {
public:
    virtual ~Reader() = default;

    // Read reads up to n bytes into p and returns how many were read
    // returns 0 when the stream has ended (Go's io.EOF)
    virtual size_t Read(char* p, size_t n) = 0;
};

// EOFError is thrown when the stream ends before we got what we asked for
class EOFError : public runtime_error {
public:
    EOFError() : runtime_error("EOF") {}
};

} // namespace io

