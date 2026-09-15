#include <gtest/gtest.h>

#include "../include/parser.hpp"

TEST(ParserTest, SetAndGet) {
    KVStore store;
    Parser parser(store);

    EXPECT_EQ(parser.execute("SET name Divyansh"), "OK");
    EXPECT_EQ(parser.execute("GET name"), "Divyansh");
}

TEST(ParserTest, GetMissingKey) {
    KVStore store;
    Parser parser(store);

    EXPECT_EQ(parser.execute("GET name"), "NOT_FOUND");
}

TEST(ParserTest, DeleteKey) {
    KVStore store;
    Parser parser(store);

    parser.execute("SET name Divyansh");

    EXPECT_EQ(parser.execute("DELETE name"), "OK");
    EXPECT_EQ(parser.execute("GET name"), "NOT_FOUND");
}

TEST(ParserTest, DeleteMissingKey) {
    KVStore store;
    Parser parser(store);

    EXPECT_EQ(parser.execute("DELETE name"), "NOT_FOUND");
}

TEST(ParserTest, InvalidCommand) {
    KVStore store;
    Parser parser(store);

    EXPECT_EQ(parser.execute("INVALID name"), "ERROR");
}

TEST(ParserTest, InvalidArguments) {
    KVStore store;
    Parser parser(store);

    EXPECT_EQ(parser.execute("SET name"), "ERROR");
    EXPECT_EQ(parser.execute("GET"), "ERROR");
    EXPECT_EQ(parser.execute("DELETE"), "ERROR");

    EXPECT_EQ(parser.execute("SET name Divyansh extra"), "ERROR");
    EXPECT_EQ(parser.execute("GET name extra"), "ERROR");
    EXPECT_EQ(parser.execute("DELETE name extra"), "ERROR");
}