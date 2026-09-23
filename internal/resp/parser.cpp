#include "internal/resp/parser.h"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>

namespace resp {

namespace {

// parseInt converts the whole string to a number (like strconv.Atoi), nullopt if it isn't one
std::optional<long long> parseInt(std::string_view s) {
    long long value = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
        return std::nullopt;
    }
    return value;
}

} // namespace

// this is the constructor of Parser
Parser::Parser(bufio::Reader& reader) : reader(reader) {}

// readLine reads from the network until it hits a newline, then strips the \r\n
std::string Parser::readLine() {
    std::string line = reader.ReadBytes('\n');

    // since RESP uses \r\n to end every line, we strip it off
    if (line.ends_with("\r\n")) {
        line.resize(line.size() - 2);
    }
    return line;
}

std::vector<std::string> Parser::Parse() {
    std::string line = readLine();
    if (line.empty()) {
        throw ProtocolError("empty RESP payload");
    }

    // clients talking to a Redis server always wrap their commands in an Array (indicated by *) --> *2\r\n$5\r\nhello\r\n$5\r\nworld\r\n
    if (line[0] != '*') {
        throw ProtocolError("expected RESP array prefix");
    }
    // the rest of the line after * is the number of elements in the array
    std::optional<long long> numArgs = parseInt(std::string_view(line).substr(1));
    if (!numArgs || *numArgs < 0 || *numArgs > maxArgs) {
        throw ProtocolError("invalid multibulk length");
    }

    // pre allocate for performance, but capped so a huge (fake) count can't eat memory up front
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(std::min(*numArgs, 1024LL)));

    for (long long i = 0; i < *numArgs; i++) {
        // read the bulk string prefix -> $
        std::string strLine = readLine();
        if (strLine.empty() || strLine[0] != '$') {
            throw ProtocolError("expected RESP bulk string prefix");
        }
        std::optional<long long> strLen = parseInt(std::string_view(strLine).substr(1));
        if (!strLen || *strLen < 0 || *strLen > maxBulkLen) {
            throw ProtocolError("invalid bulk length");
        }

        // read exactly strLen bytes plus the trailing \r\n, so the data itself may contain \r\n
        std::string dataLine = reader.ReadFull(static_cast<std::size_t>(*strLen) + 2);
        if (!dataLine.ends_with("\r\n")) {
            throw ProtocolError("expected CRLF after bulk string");
        }
        dataLine.resize(static_cast<std::size_t>(*strLen));
        args.push_back(std::move(dataLine));
    }
    return args;
}

} // namespace resp
