#ifndef CHAT_USER_HPP
#define CHAT_USER_HPP

#include "user.hpp"

class ChatUser : public User {
public:
    ChatUser(SOCKET socket, const std::string& name);
    
    void sendMessage(const std::string& message) override;
    void receiveMessage(const std::string& message) override;
};

#endif // CHAT_USER_HPP