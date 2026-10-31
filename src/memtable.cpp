
#include "../include/memtable.hpp"
#include <stdexcept>
#include <utility>

size_t MemTable::get_entry_size(const std::string& key,const Entry& entry) const {
    return sizeof(Entry) +
       sizeof(std::string) +
       key.size() +
       entry.value.size();
}

void MemTable::put(const std::string& key,const std::string& value,uint64_t lsn) {
    if (key.empty()) {
        throw std::invalid_argument("Key cannot be empty");
    }

    auto it = entries.find(key);

    // A key must never be overwritten by an older version.
    if (it != entries.end() && lsn <= it->second.lsn) {
        throw std::invalid_argument("LSN must be increasing");
    }

    Entry new_entry{
        value,
        lsn,
        false
    };

    // If key already exists, remove its old size
    // before accounting for the new entry.
    if (it != entries.end()) {
        memory_usage -= get_entry_size(key, it->second);
    }

    entries[key] = std::move(new_entry);

    memory_usage += get_entry_size(key, entries.at(key));
}

const std::map<std::string, Entry>& MemTable::get_entries() const {
    return entries;
}

void MemTable::erase(const std::string& key,uint64_t lsn) {
    if (key.empty()) {
        throw std::invalid_argument("Key cannot be empty");
    }

    auto it = entries.find(key);

    if (it != entries.end() && lsn <= it->second.lsn) {
        throw std::invalid_argument("LSN must be increasing");
    }

    Entry tombstone{
        "",
        lsn,
        true
    };

    if (it != entries.end()) {
        memory_usage -= get_entry_size(key, it->second);
    }

    entries[key] = std::move(tombstone);

    memory_usage += get_entry_size(key, entries.at(key));
}

std::optional<Entry> MemTable::get(const std::string& key) const {
    auto it = entries.find(key);

    if (it == entries.end()) {
        return std::nullopt;
    }

    return it->second;
}

size_t MemTable::size_bytes() const {
    return memory_usage;
}

size_t MemTable::size() const {
    return entries.size();
}

bool MemTable::empty() const {
    return entries.empty();
}

void MemTable::clear() {
    entries.clear();
    memory_usage = 0;
}
