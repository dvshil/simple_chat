#include "../include/user.hpp"

User::User(SOCKET socket, const std::string& name, bool admin)
    : clientSocket(socket), username(name), isAdmin(admin) {}

std::string User::getUsername() const {
    return username;
}

bool User::isAdministrator() const {
    return isAdmin;
}
