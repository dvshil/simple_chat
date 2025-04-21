#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <winsock2.h>

class User {
protected:
    SOCKET clientSocket;  // Добавляем хранение сокета
    std::string username;
    bool isAdmin;

public:
    User(SOCKET socket, const std::string& name, bool admin = false);
    virtual ~User() = default;

    SOCKET getSocket() const { return clientSocket; }  
    
    virtual void sendMessage(const std::string& message) = 0;
    virtual void receiveMessage(const std::string& message) = 0;
    
    std::string getUsername() const;
    bool isAdministrator() const;
};

#endif // USER_HPP