#include "internal/server/tcp.h"

#include <cerrno>
#include <system_error>
#include <utility>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// macOS has no MSG_NOSIGNAL, it uses the SO_NOSIGPIPE socket option instead (set in Accept)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace server {

namespace {

[[noreturn]] void throwErrno(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}

} // namespace

Conn::Conn(int fd) : fd(fd) {}

Conn::~Conn() {
    Close();
}

Conn::Conn(Conn&& other) noexcept : fd(std::exchange(other.fd, -1)) {}

Conn& Conn::operator=(Conn&& other) noexcept {
    if (this != &other) {
        Close();
        fd = std::exchange(other.fd, -1);
    }
    return *this;
}

std::size_t Conn::Read(char* p, std::size_t n) {
    for (;;) {
        ssize_t got = recv(fd, p, n, 0);
        if (got >= 0) {
            return static_cast<std::size_t>(got);
        }
        if (errno != EINTR) {
            throwErrno("recv");
        }
    }
}

void Conn::Write(std::string_view data) {
    while (!data.empty()) {
        // MSG_NOSIGNAL: writing to a closed client must return an error, not kill the whole process with SIGPIPE
        ssize_t sent = send(fd, data.data(), data.size(), MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            throwErrno("send");
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

void Conn::Close() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

Listener::Listener(int fd) : fd(fd) {}

Listener::~Listener() {
    if (fd >= 0) {
        close(fd);
    }
}

Listener::Listener(Listener&& other) noexcept : fd(std::exchange(other.fd, -1)) {}

Conn Listener::Accept() {
    for (;;) {
        int clientFd = accept(fd, nullptr, nullptr);
        if (clientFd >= 0) {
#ifdef SO_NOSIGPIPE
            int one = 1;
            setsockopt(clientFd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
            return Conn(clientFd);
        }
        if (errno != EINTR) {
            throwErrno("accept");
        }
    }
}

Listener Listen(std::uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throwErrno("socket");
    }
    // own the fd right away so it gets closed if bind/listen throws
    Listener listener(fd);

    // lets us restart the server immediately instead of waiting for TIME_WAIT to clear
    int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        throwErrno("setsockopt");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throwErrno("bind");
    }
    if (listen(fd, SOMAXCONN) < 0) {
        throwErrno("listen");
    }
    return listener;
}

} // namespace server
