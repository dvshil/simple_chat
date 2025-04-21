#include "../include/client.hpp"
#include <iostream>
#include <string>
#include <conio.h>

int main() {
    try {
        Client client;
        
        std::string ip, username;
        int port;
        
        std::cout << "Enter server IP [127.0.0.1]: ";
        std::getline(std::cin, ip);
        if (ip.empty()) ip = "127.0.0.1";
        
        std::cout << "Enter server port [8080]: ";
        std::string portStr;
        std::getline(std::cin, portStr);
        port = portStr.empty() ? 8080 : std::stoi(portStr);
        
        std::cout << "Enter username: ";
        std::getline(std::cin, username);

        if (!client.connect(ip, port, username)) {
            std::cerr << "Connection failed!" << std::endl;
            return 1;
        }
        
        std::cout << "Connected! Commands:\n"
                  << "/pm <username> <message> - private message\n"
                  << "/admin <cmd> - admin commands (if admin)\n"
                  << "/quit - exit\n> ";
        
        std::string input;
        while (client.isConnected()) {
            if (_kbhit()) {
                char ch = _getch();
                
                if (ch == '\r') {
                    std::cout << std::endl;
                    if (input == "/quit") {
                        break;
                    }
                    
                    if (input.rfind("/pm ", 0) == 0) {
                        size_t space = input.find(' ', 4);
                        if (space != std::string::npos) {
                            std::string target = input.substr(4, space - 4);
                            std::string message = input.substr(space + 1);
                            client.sendPrivateMessage(target, message);
                        }
                    } else if (!input.empty()) {
                        client.sendMessage(input);
                    }
                    
                    input.clear();
                    std::cout << "> ";
                }
                else if (ch == '\b') {
                    if (!input.empty()) {
                        input.pop_back();
                        std::cout << "\b \b";
                    }
                }
                else {
                    input += ch;
                    std::cout << ch;
                }
            }
        }
        
        client.disconnect();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}