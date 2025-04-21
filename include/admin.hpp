#ifndef ADMIN_HPP
#define ADMIN_HPP

#include "user.hpp"
#include <string>
#include <ctime>

class Admin : public User {
private:
    std::string adminPassword;
    time_t registrationTime;
    
public:
    Admin(SOCKET socket, const std::string& name, const std::string& password, time_t regTime = time(nullptr));
    
    static bool isPasswordStrong(const std::string& pass) {
        return pass.length() >= 8;
    }
    
    bool operator<(const Admin& other) const {
        return this->getUsername().length() < other.getUsername().length();
    }
    
    time_t getRegistrationTime() const { return registrationTime; }
    bool authenticate(const std::string& pass) const;
    
    void sendMessage(const std::string& message) override;
    void receiveMessage(const std::string& message) override;
    
    // Добавляем виртуальный деструктор
    virtual ~Admin() = default;
};

#endif