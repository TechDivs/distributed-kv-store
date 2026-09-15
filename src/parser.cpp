#include "../include/parser.hpp"
#include <string>
#include <sstream>
using std::string;

Parser::Parser(KVStore& store) : kv_store(store) {}

string Parser::execute(const string& cmd) {
    std::istringstream iss(cmd);
    string op;
    iss >> op;

    if (op == "SET") {
        string key, val, extra;

        if (!(iss >> key >> val)) return "ERROR";
        if (iss >> extra) return "ERROR";

        kv_store.set(key, val);
        return "OK";
    }
    else if (op == "GET") {
        string key, extra;

        if (!(iss >> key)) return "ERROR";
        if (iss >> extra) return "ERROR";
        
        auto res = kv_store.get(key);
        return res.has_value() ? res.value() : "NOT_FOUND";
    }
    else if (op == "DELETE") {
        string key, extra;

        if (!(iss >> key)) return "ERROR";
        if (iss >> extra) return "ERROR";

        if(kv_store.remove(key)) return "OK";
        return "NOT_FOUND";
    }
    else return "ERROR";
}