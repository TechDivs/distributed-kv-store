#pragma once
#include <string>
#include <unordered_map>
#include <optional>
using std::string;

class WAL;

class KVStore {
private:
    std::unordered_map<string,string> store;
    WAL& wal;

    void set_in_memory(const string& key,const string& val);
    bool remove_in_memory(const string& key);
    friend class WAL;
public:
    explicit KVStore(WAL& wal);
    void set(const string& key,const string& val);
    std::optional<string> get(const string& key);
    bool remove(const string& key);

    std::unordered_map<string,string> get_snapshot_data() const;
    void restore_snapshot(const std::unordered_map<string,string>& data);
};