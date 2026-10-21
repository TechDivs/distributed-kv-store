#include <gtest/gtest.h>

#include "../include/kv_store.hpp"
#include "../include/wal.hpp"
#include "../include/snapshot.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

class SnapshotRecoveryTest : public ::testing::Test {
protected:
    const std::string wal_path = "snapshot_recovery_test.wal";
    const std::string snapshot_path = "snapshot_recovery_test.dat";

    void SetUp() override {
        std::filesystem::remove(wal_path);
        std::filesystem::remove(snapshot_path);
        std::filesystem::remove(snapshot_path + ".tmp");
    }

    void TearDown() override {
        std::filesystem::remove(wal_path);
        std::filesystem::remove(snapshot_path);
        std::filesystem::remove(snapshot_path + ".tmp");
    }
};

TEST_F(SnapshotRecoveryTest, ReplaysOnlyWalEntriesAfterSnapshot) {
    {
        WAL wal(wal_path);
        KVStore store(wal);

        store.set("name", "Divyansh");  // LSN 1
        store.set("city", "Pune");     // LSN 2

        Snapshot snapshot(snapshot_path);
        snapshot.save(store.get_snapshot_data(),
                      wal.get_next_lsn() - 1);

        store.set("city", "Bangalore"); // LSN 3
        store.set("role", "SDE Intern"); // LSN 4
        store.remove("name");            // LSN 5
    }

    {
        WAL wal(wal_path);
        KVStore store(wal);

        Snapshot snapshot(snapshot_path);
        SnapshotData data = snapshot.load();

        store.restore_snapshot(data.data);
        wal.recover(store, data.lsn);

        EXPECT_FALSE(store.get("name").has_value());

        ASSERT_TRUE(store.get("city").has_value());
        EXPECT_EQ(*store.get("city"), "Bangalore");

        ASSERT_TRUE(store.get("role").has_value());
        EXPECT_EQ(*store.get("role"), "SDE Intern");
    }
}