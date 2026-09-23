#include "internal/storage/engine.h"

#include <mutex>

namespace storage {

void Engine::Set(const std::string& key, const std::string& value) {
    std::unique_lock lock(mu); // write lock, released automatically when lock goes out of scope

    data[key] = value;
}

std::optional<std::string> Engine::Get(const std::string& key) const {
    std::shared_lock lock(mu); // shared read lock

    auto it = data.find(key);
    if (it == data.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Engine::Delete(const std::string& key) {
    std::unique_lock lock(mu);

    return data.erase(key) > 0;
}

} // namespace storage
