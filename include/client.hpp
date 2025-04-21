#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "common_constants.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <winsock2.h>

class Client {
private:
    SOCKET clientSocket;
    std::atomic<bool> connected;
    std::thread receiveThread;
    std::string encryptionKey;
    
    void receiveMessages();
    void performKeyExchange();
    std::string generateEncryptionKey();
    
public:
    Client();
    ~Client();
    
    bool connect(const std::string& ip, int port, const std::string& username);
    void disconnect();
    void sendMessage(const std::string& message);
    void sendPrivateMessage(const std::string& target, const std::string& message);
    bool isConnected() const;
    
    std::string encrypt(const std::string& message);
    std::string decrypt(const std::string& message);
};

#endif