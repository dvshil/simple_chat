#include "../include/client.hpp"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <random>

#pragma comment(lib, "ws2_32.lib")

Client::Client() : clientSocket(INVALID_SOCKET), connected(false) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

std::string Client::generateEncryptionKey() {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);
    std::string key(KEY_LENGTH, '\0');
    for (char& c : key) {
        c = static_cast<char>(dist(rd));
    }
    return key;
}

void Client::performKeyExchange() {
    char buffer[KEY_LENGTH];
    int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytes != KEY_LENGTH) {
        throw std::runtime_error("Invalid server key size");
    }
    
    encryptionKey = generateEncryptionKey();
    if (send(clientSocket, encryptionKey.c_str(), encryptionKey.size(), 0) != KEY_LENGTH) {
        throw std::runtime_error("Failed to send encryption key");
    }
}

bool Client::connect(const std::string& ip, int port, const std::string& username) {
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) return false;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(clientSocket);
        return false;
    }

    try {
        performKeyExchange();
        
        if (send(clientSocket, username.c_str(), username.size(), 0) == SOCKET_ERROR) {
            closesocket(clientSocket);
            return false;
        }
        
        connected = true;
        receiveThread = std::thread(&Client::receiveMessages, this);
        return true;
    } catch (...) {
        closesocket(clientSocket);
        return false;
    }
}

void Client::receiveMessages() {
    char buffer[BUFFER_SIZE];
    while (connected) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            connected = false;
            break;
        }
        
        std::string message = decrypt(std::string(buffer, bytes));
        std::cout << "\n" << message << "\n> ";
        std::cout.flush();
    }
}

std::string Client::encrypt(const std::string& message) {
    std::string result = message;
    for (size_t i = 0; i < message.size(); ++i) {
        result[i] ^= encryptionKey[i % KEY_LENGTH];
    }
    return result;
}

std::string Client::decrypt(const std::string& message) {
    return encrypt(message);
}

void Client::sendMessage(const std::string& message) {
    if (!connected) return;
    
    std::string encrypted = encrypt(message);
    if (send(clientSocket, encrypted.c_str(), encrypted.size(), 0) == SOCKET_ERROR) {
        disconnect();
    }
}

void Client::sendPrivateMessage(const std::string& target, const std::string& message) {
    sendMessage("/pm " + target + " " + message);
}

void Client::disconnect() {
    if (connected) {
        connected = false;
        closesocket(clientSocket);
        if (receiveThread.joinable()) receiveThread.join();
    }
}

bool Client::isConnected() const { 
    return connected; 
}

Client::~Client() {
    disconnect();
    WSACleanup();
}
