# miniRedis

A minimal, multi-threaded Redis server clone written in modern C++20, built as an **educational project** to explore network programming, concurrency, and protocol design at a low level.

It speaks the real RESP (REdis Serialization Protocol), so it works directly with the standard `redis-cli`.

## Features

- **TCP server over raw POSIX sockets** — one thread per connected client, with RAII wrappers around socket lifetimes (`server::Conn`, `server::Listener`)
- **Streaming RESP parser** (`resp::Parser`) — binary-safe, works over a buffered reader (`bufio::Reader`) instead of assuming a full command arrives in one read, with strict validation and length limits matching real Redis
- **Thread-safe in-memory storage engine** (`storage::Engine`) — a `std::unordered_map<std::string, std::string>` guarded by a `std::shared_mutex`, so multiple readers (`GET`) can proceed concurrently while writers (`SET` / `DEL`) get exclusive access
- **Supported commands**: `PING`, `SET`, `GET`, `DEL` (case-insensitive), with Redis-compatible error replies for malformed commands

## Project layout

```
cmd/server/main.cpp        # entry point: accepts connections, dispatches commands
internal/io/               # generic Reader interface
internal/bufio/            # buffered reader over a raw socket
internal/resp/             # RESP protocol parser
internal/server/           # TCP listener/connection RAII wrappers
internal/storage/          # thread-safe key-value engine
internal/aof/              # append-only-file persistence (not implemented yet)
tests/                     # GoogleTest unit tests
```

## Building

Requires CMake 3.20+ and a C++20 compiler.

```bash
cmake -B build -S .
cmake --build build
./build/miniredis
```

The server listens on port `6379` (the default Redis port).

## Trying it out

With the server running, connect using the standard Redis CLI:

```bash
redis-cli
127.0.0.1:6379> SET hello world
OK
127.0.0.1:6379> GET hello
"world"
127.0.0.1:6379> DEL hello
(integer) 1
```

## Running tests

Unit tests use GoogleTest (fetched automatically via CMake if not found on the system):

```bash
cmake -B build -S . -DMINIREDIS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## Why this project exists

This is a learning project built to understand, from first principles, how a system like Redis works under the hood: raw socket I/O, buffered/streaming protocol parsing, thread-per-connection concurrency, and safe shared-state access with reader-writer locks. It is not intended for production use.
