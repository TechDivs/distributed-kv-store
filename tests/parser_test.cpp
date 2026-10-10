#include <gtest/gtest.h>

#include "../include/parser.hpp"
#include "../include/wal.hpp"

#include <filesystem>

class ParserTest : public ::testing::Test {
protected:
    std::string file_name = "test_parser.wal";

    void SetUp() override {
        std::filesystem::remove(file_name);
    }

    void TearDown() override {
        std::filesystem::remove(file_name);
    }
};

TEST_F(ParserTest, SetAndGet) {
    WAL wal(file_name);
    KVStore store(wal);
    Parser parser(store);

    EXPECT_EQ(parser.execute("SET name Divyansh"), "OK");
    EXPECT_EQ(parser.execute("GET name"), "Divyansh");
}

TEST_F(ParserTest, GetMissingKey) {
    WAL wal(file_name);
    KVStore store(wal);
    Parser parser(store);

    EXPECT_EQ(parser.execute("GET name"), "NOT_FOUND");
}

TEST_F(ParserTest, DeleteKey) {
    WAL wal(file_name);
    KVStore store(wal);
    Parser parser(store);

    parser.execute("SET name Divyansh");

    EXPECT_EQ(parser.execute("DELETE name"), "OK");
    EXPECT_EQ(parser.execute("GET name"), "NOT_FOUND");
}

TEST_F(ParserTest, DeleteMissingKey) {
    WAL wal(file_name);
    KVStore store(wal);
    Parser parser(store);

    EXPECT_EQ(parser.execute("DELETE name"), "NOT_FOUND");
}

TEST_F(ParserTest, InvalidCommand) {
    WAL wal(file_name);
    KVStore store(wal);
    Parser parser(store);

    EXPECT_EQ(parser.execute("INVALID name"), "ERROR");
}

TEST_F(ParserTest, InvalidArguments) {
    WAL wal(file_name);
    KVStore store(wal);
    Parser parser(store);

    EXPECT_EQ(parser.execute("SET name"), "ERROR");
    EXPECT_EQ(parser.execute("GET"), "ERROR");
    EXPECT_EQ(parser.execute("DELETE"), "ERROR");

    EXPECT_EQ(parser.execute("SET name Divyansh extra"), "ERROR");
    EXPECT_EQ(parser.execute("GET name extra"), "ERROR");
    EXPECT_EQ(parser.execute("DELETE name extra"), "ERROR");
}