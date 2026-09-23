#include "internal/bufio/reader.h"
#include "tests/string_reader.h"

#include <gtest/gtest.h>

TEST(BufioReader, ReadBytesKeepsDelimiter) {
    StringReader src("hello\nworld\n");
    bufio::Reader reader(src);

    EXPECT_EQ(reader.ReadBytes('\n'), "hello\n");
    EXPECT_EQ(reader.ReadBytes('\n'), "world\n");
}

TEST(BufioReader, ReadBytesAcrossTinyChunks) {
    StringReader src("hello\nworld\n", 1);
    bufio::Reader reader(src);

    EXPECT_EQ(reader.ReadBytes('\n'), "hello\n");
    EXPECT_EQ(reader.ReadBytes('\n'), "world\n");
}

TEST(BufioReader, ReadFullReturnsExactBytes) {
    StringReader src("abc\r\ndef", 2);
    bufio::Reader reader(src);

    EXPECT_EQ(reader.ReadFull(5), "abc\r\n");
    EXPECT_EQ(reader.ReadFull(3), "def");
}

TEST(BufioReader, ThrowsEOFWhenStreamEndsEarly) {
    StringReader src("no newline");
    bufio::Reader reader(src);

    EXPECT_THROW(reader.ReadBytes('\n'), io::EOFError);

    StringReader src2("abc");
    bufio::Reader reader2(src2);
    EXPECT_THROW(reader2.ReadFull(10), io::EOFError);
}

TEST(BufioReader, ThrowsWhenLineTooLong) {
    StringReader src(std::string(100, 'x') + "\n");
    bufio::Reader reader(src, 16);

    EXPECT_THROW(reader.ReadBytes('\n'), bufio::BufferFullError);
}
