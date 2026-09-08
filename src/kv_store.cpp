#include "../include/kv_store.hpp"
using std::string;

void KVStore::set(const string& key, const string& val) {
    store[key]=val;
}

std::optional<string> KVStore::get(const string& key) {
    auto it = store.find(key);
    if(it!=store.end()) return it->second;
    return std::nullopt;
}

bool KVStore::remove(const string& key) {
    return store.erase(key) > 0; // erase returns no. of elements removed
}