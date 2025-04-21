#include "../include/chat_user.hpp"
#include <iostream>

ChatUser::ChatUser(SOCKET socket, const std::string& name)
    : User(socket, name, false) {}

void ChatUser::sendMessage(const std::string& message) {
    send(clientSocket, message.c_str(), message.size(), 0);
    // Реальная отправка через сокет
}

void ChatUser::receiveMessage(const std::string& message) {
    std::cout << "Received message: " << message << std::endl;
    // Реальная логика получения сообщения
}