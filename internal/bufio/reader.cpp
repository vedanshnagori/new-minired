#include "internal/bufio/reader.h"

#include <algorithm>

using namespace std;

namespace bufio {

namespace {
constexpr size_t chunkSize = 4096;
}

Reader::Reader(io::Reader& rd, size_t maxLineSize)
    : rd(rd), maxLineSize(maxLineSize) {}

string Reader::ReadBytes(char delim) {
    size_t searched = 0; // bytes already scanned, so we don't rescan after each fill
    for (;;) {
        size_t pos = buf.find(delim, r + searched);
        if (pos != string::npos) {
            size_t n = pos - r + 1;
            if (n > maxLineSize) {
                throw BufferFullError();
            }
            string line = buf.substr(r, n);
            consume(n);
            return line;
        }
        searched = buffered();

        // stop a client from making us buffer an endless line
        if (searched >= maxLineSize) {
            throw BufferFullError();
        }
        if (!fill()) {
            throw io::EOFError();
        }
    }
}

string Reader::ReadFull(size_t n) {
    string out;
    while (out.size() < n) {
        if (buffered() == 0 && !fill()) {
            throw io::EOFError();
        }
        // grow as data actually arrives instead of trusting n up front
        size_t take = min(n - out.size(), buffered());
        out.append(buf, r, take);
        consume(take);
    }
    return out;
}

bool Reader::fill() {
    char chunk[chunkSize];
    size_t got = rd.Read(chunk, sizeof(chunk));
    if (got == 0) {
        return false;
    }
    buf.append(chunk, got);
    return true;
}

void Reader::consume(size_t n) {
    r += n;
    if (r == buf.size()) {
        // everything handed out, reuse the buffer from the start
        buf.clear();
        r = 0;
    } else if (r > chunkSize && r > buf.size() / 2) {
        // drop the already-read front so the buffer doesn't grow forever
        buf.erase(0, r);
        r = 0;
    }
}

} // namespace bufio

