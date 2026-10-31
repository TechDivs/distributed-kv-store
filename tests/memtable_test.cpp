
#include "memtable.hpp"
#include <vector>
#include <gtest/gtest.h>

class MemTableTest : public ::testing::Test {
protected:
    MemTable mem;
};

TEST_F(MemTableTest, InsertAndGet) {
    mem.put("name", "Divyansh", 1);

    auto result = mem.get("name");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "Divyansh");
    EXPECT_EQ(result->lsn, 1);
    EXPECT_FALSE(result->is_deleted);
}

TEST_F(MemTableTest, UpdateExistingKey) {
    mem.put("name", "Divyansh", 1);
    mem.put("name", "Rahul", 2);

    auto result = mem.get("name");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "Rahul");
    EXPECT_EQ(result->lsn, 2);
    EXPECT_EQ(mem.size(), 1);
}

TEST_F(MemTableTest, GetMissingKey) {
    auto result = mem.get("missing");

    EXPECT_FALSE(result.has_value());
}

TEST_F(MemTableTest, DeleteCreatesTombstone) {
    mem.put("city", "Pune", 1);
    mem.erase("city", 2);

    auto result = mem.get("city");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_deleted);
    EXPECT_EQ(result->lsn, 2);
    EXPECT_TRUE(result->value.empty());
}

TEST_F(MemTableTest, DeleteMissingKey) {
    mem.erase("city", 1);

    auto result = mem.get("city");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_deleted);
}

TEST_F(MemTableTest, RejectOlderLSN) {
    mem.put("name", "Rahul", 10);

    EXPECT_THROW(
        mem.put("name", "Divyansh", 8),
        std::invalid_argument
    );

    auto result = mem.get("name");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "Rahul");
}

TEST_F(MemTableTest, RejectDuplicateLSN) {
    mem.put("name", "Rahul", 10);

    EXPECT_THROW(
        mem.put("name", "Anuj", 10),
        std::invalid_argument
    );
}

TEST_F(MemTableTest, MemoryUsageIncreases) {
    EXPECT_EQ(mem.size_bytes(), 0);

    mem.put("name", "Divyansh", 1);

    EXPECT_GT(mem.size_bytes(), 0);
}

TEST_F(MemTableTest, ClearMemTable) {
    mem.put("name", "Divyansh", 1);
    mem.put("city", "Pune", 2);

    mem.clear();

    EXPECT_TRUE(mem.empty());
    EXPECT_EQ(mem.size(), 0);
    EXPECT_EQ(mem.size_bytes(), 0);
}

TEST_F(MemTableTest, KeysRemainSorted) {
    mem.put("zebra", "black", 1);
    mem.put("apple", "red", 2);
    mem.put("mango", "yellow", 3);

    std::vector<std::string> keys;

    for (const auto& [key, entry] : mem.get_entries()) {
        keys.push_back(key);
    }

    EXPECT_EQ(
        keys,
        (std::vector<std::string>{"apple", "mango", "zebra"})
    );
}
