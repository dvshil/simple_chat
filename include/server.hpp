#ifndef SERVER_HPP
#define SERVER_HPP

#include "common_constants.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <winsock2.h>

class User;

class Server {
public:
    Server(int port);
    ~Server();

    void start();
    void registerAdmin(const std::string& name, const std::string& password);

private:
    void handleClient(SOCKET clientSocket);
    void broadcastMessage(const std::string& message, const std::string& sender);
    void processAdminCommand(const std::string& adminName, const std::string& command);
    void sendMessageToAdmin(const std::string &adminName, const std::string &message);
    void processPrivateMessage(const std::string &sender, const std::string &message);
    void disconnectUser(const std::string& username);
    void disconnectAllClients();
    
    std::string generateServerKey();
    std::string encryptForClient(SOCKET socket, const std::string& message);
    std::string decryptForClient(SOCKET socket, const std::string& message);

    int port;
    std::atomic<bool> running;
    SOCKET serverSocket;
    std::unordered_map<std::string, std::shared_ptr<User>> connectedUsers;
    std::unordered_map<std::string, std::string> adminCredentials;
    std::unordered_map<SOCKET, std::string> clientKeys;
    std::vector<std::thread> clientThreads;
    std::mutex usersMutex;
    std::mutex keysMutex;
};

#endif