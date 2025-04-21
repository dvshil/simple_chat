#include "../include/admin.hpp"
#include <iostream>

Admin::Admin(SOCKET socket, const std::string& name, const std::string& password, time_t regTime)
    : User(socket, name, true), adminPassword(password), registrationTime(regTime) {}

bool Admin::authenticate(const std::string& pass) const {
    return adminPassword == pass;
}

void Admin::sendMessage(const std::string& message) {
    if (send(clientSocket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to send admin message");
    }
}

void Admin::receiveMessage(const std::string& message) {
    // Логирование для отладки
    std::cout << "Admin " << username << " received: " << message << std::endl;
    
    // Можно добавить дополнительную обработку для админ-сообщений
    if (message.find("[SERVER NOTICE]") != std::string::npos) {
        // Особая обработка серверных уведомлений
    }
}