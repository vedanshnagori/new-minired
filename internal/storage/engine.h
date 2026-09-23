#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace storage {

// Engine represent our thread-safe, in-memory key-value database
class Engine {
public:
    // Set writes a KV pair to the database
    void Set(const std::string& key, const std::string& value);

    // Get retrieves a value from the database safely
    // returns nullopt for a missing key so we can differentiate it from an empty string value
    std::optional<std::string> Get(const std::string& key) const;

    // Delete removes a key from the database, returns true if the key existed
    bool Delete(const std::string& key);

private:
    std::unordered_map<std::string, std::string> data;
    mutable std::shared_mutex mu;
};

} // namespace storage
