#include "../include/tcp_server.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

bool send_all(SOCKET socket, const std::string& data) {
    int total_sent = 0;
    int data_size = static_cast<int>(data.size());

    while (total_sent < data_size) {
        int bytes_sent = send(
            socket,
            data.c_str() + total_sent,
            data_size - total_sent,
            0
        );

        if (bytes_sent == SOCKET_ERROR) {
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

TCPServer::TCPServer(int port)
    : port(port),
    server_socket(INVALID_SOCKET),
    parser(kv_store) {
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
        for (const Client& client : clients) {
            FD_SET(client.socket, &read_fds);
        }

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

            clients.push_back({client_socket, ""});
            continue;
        }

        // Check existing clients
        for (auto it = clients.begin(); it != clients.end();) {
            SOCKET client = it->socket;
            if (FD_ISSET(client, &read_fds)) {
                char buffer[1024];

                int bytes_received = recv(
                    client,
                    buffer,
                    sizeof(buffer),
                    0
                );

                if (bytes_received == 0) {
                    std::cout << "Client disconnected!\n";
                    closesocket(client);
                    it=clients.erase(it);
                    continue;
                }
                
                if (bytes_received == SOCKET_ERROR) {
                    std::cout << "recv() failed, closing client socket\n";
                    closesocket(client);
                    it=clients.erase(it);
                    continue;
                }
                
                bool send_failed = false;
                it->buffer.append(buffer, bytes_received);
                while (true) {
                    size_t pos = it->buffer.find('\n');
                    if (pos == std::string::npos) break;

                    std::string request = it->buffer.substr(0, pos);
                    it->buffer.erase(0, pos + 1);

                    std::cout << "Request: " << request << '\n';

                    std::string response = parser.execute(request);
                    response += '\n';

                    if (!send_all(client, response)) {
                        std::cout << "send() failed\n";
                        send_failed = true;
                        break;
                    }
                }

                if(send_failed) {
                    closesocket(client);
                    it = clients.erase(it);
                    continue;
                }
                it++;
            }
        }
    }
    

    // 10. Close server
    // closesocket(server_socket);

    // 11. Cleanup Winsock
    // WSACleanup();
}