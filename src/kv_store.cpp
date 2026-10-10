#include "../include/kv_store.hpp"
#include "../include/wal.hpp"
using std::string;

KVStore::KVStore(WAL& wal):wal(wal) {}

void KVStore::set(const string& key, const string& val) {
    wal.append_set(key,val);
    set_in_memory(key,val);
}

std::optional<string> KVStore::get(const string& key) {
    auto it = store.find(key);
    if(it!=store.end()) return it->second;
    return std::nullopt;
}

bool KVStore::remove(const string& key) {
    if(store.find(key)==store.end()) return false;
    wal.append_delete(key);
    return remove_in_memory(key);
}

void KVStore::set_in_memory(const string& key, const string& val) {
    store[key] = val;
}

bool KVStore::remove_in_memory(const string& key) {
    return store.erase(key) > 0;
}