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

using namespace std;

void handleConnection(server::Conn conn, storage::Engine& db);

int main() {
    // (each line is built first and written in one call, so lines from different clients don't interleave)
    cout << unitbuf;

    // init central storage engine once
    storage::Engine db;

    // if we can't listen, the uncaught exception aborts with the reason
    server::Listener listener = server::Listen(6379);

    cout << "Mini-Redis listening on port 6379\n";

    for (;;) {
        try {
            server::Conn conn = listener.Accept();
            // pass the shared engine to every client
            thread(handleConnection, move(conn), ref(db)).detach();
        } catch (const system_error& err) {
            cout << (string("Error accepting: ") + err.what() + "\n");
        }
    }
}

namespace {

string toUpper(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });
    return s;
}

// quoteArgs formats args on a []string: ["SET" "hello" "vedansh"]
string quoteArgs(const vector<string>& args) {
    string out = "[";
    for (size_t i = 0; i < args.size(); i++) {
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
            } else if (isprint(c)) {
                out += static_cast<char>(c);
            } else {
                char hex[5];
                snprintf(hex, sizeof(hex), "\\x%02x", c);
                out += hex;
            }
        }
        out += '"';
    }
    return out + "]";
}

} // namespace

void handleConnection(server::Conn conn, storage::Engine& db) {
    cout << "New client connected.\n";
    // no defer needed: conn closes its socket when this function returns

    bufio::Reader reader(conn); // buffered reader because raw network sockets read in tiny chunks
    resp::Parser parser(reader);

    // an exception escaping a thread would kill the whole server, so everything is caught here
    try {
        for (;;) {
            vector<string> args;
            try {
                args = parser.Parse();
            } catch (const resp::ProtocolError& err) {
                // after garbage input we can't find the next command boundary, so reply and hang up (like real Redis)
                conn.Write(string("-ERR Protocol error: ") + err.what() + "\r\n");
                cout << (string("Client disconnected on error: ") + err.what() + "\n");
                return;
            }
            // ignore empty commands
            if (args.empty()) {
                continue;
            }

            // convert command to uppercase to make it case insensitive
            string cmd = toUpper(args[0]);

            if (cmd == "PING") {
                conn.Write("+PONG\r\n");

            } else if (cmd == "SET") {
                if (args.size() != 3) { // [SET, "hello", "vedansh"]
                    conn.Write("-ERR wrong number of arguments for 'set' command\r\n");
                    continue;
                }
                db.Set(args[1], args[2]);
                conn.Write("+OK\r\n");

            } else if (cmd == "GET") {
                if (args.size() != 2) {
                    conn.Write("-ERR wrong number of arguments for 'get' command\r\n");
                    continue;
                }
                optional<string> val = db.Get(args[1]);
                if (!val) {
                    // "$-1\r\n" is the exact RESP protocol for a Null String (key not found)
                    conn.Write("$-1\r\n");
                    continue;
                }
                // respond with a bulk string containing the value
                conn.Write("$" + to_string(val->size()) + "\r\n" + *val + "\r\n");

            } else if (cmd == "DEL") {
                if (args.size() < 2) {
                    conn.Write("-ERR wrong number of arguments for 'del' command\r\n");
                    continue;
                }
                // DEL key [key ...] replies with how many keys were actually removed
                int deleted = 0;
                for (size_t i = 1; i < args.size(); i++) {
                    if (db.Delete(args[i])) {
                        deleted++;
                    }
                }
                conn.Write(":" + to_string(deleted) + "\r\n"); // RESP Integer

            } else {
                conn.Write("-ERR unknown command '" + cmd + "'\r\n");
            }
            cout << ("Received Command: " + quoteArgs(args) + "\n");
        }
    } catch (const exception& err) {
        cout << (string("Client disconnected on error: ") + err.what() + "\n");
    }
}

