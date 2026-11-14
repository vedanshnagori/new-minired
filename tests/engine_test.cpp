#include "internal/storage/engine.h"

#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

using namespace std;

TEST(Engine, SetThenGet) {
    storage::Engine db;
    db.Set("hello", "vedansh");

    EXPECT_EQ(db.Get("hello"), "vedansh");
}

TEST(Engine, MissingKeyIsNullopt) {
    storage::Engine db;

    EXPECT_EQ(db.Get("missing"), nullopt);
}

TEST(Engine, EmptyValueIsNotMissing) {
    storage::Engine db;
    db.Set("empty", "");

    EXPECT_EQ(db.Get("empty"), "");
}

TEST(Engine, SetOverwrites) {
    storage::Engine db;
    db.Set("k", "v1");
    db.Set("k", "v2");

    EXPECT_EQ(db.Get("k"), "v2");
}

TEST(Engine, DeleteReportsWhetherKeyExisted) {
    storage::Engine db;
    db.Set("k", "v");

    EXPECT_TRUE(db.Delete("k"));
    EXPECT_FALSE(db.Delete("k"));
    EXPECT_EQ(db.Get("k"), nullopt);
}

TEST(Engine, ConcurrentWritersAndReaders) {
    storage::Engine db;
    constexpr int numThreads = 8;
    constexpr int perThread = 1000;

    vector<thread> threads;
    for (int t = 0; t < numThreads; t++) {
        threads.emplace_back([&db, t] {
            for (int i = 0; i < perThread; i++) {
                string key = to_string(t) + ":" + to_string(i);
                db.Set(key, key);
                db.Get(key);
            }
        });
    }
    for (thread& th : threads) {
        th.join();
    }

    for (int t = 0; t < numThreads; t++) {
        for (int i = 0; i < perThread; i++) {
            string key = to_string(t) + ":" + to_string(i);
            ASSERT_EQ(db.Get(key), key);
        }
    }
}

