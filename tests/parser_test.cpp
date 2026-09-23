#include "internal/resp/parser.h"
#include "tests/string_reader.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

using Args = std::vector<std::string>;

Args parseOne(const std::string& input, std::size_t chunkSize = 4096) {
    StringReader src(input, chunkSize);
    bufio::Reader reader(src);
    resp::Parser parser(reader);
    return parser.Parse();
}

} // namespace

TEST(Parser, ParsesCommand) {
    EXPECT_EQ(parseOne("*3\r\n$3\r\nSET\r\n$5\r\nhello\r\n$7\r\nvedansh\r\n"),
              (Args{"SET", "hello", "vedansh"}));
}

TEST(Parser, ParsesEmptyArray) {
    EXPECT_EQ(parseOne("*0\r\n"), Args{});
}

TEST(Parser, ParsesEmptyBulkString) {
    EXPECT_EQ(parseOne("*2\r\n$3\r\nGET\r\n$0\r\n\r\n"), (Args{"GET", ""}));
}

TEST(Parser, ValueMayContainCRLF) {
    // the old Go parser read bulk data up to the next newline and broke on this
    EXPECT_EQ(parseOne("*3\r\n$3\r\nSET\r\n$1\r\nk\r\n$4\r\na\r\nb\r\n"),
              (Args{"SET", "k", "a\r\nb"}));
}

TEST(Parser, HandlesOneByteChunks) {
    EXPECT_EQ(parseOne("*2\r\n$3\r\nGET\r\n$5\r\nhello\r\n", 1), (Args{"GET", "hello"}));
}

TEST(Parser, ParsesPipelinedCommands) {
    StringReader src("*1\r\n$4\r\nPING\r\n*2\r\n$3\r\nGET\r\n$1\r\nk\r\n");
    bufio::Reader reader(src);
    resp::Parser parser(reader);

    EXPECT_EQ(parser.Parse(), Args{"PING"});
    EXPECT_EQ(parser.Parse(), (Args{"GET", "k"}));
}

TEST(Parser, RejectsMalformedInput) {
    const std::vector<std::string> bad = {
        "\r\n",                            // empty payload
        "PING\r\n",                        // not an array
        "*-1\r\n",                         // negative count (crashed the Go version)
        "*abc\r\n",                        // count isn't a number
        "*99999999999\r\n",                // absurd count
        "*1\r\n+PING\r\n",                 // element isn't a bulk string
        "*1\r\n$-1\r\n",                   // negative bulk length
        "*1\r\n$999999999999\r\n",         // absurd bulk length
        "*1\r\n$4\r\nPINGxx",              // bulk data not followed by CRLF
    };
    for (const std::string& input : bad) {
        EXPECT_THROW(parseOne(input), resp::ProtocolError) << "input: " << input;
    }
}

TEST(Parser, ThrowsEOFOnTruncatedCommand) {
    EXPECT_THROW(parseOne("*2\r\n$3\r\nGET\r\n"), io::EOFError);
    EXPECT_THROW(parseOne("*1\r\n$5\r\nhel"), io::EOFError);
}
