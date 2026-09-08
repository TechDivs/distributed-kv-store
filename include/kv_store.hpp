#pragma once
#include <string>
#include <unordered_map>
#include <optional>
using std::string;

class KVStore {
private:
    std::unordered_map<string,string> store;
public:
    void set(const string& key,const string& val);
    std::optional<string> get(const string& key);
    bool remove(const string& key);
};