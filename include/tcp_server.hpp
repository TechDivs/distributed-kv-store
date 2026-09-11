#pragma once

#include <winsock2.h>
#include <vector>

class TCPServer {
private:
    SOCKET server_socket;
    int port;
    std::vector<SOCKET> clients;

public:
    TCPServer(int port);
    void start();
};