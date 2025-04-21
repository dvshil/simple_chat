#include "../include/server.hpp"
#include <iostream>
#include <atomic>
#include <csignal>

std::atomic<bool> shutdown_flag(false);

void signalHandler(int signal) {
    shutdown_flag.store(true);
}

int main() {
    try {
        // РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј РѕР±СЂР°Р±РѕС‚С‡РёРє Ctrl+C
        signal(SIGINT, signalHandler);
        
        Server server(8080);
        server.registerAdmin("admin", "secret123");
        server.registerAdmin("root", "secret123");
        
        std::thread server_thread([&server]() {
            server.start();
        });

        std::cout << "Server running. Commands:\n"
                  << "stop - shutdown server\n"
                  << "Ctrl+C - emergency stop\n";

        std::string command;
        while (!shutdown_flag.load()) {
            if (std::getline(std::cin, command)) {
                if (command == "stop") {
                    shutdown_flag.store(true);
                    break;
                }
            }
        }

        // Р“СЂСѓР±Рѕ Р·Р°РІРµСЂС€Р°РµРј СЂР°Р±РѕС‚Сѓ
        std::cout << "Force shutting down server...\n";
        exit(0);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
