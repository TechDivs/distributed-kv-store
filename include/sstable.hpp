
#pragma once

#include "memtable.hpp"

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

class SSTable {
private:
    struct IndexEntry {
        std::string key;
        uint64_t offset;
        uint64_t record_index;
    };

    std::string file_path;
    std::vector<IndexEntry> index;
    uint64_t record_count = 0;

    static constexpr uint32_t MAGIC = 0x53535431;
    static constexpr uint32_t VERSION = 1;
    static constexpr size_t INDEX_INTERVAL = 16;

public:
    // Create a new SSTable from a MemTable.
    static void create(const std::string& path,const MemTable& mem);

    // Open an existing SSTable and rebuild its index.
    void open(const std::string& path);

    // Search for a key.
    std::optional<Entry> get(const std::string& key) const;
    uint64_t size() const;
};
