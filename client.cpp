#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
std::string username;

std::string encryptDecrypt(const std::string& input, const std::string& key) {
    std::string output = input;
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ key[i % key.size()];
    }
    return output;
}

void receiveMessages(SOCKET clientSocket, const std::string& key) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Server disconnected." << std::endl;
            closesocket(clientSocket);
            exit(1);
        }

        std::string encryptedMessage(buffer, bytesReceived);
        std::string decryptedMessage = encryptDecrypt(encryptedMessage, key);
        std::cout << decryptedMessage << std::endl;
        std::cout << "Enter message: \n";
    }
}

std::string getHSEArt() {
    return R"(
    ***    ***      *****       ********
    ***    ***    ***           ********
    ***    ***    ***           ***
    **********     ***          ********  
    **********      ***         ********
    ***    ***        ***       ***
    ***    ***      ****        ********
    ***    ***   *****          ********
)";
}

std::string getCatArt() {
    return R"(
╭━╮┈╭━╮┈┈┈┈┈╭━╮
┃╭╯┈┃┊┗━━━━━┛┊┃
┃╰┳┳┫┏━▅╮┊╭━▅┓┃
┃┫┫┫┫┃┊▉┃┊┃┊▉┃┃
┃┫┫┫╋╰━━┛▅┗━━╯╋
┃┫┫┫╋┊┊┊┣┻┫┊┊┊╋
┃┊┊┊╰┈┈┈┈┈┈┈┳━╯
┃┣┳┳━━┫┣━━┳╭╯

)";
}

std::string getAgroDronArt() {
    return R"(
       __|__
      /     \
     | () () |
  ___|       |___
 /               \
|--O--       --O--|
|                 |
|--O--       --O--|
 \_______________/
      |  |  |
      |  |  |
      |  |  |
     _|  |  |_
    (_________) 

)";
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    std::string key;
    std::cout << "Enter encryption key: ";
    std::cin >> key;
    std::cin.ignore(); 

    std::thread(receiveMessages, clientSocket, key).detach();

    std::string message;
    while (true) {
        std::cout << "Enter message: \n";
        std::getline(std::cin, message);

        if (message == "exit") {
            std::cout << "Exiting chat..." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 0;
        }

        if (message == "!hse") {
            message = getHSEArt(); 
        }

        if (message == "!cat") {
            message = getCatArt(); 
        }

        if (message == "!dron") {
            message = getAgroDronArt(); 
        }

        std::string fullMessage = "[" + username + "]: " + message;
        std::string encryptedMessage = encryptDecrypt(fullMessage, key);
        send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
