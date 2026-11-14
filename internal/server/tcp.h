#pragma once

#include "internal/io/io.h"

#include <cstdint>
#include <string_view>

using namespace std;

// thin RAII wrappers over POSIX sockets, playing the role of Go's net.Listener / net.Conn
namespace server {

// Conn is one connected client; the socket is closed when the Conn is destroyed
class Conn : public io::Reader {
public:
    explicit Conn(int fd);
    ~Conn() override;

    Conn(Conn&& other) noexcept;
    Conn& operator=(Conn&& other) noexcept;
    Conn(const Conn&) = delete;
    Conn& operator=(const Conn&) = delete;

    // Read reads up to n bytes from the socket, 0 means the client hung up
    size_t Read(char* p, size_t n) override;

    // Write sends all of data, looping over partial sends
    void Write(string_view data);

    void Close();

private:
    int fd;
};

// Listener accepts incoming TCP connections
class Listener {
public:
    explicit Listener(int fd);
    ~Listener();

    Listener(Listener&& other) noexcept;
    Listener& operator=(Listener&&) = delete;
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    // Accept blocks until the next client connects
    Conn Accept();

private:
    int fd;
};

// Listen opens a TCP listener on all interfaces at the given port (like net.Listen("tcp", ":port"))
Listener Listen(uint16_t port);

} // namespace server

