#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <optional>
#include <cstddef>

struct Entry {
    std::string value;
    uint64_t lsn;
    bool is_deleted;
};

class MemTable {
private:
    std::map<std::string, Entry> entries;
    size_t memory_usage=0;

    size_t get_entry_size(const std::string& key,const Entry& entry) const;

public:
    void put(const std::string& key,const std::string& value,uint64_t lsn);
    void erase(const std::string& key,uint64_t lsn);
    std::optional<Entry> get(const std::string& key) const;
    size_t size_bytes() const;
    size_t size() const;
    bool empty() const;
    void clear();
    const std::map<std::string, Entry>& get_entries() const;
};