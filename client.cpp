#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

SOCKET client_socket;
bool running = true;

void receive_messages() {
    char buffer[1024];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << "\nСоединение с сервером потеряно" << std::endl;
            running = false;
            break;
        }
        buffer[bytes] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "Подключение к серверу..." << std::endl;
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Подключено к серверу" << std::endl;
    
    // Ввод имени
    std::string name;
    std::cout << "Введите ваше имя: ";
    std::getline(std::cin, name);
    
    // Отправляем имя на сервер
    send(client_socket, name.c_str(), name.length(), 0);

    std::cout << "Добро пожаловать в чат, " << name << "!" << std::endl;
    std::cout << "Введите 'exit' для выхода" << std::endl;

    // Запускаем поток для получения сообщений
    std::thread receiver(receive_messages);

    // Отправка сообщений
    std::string message;
    while (running) {
        std::getline(std::cin, message);
        
        if (message == "exit" || message == "/quit") {
            running = false;
            break;
        }
        
        if (!message.empty()) {
            message += "\n";
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }

    closesocket(client_socket);
    WSACleanup();
    
    if (receiver.joinable()) {
        receiver.join();
    }
    
    return 0;
}