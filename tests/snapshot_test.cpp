
#include "../include/snapshot.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>

class SnapshotTest : public ::testing::Test {
protected:
    const std::string path = "snapshot_test.dat";
    Snapshot snapshot{path};

    void SetUp() override {
        std::remove(path.c_str());
        std::remove((path + ".tmp").c_str());
    }

    void TearDown() override {
        std::remove(path.c_str());
        std::remove((path + ".tmp").c_str());
    }

    void write_bytes(const std::string& bytes) {
        std::ofstream out(
            path,
            std::ios::binary | std::ios::trunc
        );

        ASSERT_TRUE(out.is_open());
        out.write(
            bytes.data(),
            static_cast<std::streamsize>(bytes.size())
        );
        ASSERT_TRUE(out.good());
    }
};

TEST_F(SnapshotTest, SavesAndLoadsData) {
    std::unordered_map<std::string, std::string> data = {
        {"name", "divs"},
        {"city", "Pune"},
        {"language", "cpp"}
    };

    snapshot.save(data, 42);

    SnapshotData loaded = snapshot.load();

    EXPECT_EQ(loaded.lsn, 42);
    EXPECT_EQ(loaded.data, data);
}

TEST_F(SnapshotTest, HandlesEmptyDatabase) {
    std::unordered_map<std::string, std::string> data;

    snapshot.save(data, 0);

    SnapshotData loaded = snapshot.load();

    EXPECT_EQ(loaded.lsn, 0);
    EXPECT_TRUE(loaded.data.empty());
}

TEST_F(SnapshotTest, HandlesEmptyValues) {
    std::unordered_map<std::string, std::string> data = {
        {"empty", ""},
        {"normal", "value"}
    };

    snapshot.save(data, 7);

    SnapshotData loaded = snapshot.load();

    EXPECT_EQ(loaded.data, data);
    EXPECT_EQ(loaded.lsn, 7);
}

TEST_F(SnapshotTest, HandlesSpecialCharactersAndNullBytes) {
    std::string key = "key|with\nnewline";
    const char raw_value[] = "value\0with|delimiters\n";
    std::string value(raw_value, sizeof(raw_value) - 1);    

    std::unordered_map<std::string, std::string> data = {
        {key, value}
    };

    snapshot.save(data, 15);

    SnapshotData loaded = snapshot.load();

    ASSERT_EQ(loaded.data.size(), 1);
    EXPECT_EQ(loaded.data.at(key), value);
    EXPECT_EQ(loaded.lsn, 15);
}

TEST_F(SnapshotTest, RejectsInvalidMagic) {
    write_bytes("NOPE");

    EXPECT_THROW(snapshot.load(), std::runtime_error);
}

TEST_F(SnapshotTest, RejectsTruncatedHeader) {
    write_bytes("KVSN");

    EXPECT_THROW(snapshot.load(), std::runtime_error);
}

TEST_F(SnapshotTest, RejectsUnsupportedVersion) {
    // Write a snapshot with a valid magic header but version 2.
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());

    const char magic[] = {'K', 'V', 'S', 'N'};
    const uint32_t version = 2;
    const uint64_t lsn = 0;
    const uint64_t count = 0;

    out.write(magic, sizeof(magic));
    out.write(
        reinterpret_cast<const char*>(&version),
        sizeof(version)
    );
    out.write(
        reinterpret_cast<const char*>(&lsn),
        sizeof(lsn)
    );
    out.write(
        reinterpret_cast<const char*>(&count),
        sizeof(count)
    );
    out.close();

    ASSERT_TRUE(out.good());
    EXPECT_THROW(snapshot.load(), std::runtime_error);
}

TEST_F(SnapshotTest, RejectsTruncatedEntry) {
    std::unordered_map<std::string, std::string> data = {
        {"name", "divs"}
    };

    snapshot.save(data, 1);

    // Truncate the final byte of the saved snapshot.
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open());

    std::string bytes(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );
    in.close();

    ASSERT_GT(bytes.size(), 24);
    bytes.pop_back();
    write_bytes(bytes);

    EXPECT_THROW(snapshot.load(), std::runtime_error);
}

TEST_F(SnapshotTest, RejectsTrailingData) {
    snapshot.save({{"name", "divs"}}, 1);

    std::ofstream out(
        path,
        std::ios::binary | std::ios::app
    );
    ASSERT_TRUE(out.is_open());
    out.put('X');
    out.close();

    EXPECT_THROW(snapshot.load(), std::runtime_error);
}
