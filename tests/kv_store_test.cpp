#include "kv_store.hpp"
#include <gtest/gtest.h>

TEST(KVStoreTest, SetAndGet) {
    KVStore store;
    store.set("name", "divyansh");
    auto val = store.get("name");

    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "divyansh");
}

TEST(KVStoreTest, GetMissingKey) {
    KVStore store;
    auto val = store.get("missing");

    EXPECT_FALSE(val.has_value());
}

TEST(KVStoreTest, UpdateValue) {
    KVStore store;

    store.set("name", "divyansh");
    store.set("name", "new_name");

    auto val = store.get("name");

    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "new_name");
}

TEST(KVStoreTest, RemoveKey) {
    KVStore store;
    store.set("name", "divyansh");

    EXPECT_TRUE(store.remove("name"));
    EXPECT_FALSE(store.get("name").has_value());
}

TEST(KVStoreTest, RemoveMissingKey) {
    KVStore store;
    EXPECT_FALSE(store.remove("missing"));
}