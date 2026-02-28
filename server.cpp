#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

std::vector<SOCKET> clients;
std::vector<std::string> client_names;
std::mutex clients_mutex;

void broadcast_message(const std::string& message, SOCKET sender) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (SOCKET client : clients) {
        if (client != sender) {
            send(client, message.c_str(), message.length(), 0);
        }
    }
}

void remove_client(SOCKET client) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = std::find(clients.begin(), clients.end(), client);
    if (it != clients.end()) {
        int index = it - clients.begin();
        std::string leave_msg = client_names[index] + " покинул чат\n";
        clients.erase(it);
        client_names.erase(client_names.begin() + index);
        closesocket(client);
        broadcast_message(leave_msg, INVALID_SOCKET);
        std::cout << leave_msg;
    }
}

void handle_client(SOCKET client_socket) {
    char buffer[1024];
    std::string client_name;

    // Получаем имя клиента
    int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        client_name = buffer;
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client_names.push_back(client_name);
        }
        
        std::string welcome_msg = "Добро пожаловать в чат, " + client_name + "!\n";
        send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);
        
        std::string join_msg = client_name + " присоединился к чату\n";
        broadcast_message(join_msg, client_socket);
        std::cout << join_msg;
    }

    // Обрабатываем сообщения
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) {
            remove_client(client_socket);
            break;
        }
        
        buffer[bytes] = '\0';
        std::string message = client_name + ": " + buffer;
        broadcast_message(message, client_socket);
        std::cout << message;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Сервер чата запущен на порту 8888" << std::endl;
    std::cout << "Ожидание подключений..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}