#include "kv_store.hpp"
#include <cassert>
#include <iostream>

int main() {
    KVStore store;

    // Test SET + GET
    store.set("name","divyansh");
    auto val = store.get("name");

    assert(val.has_value());
    assert(*val == "divyansh");

    // Test GET for missing key
    auto missing = store.get("age");
    assert(!missing.has_value());

    // Test UPDATE
    store.set("name", "new_name");
    val = store.get("name");

    assert(val.has_value());
    assert(*val == "new_name");

    // Test DELETE
    assert(store.remove("name"));
    val = store.get("name");
    assert(!val.has_value());

    // Test DELETE nonexistent key
    assert(!store.remove("does_not_exist"));
    std::cout << "All tests passed!\n";
}