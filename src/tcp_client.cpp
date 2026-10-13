#include <winsock2.h>
#include <iostream>
#include <string>
#include <stdexcept>

int main() {
    // 1. Initialize Winsock
    WSADATA wsa_data;

    int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsa_data
    );

    if (result != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // 2. Create socket
    SOCKET client_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client_socket == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        WSACleanup();
        return 1;
    }

    // 3. Configure server address
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);

    // 4. Connect to server
    result = connect(
        client_socket,
        reinterpret_cast<sockaddr*>(&server_addr),
        sizeof(server_addr)
    );

    if (result == SOCKET_ERROR) {
        std::cerr << "connect() failed\n";
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!\n";

    std::string command;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command == "EXIT") {
            break;
        }

        if (command.empty()) {
            continue;
        }

        // Server expects commands ending with '\n'
        command += '\n';

        // Send the complete command
        size_t total_sent = 0;

        while (total_sent < command.size()) {
            int bytes_sent = send(
                client_socket,
                command.c_str() + total_sent,
                static_cast<int>(command.size() - total_sent),
                0
            );

            if (bytes_sent == SOCKET_ERROR || bytes_sent == 0) {
                std::cerr << "send() failed\n";
                total_sent = 0;
                break;
            }

            total_sent += bytes_sent;
        }

        if (total_sent == 0) {
            break;
        }

        // Receive response from server
        char buffer[1024];

        int bytes_received = recv(
            client_socket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes_received <= 0) {
            std::cerr << "Server disconnected or recv() failed\n";
            break;
        }

        buffer[bytes_received] = '\0';

        std::cout << "Response: " << buffer;
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}