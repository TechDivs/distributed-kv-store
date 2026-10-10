#include "kv_store.hpp"
#include "wal.hpp"
#include <gtest/gtest.h>
#include <filesystem>

class KVStoreTest : public ::testing::Test {
protected:
    std::string file_name = "test_kv_store.wal";

    void SetUp() override {
        std::filesystem::remove(file_name);
    }

    void TearDown() override {
        std::filesystem::remove(file_name);
    }
};

TEST_F(KVStoreTest, SetAndGet) {
    WAL wal(file_name);
    KVStore store(wal);

    store.set("name", "divyansh");
    auto val = store.get("name");

    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "divyansh");
}

TEST_F(KVStoreTest, GetMissingKey) {
    WAL wal(file_name);
    KVStore store(wal);

    auto val = store.get("missing");

    EXPECT_FALSE(val.has_value());
}

TEST_F(KVStoreTest, UpdateValue) {
    WAL wal(file_name);
    KVStore store(wal);

    store.set("name", "divyansh");
    store.set("name", "new_name");

    auto val = store.get("name");

    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "new_name");
}

TEST_F(KVStoreTest, RemoveKey) {
    WAL wal(file_name);
    KVStore store(wal);

    store.set("name", "divyansh");

    EXPECT_TRUE(store.remove("name"));
    EXPECT_FALSE(store.get("name").has_value());
}

TEST_F(KVStoreTest, RemoveMissingKey) {
    WAL wal(file_name);
    KVStore store(wal);

    EXPECT_FALSE(store.remove("missing"));
}