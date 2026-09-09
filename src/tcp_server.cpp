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
    while(true) {
        SOCKET client_socket = accept(
            server_socket,
            nullptr,
            nullptr
        );

        if (client_socket == INVALID_SOCKET) {
            closesocket(server_socket);
            WSACleanup();
            throw std::runtime_error("accept() failed");
        }

        std::cout << "Client connected!\n";

        // 7. Receive data
        char buffer[1024];

        int bytes_received = recv(
            client_socket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Received: " << buffer << '\n';

            // 8. Send response
            const char* response = "Hello from server!";

            send(
                client_socket,
                response,
                static_cast<int>(strlen(response)),
                0
            );
        }

        // 9. Close client connection
        closesocket(client_socket);
    }
    

    // 10. Close server
    // closesocket(server_socket);

    // 11. Cleanup Winsock
    // WSACleanup();
}