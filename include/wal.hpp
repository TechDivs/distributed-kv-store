#pragma once

#include <cstdint>
#include <windows.h>
#include <string>

class KVStore;

class WAL {
private:
    std::string file_path;
    HANDLE file_handle;
    uint64_t next_lsn;

    void flush_to_disk();
    
public:
    explicit WAL(const std::string& path);
    ~WAL();
    uint64_t append_set(const std::string& key,const std::string& val);
    uint64_t append_delete(const std::string& key);
    void recover(KVStore& store);
    uint64_t get_next_lsn() const;
};