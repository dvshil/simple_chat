#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345

std::string encryptDecrypt(const std::string& input, const std::string& key) {
    std::string output = input;
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ key[i % key.size()];
    }
    return output;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::string key;
    std::cout << "Enter encryption key: ";
    std::cin >> key;

    std::thread([clientSocket, key]() {
        char buffer[1024];
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                std::cerr << "Server disconnected." << std::endl;
                closesocket(clientSocket);
                exit(1);
            }

            std::string encryptedMessage(buffer, bytesReceived);
            std::string decryptedMessage = encryptDecrypt(encryptedMessage, key);
            std::cout << "Received: " << decryptedMessage << std::endl;
        }
    }).detach();

    std::string message;
    while (true) {
        std::cout << "Enter message: ";
        std::getline(std::cin, message);
        std::string encryptedMessage = encryptDecrypt(message, key);
        send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
