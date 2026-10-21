#pragma once

#include <winsock2.h>
#include <vector>
#include <string>
#include "parser.hpp"
#include "wal.hpp"
#include "snapshot.hpp"
#include <cstdint>

class TCPServer {
private:
    SOCKET server_socket;
    int port;

    WAL wal;
    KVStore kv_store;
    Parser parser;
    Snapshot snapshot;
    
    struct Client {
        SOCKET socket;
        std::string buffer;
    };
    std::vector<Client> clients;
    uint64_t last_snapshot_lsn=0;
public:
    TCPServer(int port);
    void start();
    void stop();
    void maybe_create_snapshot();
};