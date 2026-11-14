#pragma once

#include "internal/bufio/reader.h"

#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace resp {

// ProtocolError means the client sent bytes that aren't valid RESP
class ProtocolError : public runtime_error {
public:
    using runtime_error::runtime_error;
};

// wraps a bufio::Reader to decode incoming RESP data
class Parser {
public:
    // same limits real Redis uses for client requests
    static constexpr long long maxArgs = 1024 * 1024;
    static constexpr long long maxBulkLen = 512LL * 1024 * 1024;

    explicit Parser(bufio::Reader& reader);

    // Parse extracts a full RESP array into a vector of strings
    vector<string> Parse();

private:
    string readLine();

    bufio::Reader& reader;
};

} // namespace resp

