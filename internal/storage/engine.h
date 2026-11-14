#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

using namespace std;

namespace storage {

// Engine represent our thread-safe, in-memory key-value database
class Engine {
public:
    // Set writes a KV pair to the database
    void Set(const string& key, const string& val);

    // Get retrieves a value from the database safely
    // returns nullopt for a missing key so we can differentiate it from an empty string value
    optional<string> Get(const string& key) const;

    // Delete removes a key from the database, returns true if the key existed
    bool Delete(const string& key);

private:
    unordered_map<string, string> data;
    mutable shared_mutex mu;
};

} // namespace storage

