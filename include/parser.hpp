#pragma once
#include "kv_store.hpp"

class Parser {
private:
    KVStore& kv_store;
public:
    Parser(KVStore& store);
    std::string execute(const std::string& cmd);
};