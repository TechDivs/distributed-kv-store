#pragma once

#include <cstring>
#include <string>
#include <unordered_map>
using std::string;

struct SnapshotData {
    uint64_t lsn;
    std::unordered_map<string,string> data;
};

class Snapshot {
private:
    string file_path;
public:
    explicit Snapshot(const string& path);
    void save(const std::unordered_map<string,string>& data, uint64_t lsn);
    SnapshotData load() const;
};