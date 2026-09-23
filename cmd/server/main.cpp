#include "internal/bufio/reader.h"
#include "internal/resp/parser.h"
#include "internal/server/tcp.h"
#include "internal/storage/engine.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

void handleConnection(server::Conn conn, storage::Engine& db);

int main() {
    // flush every log line right away, like Go's fmt.Println
    // (each line is built first and written in one call, so lines from different clients don't interleave)
    std::cout << std::unitbuf;

    // init central storage engine once
    storage::Engine db;

    // like Go's panic(err): if we can't listen, the uncaught exception aborts with the reason
    server::Listener listener = server::Listen(6379);

    std::cout << "Mini-Redis listening on port 6379\n";

    for (;;) {
        try {
            server::Conn conn = listener.Accept();
            // pass the shared engine to every client
            std::thread(handleConnection, std::move(conn), std::ref(db)).detach();
        } catch (const std::system_error& err) {
            std::cout << (std::string("Error accepting: ") + err.what() + "\n");
        }
    }
}

namespace {

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

// quoteArgs formats args like Go's %q on a []string: ["SET" "hello" "vedansh"]
std::string quoteArgs(const std::vector<std::string>& args) {
    std::string out = "[";
    for (std::size_t i = 0; i < args.size(); i++) {
        if (i > 0) {
            out += ' ';
        }
        out += '"';
        for (unsigned char c : args[i]) {
            if (c == '"' || c == '\\') {
                out += '\\';
                out += static_cast<char>(c);
            } else if (c == '\r') {
                out += "\\r";
            } else if (c == '\n') {
                out += "\\n";
            } else if (std::isprint(c)) {
                out += static_cast<char>(c);
            } else {
                char hex[5];
                std::snprintf(hex, sizeof(hex), "\\x%02x", c);
                out += hex;
            }
        }
        out += '"';
    }
    return out + "]";
}

} // namespace

void handleConnection(server::Conn conn, storage::Engine& db) {
    std::cout << "New client connected.\n";
    // no defer needed: conn closes its socket when this function returns

    bufio::Reader reader(conn); // buffered reader because raw network sockets read in tiny chunks
    resp::Parser parser(reader);

    // an exception escaping a thread would kill the whole server, so everything is caught here
    try {
        for (;;) {
            std::vector<std::string> args;
            try {
                args = parser.Parse();
            } catch (const resp::ProtocolError& err) {
                // after garbage input we can't find the next command boundary, so reply and hang up (like real Redis)
                conn.Write(std::string("-ERR Protocol error: ") + err.what() + "\r\n");
                std::cout << (std::string("Client disconnected on error: ") + err.what() + "\n");
                return;
            }
            // ignore empty commands
            if (args.empty()) {
                continue;
            }

            // convert command to uppercase to make it case insensitive
            std::string command = toUpper(args[0]);

            if (command == "PING") {
                conn.Write("+PONG\r\n");

            } else if (command == "SET") {
                if (args.size() != 3) { // [SET, "hello", "vedansh"]
                    conn.Write("-ERR wrong number of arguments for 'set' command\r\n");
                    continue;
                }
                db.Set(args[1], args[2]);
                conn.Write("+OK\r\n");

            } else if (command == "GET") {
                if (args.size() != 2) {
                    conn.Write("-ERR wrong number of arguments for 'get' command\r\n");
                    continue;
                }
                std::optional<std::string> val = db.Get(args[1]);
                if (!val) {
                    // "$-1\r\n" is the exact RESP protocol for a Null String (key not found)
                    conn.Write("$-1\r\n");
                    continue;
                }
                // respond with a bulk string containing the value
                conn.Write("$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n");

            } else if (command == "DEL") {
                if (args.size() < 2) {
                    conn.Write("-ERR wrong number of arguments for 'del' command\r\n");
                    continue;
                }
                // DEL key [key ...] replies with how many keys were actually removed
                int deleted = 0;
                for (std::size_t i = 1; i < args.size(); i++) {
                    if (db.Delete(args[i])) {
                        deleted++;
                    }
                }
                conn.Write(":" + std::to_string(deleted) + "\r\n"); // RESP Integer

            } else {
                conn.Write("-ERR unknown command '" + command + "'\r\n");
            }
            std::cout << ("Received Command: " + quoteArgs(args) + "\n");
        }
    } catch (const std::exception& err) {
        std::cout << (std::string("Client disconnected on error: ") + err.what() + "\n");
    }
}
