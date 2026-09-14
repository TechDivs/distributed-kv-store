#pragma once

#include <winsock2.h>
#include <vector>
#include <string>

class TCPServer {
private:
    SOCKET server_socket;
    int port;
    struct Client {
        SOCKET socket;
        std::string buffer;
    };
    std::vector<Client> clients;

public:
    TCPServer(int port);
    void start();
};