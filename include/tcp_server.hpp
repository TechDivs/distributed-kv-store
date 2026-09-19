#pragma once

#include <winsock2.h>
#include <vector>
#include <string>
#include "parser.hpp"

class TCPServer {
private:
    SOCKET server_socket;
    int port;

    KVStore kv_store;
    Parser parser;
    
    struct Client {
        SOCKET socket;
        std::string buffer;
    };
    std::vector<Client> clients;

public:
    TCPServer(int port);
    void start();
    void stop();
};