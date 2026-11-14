#include "internal/storage/engine.h"

#include <mutex>

using namespace std;

namespace storage {

void Engine::Set(const string& key, const string& val) {
    unique_lock lock(mu); // write lock, released automatically when lock goes out of scope

    data[key] = val;
}

optional<string> Engine::Get(const string& key) const {
    shared_lock lock(mu); // shared read lock

    auto it = data.find(key);
    if (it == data.end()) {
        return nullopt;
    }
    return it->second;
}

bool Engine::Delete(const string& key) {
    unique_lock lock(mu);

    return data.erase(key) > 0;
}

} // namespace storage

