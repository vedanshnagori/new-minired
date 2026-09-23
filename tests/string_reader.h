#pragma once

#include "internal/io/io.h"

#include <algorithm>
#include <cstring>
#include <string>

// StringReader feeds a fixed string through the io::Reader interface,
// at most chunkSize bytes per Read to imitate data trickling in from a socket
class StringReader : public io::Reader {
public:
    explicit StringReader(std::string data, std::size_t chunkSize = 4096)
        : data(std::move(data)), chunkSize(chunkSize) {}

    std::size_t Read(char* p, std::size_t n) override {
        std::size_t take = std::min({n, chunkSize, data.size() - pos});
        std::memcpy(p, data.data() + pos, take);
        pos += take;
        return take;
    }

private:
    std::string data;
    std::size_t chunkSize;
    std::size_t pos = 0;
};
