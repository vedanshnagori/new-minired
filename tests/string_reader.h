#pragma once

#include "internal/io/io.h"

#include <algorithm>
#include <cstring>
#include <string>

using namespace std;

// StringReader feeds a fixed string through the io::Reader interface,
// at most chunkSize bytes per Read to imitate data trickling in from a socket
class StringReader : public io::Reader {
public:
    explicit StringReader(string data, size_t chunkSize = 4096)
        : data(move(data)), chunkSize(chunkSize) {}

    size_t Read(char* p, size_t n) override {
        size_t take = min({n, chunkSize, data.size() - pos});
        memcpy(p, data.data() + pos, take);
        pos += take;
        return take;
    }

private:
    string data;
    size_t chunkSize;
    size_t pos = 0;
};

