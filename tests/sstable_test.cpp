#include "sstable.hpp"
#include <gtest/gtest.h>
#include <filesystem>

class SSTableTest : public ::testing::Test {
protected:
    MemTable mem;
    SSTable sst;

    std::string path = "test_sstable.dat";

    void SetUp() override {
        std::filesystem::remove(path);
        std::filesystem::remove(path + ".tmp");
    }

    void TearDown() override {
        std::filesystem::remove(path);
        std::filesystem::remove(path + ".tmp");
    }
};

TEST_F(SSTableTest, CreateAndRead) {
    mem.put("name", "Divyansh", 1);
    mem.put("city", "Pune", 2);

    SSTable::create(path, mem);
    sst.open(path);

    auto result = sst.get("name");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "Divyansh");
    EXPECT_EQ(result->lsn, 1);
    EXPECT_FALSE(result->is_deleted);
}

TEST_F(SSTableTest, MissingKey) {
    mem.put("name", "Divyansh", 1);

    SSTable::create(path, mem);
    sst.open(path);

    EXPECT_FALSE(sst.get("missing").has_value());
}

TEST_F(SSTableTest, TombstonePreserved) {
    mem.put("city", "Pune", 1);
    mem.erase("city", 2);

    SSTable::create(path, mem);
    sst.open(path);

    auto result = sst.get("city");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_deleted);
    EXPECT_EQ(result->lsn, 2);
}

TEST_F(SSTableTest, EmptySSTable) {
    SSTable::create(path, mem);
    sst.open(path);

    EXPECT_EQ(sst.size(), 0);
    EXPECT_FALSE(sst.get("anything").has_value());
}

TEST_F(SSTableTest, MultipleRecords) {
    for (int i = 0; i < 100; i++) {
        std::string key = "key" + std::to_string(i);
        mem.put(key, "value" + std::to_string(i), i + 1);
    }

    SSTable::create(path, mem);
    sst.open(path);

    for (int i = 0; i < 100; i++) {
        std::string key = "key" + std::to_string(i);

        auto result = sst.get(key);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(
            result->value,
            "value" + std::to_string(i)
        );
    }

    EXPECT_EQ(sst.size(), 100);
}

TEST_F(SSTableTest, RejectDuplicateFile) {
    SSTable::create(path, mem);

    EXPECT_THROW(
        SSTable::create(path, mem),
        std::runtime_error
    );
}
