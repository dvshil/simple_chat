#include "../include/server.hpp"
#include "../include/user.hpp"
#include "../include/admin.hpp"
#include "../include/chat_user.hpp"
#include "../include/common_constants.hpp"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <sstream>
#include <random>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

Server::Server(int port) : port(port), running(false), serverSocket(INVALID_SOCKET) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

std::string Server::generateServerKey() {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);
    std::string key(KEY_LENGTH, '\0');
    for (char& c : key) {
        c = static_cast<char>(dist(rd));
    }
    return key;
}

void Server::registerAdmin(const std::string& name, const std::string& password) {
    std::lock_guard<std::mutex> lock(usersMutex);
    adminCredentials[name] = password;
}

void Server::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Bind failed");
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Listen failed");
    }

    running = true;
    std::cout << "Server started on port " << port << std::endl;

    while (running) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        
        if (clientSocket == INVALID_SOCKET) {
            if (running) std::cerr << "Accept error: " << WSAGetLastError() << std::endl;
            continue;
        }

        clientThreads.emplace_back([this, clientSocket]() {
            this->handleClient(clientSocket);
        });
    }
}

void Server::handleClient(SOCKET clientSocket) {
    std::string username;
    bool isAdmin = false;

    try {
        // 1. Key exchange
        std::string serverKey = generateServerKey();
        if (send(clientSocket, serverKey.c_str(), serverKey.size(), 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send server key");
        }

        char clientKey[KEY_LENGTH];
        int bytes = recv(clientSocket, clientKey, sizeof(clientKey), 0);
        if (bytes != KEY_LENGTH) {
            throw std::runtime_error("Invalid client key");
        }

        {
            std::lock_guard<std::mutex> keyLock(keysMutex);
            clientKeys[clientSocket] = std::string(clientKey, KEY_LENGTH);
        }

        // 2. Username handling
        char usernameBuffer[256] = {0};
        bytes = recv(clientSocket, usernameBuffer, sizeof(usernameBuffer) - 1, 0);
        if (bytes <= 0) {
            throw std::runtime_error("Username receive failed");
        }
        username = usernameBuffer;

        // 3. Admin authentication
        if (adminCredentials.count(username)) {
            std::string authRequest = encryptForClient(clientSocket, "PASSWORD:");
            if (send(clientSocket, authRequest.c_str(), authRequest.size(), 0) == SOCKET_ERROR) {
                throw std::runtime_error("Password request failed");
            }

            char passwordBuffer[256];
            bytes = recv(clientSocket, passwordBuffer, sizeof(passwordBuffer), 0);
            if (bytes <= 0) {
                throw std::runtime_error("Password receive failed");
            }

            std::string password = decryptForClient(clientSocket, 
                                  std::string(passwordBuffer, bytes));
            if (adminCredentials[username] != password) {
                std::string response = encryptForClient(clientSocket, "AUTH_FAILED");
                send(clientSocket, response.c_str(), response.size(), 0);
                throw std::runtime_error("Invalid admin password");
            }
            isAdmin = true;
        }

        // 4. User registration
        std::shared_ptr<User> user;
    {
        std::lock_guard<std::mutex> userLock(usersMutex);
        
        if (connectedUsers.count(username)) {
            std::string response = encryptForClient(clientSocket, "NAME_TAKEN");
            send(clientSocket, response.c_str(), response.size(), 0);
            throw std::runtime_error("Username already in use");
        }

        // РЇРІРЅРѕРµ СЃРѕР·РґР°РЅРёРµ РЅСѓР¶РЅРѕРіРѕ С‚РёРїР° РїРѕР»СЊР·РѕРІР°С‚РµР»СЏ
        if (isAdmin) {
            user = std::shared_ptr<User>(
                new Admin(clientSocket, username, adminCredentials[username]),
                [](User* ptr) { delete static_cast<Admin*>(ptr); }
            );
        } else {
            user = std::shared_ptr<User>(
                new ChatUser(clientSocket, username),
                [](User* ptr) { delete static_cast<ChatUser*>(ptr); }
            );
        }

        connectedUsers[username] = user;
        std::string response = encryptForClient(clientSocket, "AUTH_SUCCESS");
        send(clientSocket, response.c_str(), response.size(), 0);
    }

        broadcastMessage(username + (isAdmin ? " (admin)" : "") + " joined", "");

        // 5. Main message loop
        char messageBuffer[BUFFER_SIZE];
        while (true) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(clientSocket, &readSet);
            timeval timeout{0, 100000}; // 100ms timeout

            int ready = select(0, &readSet, nullptr, nullptr, &timeout);
            if (ready == SOCKET_ERROR) break;

            if (ready > 0 && FD_ISSET(clientSocket, &readSet)) {
                bytes = recv(clientSocket, messageBuffer, sizeof(messageBuffer), 0);
                if (bytes <= 0) break;

                std::string message = decryptForClient(clientSocket, 
                    std::string(messageBuffer, bytes));

                if (message == "/quit") break;

                if (isAdmin && message.rfind("/admin ", 0) == 0) {
                    processAdminCommand(username, message.substr(7));
                } 
                else if (message.rfind("/pm ", 0) == 0) {
                    processPrivateMessage(username, message);
                }
                else {
                    broadcastMessage(username + ": " + message, username);
                }
            }

            // Check if still connected
            {
                std::lock_guard<std::mutex> userLock(usersMutex);
                if (connectedUsers.count(username) == 0) {
                    break; // Was kicked
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Client error (" << username << "): " << e.what() << std::endl;
    }

    // Cleanup
    if (!username.empty()) {
        bool wasKicked = false;
        {
            std::lock_guard<std::mutex> userLock(usersMutex);
            wasKicked = (connectedUsers.count(username) == 0);
            if (!wasKicked) {
                connectedUsers.erase(username);
                broadcastMessage(username + " left", "");
            }
        }
        {
            std::lock_guard<std::mutex> keyLock(keysMutex);
            clientKeys.erase(clientSocket);
        }
    }
    closesocket(clientSocket);
}


std::string Server::encryptForClient(SOCKET socket, const std::string& message) {
    std::lock_guard<std::mutex> lock(keysMutex);
    auto it = clientKeys.find(socket);
    if (it == clientKeys.end()) {
        throw std::runtime_error("No encryption key for client");
    }
    
    std::string result = message;
    const std::string& key = it->second;
    for (size_t i = 0; i < message.size(); ++i) {
        result[i] ^= key[i % key.size()];
    }
    return result;
}

std::string Server::decryptForClient(SOCKET socket, const std::string& message) {
    return encryptForClient(socket, message);
}

void Server::broadcastMessage(const std::string& message, const std::string& sender) {
    std::lock_guard<std::mutex> lock(usersMutex);
    
    for (const auto& [username, user] : connectedUsers) {
        if (username != sender) {  // РќРµ РѕС‚РїСЂР°РІР»СЏРµРј СЃРѕРѕР±С‰РµРЅРёРµ РѕС‚РїСЂР°РІРёС‚РµР»СЋ
            try {
                std::string encrypted = encryptForClient(user->getSocket(), message);
                user->sendMessage(encrypted);
            } catch (const std::exception& e) {
                std::cerr << "Broadcast error to " << username << ": " << e.what() << std::endl;
            }
        }
    }
}

void Server::processAdminCommand(const std::string& adminName, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    try {
        if (cmd == "checkpass") {
            std::string username;
            iss >> username;
            
            auto it = adminCredentials.find(username);
            if (it != adminCredentials.end()) {
                bool isStrong = Admin::isPasswordStrong(it->second);
                sendMessageToAdmin(adminName, "Password for " + username + " is " + 
                                  (isStrong ? "STRONG" : "WEAK"));
            } else {
                sendMessageToAdmin(adminName, "Admin not found");
            }
        }
        else if (cmd == "kick") {
            std::string target;
            iss >> target;
            
            if (target == adminName) {
                sendMessageToAdmin(adminName, "You cannot kick yourself");
            } else if (!connectedUsers.count(target)) {
                sendMessageToAdmin(adminName, "User not found");
            } else if (connectedUsers[target]->isAdministrator()) {
                sendMessageToAdmin(adminName, "Cannot kick other admins");
            } else {
                disconnectUser(target);
                sendMessageToAdmin(adminName, "Successfully kicked " + target);
            }
        }
        else if (cmd == "broadcast") { 
            std::string message;
            getline(iss, message);
            message.erase(0, message.find_first_not_of(" "));
            
            if (!message.empty()) {
                broadcastMessage("[SERVER NOTICE] " + message, adminName);
                sendMessageToAdmin(adminName, "Broadcast sent successfully");
            } else {
                sendMessageToAdmin(adminName, "Error: Empty message");
            }
        }
        else if (cmd == "compare") {
            std::string otherAdmin;
            iss >> otherAdmin;
            
            if (connectedUsers.count(otherAdmin)) {
                auto admin1 = std::dynamic_pointer_cast<Admin>(connectedUsers[adminName]);
                auto admin2 = std::dynamic_pointer_cast<Admin>(connectedUsers[otherAdmin]);
                
                if (admin1 && admin2) {
                    std::string msg = adminName + "'s name is " + 
                                    (*admin1 < *admin2 ? "SHORTER" : "LONGER") + 
                                    " than " + otherAdmin;
                    sendMessageToAdmin(adminName, msg);
                } else {
                    sendMessageToAdmin(adminName, "One of users is not admin");
                }
            } else {
                sendMessageToAdmin(adminName, "User not found");
            }
        }
        else if (cmd == "list") {
            std::lock_guard<std::mutex> lock(usersMutex);
            std::string userList = "Connected users (" + std::to_string(connectedUsers.size()) + "):\n";
            
            for (const auto& [name, user] : connectedUsers) {
                userList += "- " + name;
                if (user->isAdministrator()) {
                    userList += " (admin)";
                }
                userList += "\n";
            }
            sendMessageToAdmin(adminName, userList);
        }
        else {
            sendMessageToAdmin(adminName, 
                "Available admin commands:\n"
                "/admin kick <username>\n"
                "/admin broadcast <message>\n"  
                "/admin compare <username>\n"
                "/admin list\n"
                "/admin checkpass <username>");
        }
    } catch (const std::exception& e) {
        sendMessageToAdmin(adminName, "Error: " + std::string(e.what()));
    }
}


void Server::sendMessageToAdmin(const std::string& adminName, const std::string& message) {
    auto admin = connectedUsers.at(adminName);
    std::string encrypted = encryptForClient(admin->getSocket(), message);
    admin->sendMessage(encrypted);
}

void Server::processPrivateMessage(const std::string& sender, const std::string& message) {
    size_t space = message.find(' ', 4);
    if (space == std::string::npos) {
        sendMessageToAdmin(sender, "Invalid PM format. Use: /pm username message");
        return;
    }

    std::string target = message.substr(4, space - 4);
    std::string pmMessage = message.substr(space + 1);
    
    std::lock_guard<std::mutex> lock(usersMutex);
    
    auto targetIt = connectedUsers.find(target);
    auto senderIt = connectedUsers.find(sender);
    
    if (targetIt == connectedUsers.end()) {
        sendMessageToAdmin(sender, "User " + target + " not found");
        return;
    }
    
    // РћС‚РїСЂР°РІР»СЏРµРј РїРѕР»СѓС‡Р°С‚РµР»СЋ
    std::string receiverMsg = "[PM from " + sender + "]: " + pmMessage;
    targetIt->second->sendMessage(encryptForClient(targetIt->second->getSocket(), receiverMsg));
    
    // РћС‚РїСЂР°РІР»СЏРµРј РѕС‚РїСЂР°РІРёС‚РµР»СЋ РїРѕРґС‚РІРµСЂР¶РґРµРЅРёРµ
    std::string senderMsg = "[PM to " + target + "]: " + pmMessage;
    senderIt->second->sendMessage(encryptForClient(senderIt->second->getSocket(), senderMsg));
}

void Server::disconnectUser(const std::string& username) {
    std::unique_lock<std::mutex> usersLock(usersMutex);
    
    auto it = connectedUsers.find(username);
    if (it == connectedUsers.end() || it->second->isAdministrator()) return;

    SOCKET sock = it->second->getSocket();
    connectedUsers.erase(it);
    usersLock.unlock();

    // Send kick notification
    try {
        std::string msg = encryptForClient(sock, "You were kicked by admin");
        send(sock, msg.c_str(), msg.size(), 0);
    } catch (...) {}

    // Close socket after short delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    closesocket(sock);

    {
        std::lock_guard<std::mutex> keyLock(keysMutex);
        clientKeys.erase(sock);
    }

    broadcastMessage(username + " was kicked", "");
}

Server::~Server() {
    WSACleanup();
}
