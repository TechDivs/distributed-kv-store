#include <iostream>
#include "../include/tcp_server.hpp"
using std::cout;

int main() {
    try {
        TCPServer server(8080);
        server.start();
    }
    catch (const std::exception& e) {
        std::cerr << "Server failed: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}
