#pragma once

#include <winsock2.h>

class TCPServer {
private:
    SOCKET server_socket;
    int port;

public:
    TCPServer(int port);
    void start();
};