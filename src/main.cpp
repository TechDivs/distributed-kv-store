#include <iostream>
#include "../include/tcp_server.hpp"
using std::cout;

int main() {
    TCPServer server(8080);
    server.start();
    return 0;
}
