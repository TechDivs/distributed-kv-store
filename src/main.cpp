#include <iostream>
#include "../include/kv_store.hpp"
using std::cout;

int main() {
    KVStore store;
    store.set("key","value");
    auto it = store.get("key");

    if(it) cout << *it << '\n';
    else cout << "Key not found" << '\n';
    return 0;
}
