#include "../include/tcp_server.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

TCPServer::TCPServer(int port) {
    this->port = port;
    server_socket = INVALID_SOCKET;
}

void TCPServer::start() {
    // 1. Initialize Winsock
    WSADATA wsa_data;

    int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsa_data
    );

    if (result != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    // 2. Create socket
    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("socket() failed");
    }

    // 3. Configure server address
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 4. Bind socket to address + port
    result = bind(
        server_socket,
        reinterpret_cast<sockaddr*>(&server_addr),
        sizeof(server_addr)
    );

    if (result == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        throw std::runtime_error("bind() failed");
    }

    // 5. Start listening
    result = listen(server_socket, SOMAXCONN);

    if (result == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        throw std::runtime_error("listen() failed");
    }

    std::cout << "Server listening on port " << port << '\n';

    // 6. Accept clients
    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        // Watch the listening socket
        FD_SET(server_socket, &read_fds);

        // Watch all connected clients
        for (SOCKET client : clients) {
            FD_SET(client, &read_fds);
        }

        // Wait until at least one socket is ready
        int result = select(
            0,
            &read_fds,
            nullptr,
            nullptr,
            nullptr
        );

        if (result == SOCKET_ERROR) {
            throw std::runtime_error("select() failed");
        }

        // Check if a new client wants to connect
        if (FD_ISSET(server_socket, &read_fds)) {
            SOCKET client_socket = accept(
                server_socket,
                nullptr,
                nullptr
            );

            if (client_socket == INVALID_SOCKET) {
                continue;
            }

            clients.push_back(client_socket);

            std::cout << "Client connected!\n";
        }

        // Check existing clients
        for (SOCKET client : clients) {
            if (FD_ISSET(client, &read_fds)) {

                char buffer[1024];

                int bytes_received = recv(
                    client,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';

                    std::cout << "Received: "
                            << buffer << '\n';

                    const char* response = "Hello from server!";

                    send(
                        client,
                        response,
                        static_cast<int>(strlen(response)),
                        0
                    );
                }
            }
        }
    }
    

    // 10. Close server
    // closesocket(server_socket);

    // 11. Cleanup Winsock
    // WSACleanup();
}