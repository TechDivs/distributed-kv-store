#include <gtest/gtest.h>

#include "../include/wal.hpp"
#include "../include/kv_store.hpp"

#include <fstream>
#include <filesystem>

class WALTest : public ::testing::Test {
protected:
    std::string file_name = "test_wal.log";

    void SetUp() override {
        std::filesystem::remove(file_name);
    }

    void TearDown() override {
        std::filesystem::remove(file_name);
    }
};

TEST_F(WALTest, AppendSet) {

    WAL wal(file_name);

    uint64_t lsn = wal.append_set(
        "name",
        "Divyansh"
    );

    EXPECT_EQ(lsn, 1);
    EXPECT_EQ(wal.get_next_lsn(), 2);

    std::ifstream file(file_name);

    std::string line;
    std::getline(file, line);

    EXPECT_EQ(
        line,
        "1|SET|name|Divyansh"
    );
}

TEST_F(WALTest, AppendDelete) {

    WAL wal(file_name);

    uint64_t lsn = wal.append_delete("name");

    EXPECT_EQ(lsn, 1);
    EXPECT_EQ(wal.get_next_lsn(), 2);

    std::ifstream file(file_name);

    std::string line;
    std::getline(file, line);

    EXPECT_EQ(
        line,
        "1|DELETE|name|"
    );
}

TEST_F(WALTest, LSNIncrements) {

    WAL wal(file_name);

    EXPECT_EQ(wal.get_next_lsn(), 1);

    EXPECT_EQ(
        wal.append_set("A", "10"),
        1
    );

    EXPECT_EQ(
        wal.append_set("B", "20"),
        2
    );

    EXPECT_EQ(
        wal.append_delete("A"),
        3
    );

    EXPECT_EQ(wal.get_next_lsn(), 4);
}

TEST_F(WALTest, Recovery) {

    {
        WAL wal(file_name);

        wal.append_set("A", "10");
        wal.append_set("B", "20");
        wal.append_set("A", "50");
        wal.append_delete("B");
    }

    WAL wal(file_name);
    KVStore store(wal);


    wal.recover(store);

    auto a = store.get("A");
    auto b = store.get("B");

    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a.value(), "50");

    EXPECT_FALSE(b.has_value());
}

TEST_F(WALTest, LSNContinuesAfterRestart) {

    {
        WAL wal(file_name);

        wal.append_set("A", "10");
        wal.append_set("B", "20");
    }

    {
        WAL wal(file_name);

        EXPECT_EQ(
            wal.get_next_lsn(),
            3
        );

        EXPECT_EQ(
            wal.append_set("C", "30"),
            3
        );
    }
}

TEST_F(WALTest, FailsRecoveryOnIncompleteRecord) {   
    {
        std::ofstream file(
            file_name,
            std::ios::out | std::ios::binary
        );
        file << "1|SET|A|10\n";
        file << "2|SET|B";
    } 

    WAL wal(file_name);
    KVStore store(wal);

    EXPECT_THROW(
        wal.recover(store),
        std::runtime_error
    );
}

TEST_F(WALTest, FailsRecoveryOnCorruptedRecord) {

    {
        std::ofstream file(
            file_name,
            std::ios::out | std::ios::binary
        );

        file << "1|SET|A|10\n";
        file << "2|INVALID|B|20\n";
        file << "3|SET|C|30\n";
    }

    WAL wal(file_name);
    KVStore store(wal);

    EXPECT_THROW(
        wal.recover(store),
        std::runtime_error
    );
}