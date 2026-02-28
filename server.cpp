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

// Функции преобразования кодировок (те же, что и в клиенте)
std::string utf8_to_cp866(const std::string& utf8_str) {
    if (utf8_str.empty()) return {};
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
    if (wlen == 0) return {};
    wchar_t* wbuf = new wchar_t[wlen];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wbuf, wlen);
    int clen = WideCharToMultiByte(866, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (clen == 0) {
        delete[] wbuf;
        return {};
    }
    char* cbuf = new char[clen];
    WideCharToMultiByte(866, 0, wbuf, -1, cbuf, clen, NULL, NULL);
    std::string result(cbuf);
    delete[] wbuf;
    delete[] cbuf;
    return result;
}

std::string cp866_to_utf8(const std::string& cp866_str) {
    if (cp866_str.empty()) return {};
    int wlen = MultiByteToWideChar(866, 0, cp866_str.c_str(), -1, NULL, 0);
    if (wlen == 0) return {};
    wchar_t* wbuf = new wchar_t[wlen];
    MultiByteToWideChar(866, 0, cp866_str.c_str(), -1, wbuf, wlen);
    int clen = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (clen == 0) {
        delete[] wbuf;
        return {};
    }
    char* cbuf = new char[clen];
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, cbuf, clen, NULL, NULL);
    std::string result(cbuf);
    delete[] wbuf;
    delete[] cbuf;
    return result;
}

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
        std::string leave_msg_utf8 = client_names[index] + " покинул чат\n";
        clients.erase(it);
        client_names.erase(client_names.begin() + index);
        closesocket(client);
        broadcast_message(leave_msg_utf8, INVALID_SOCKET);
        // Для вывода в консоль сервера преобразуем в CP866
        std::cout << utf8_to_cp866(leave_msg_utf8);
    }
}

void handle_client(SOCKET client_socket) {
    char buffer[1024];
    std::string client_name_utf8;

    // Получаем имя клиента (в UTF-8)
    int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        client_name_utf8 = buffer;
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client_names.push_back(client_name_utf8);
        }
        
        std::string welcome_msg_utf8 = "Добро пожаловать в чат, " + client_name_utf8 + "!\n";
        send(client_socket, welcome_msg_utf8.c_str(), welcome_msg_utf8.length(), 0);
        
        std::string join_msg_utf8 = client_name_utf8 + " присоединился к чату\n";
        broadcast_message(join_msg_utf8, client_socket);
        // Выводим в консоль сервера в CP866
        std::cout << utf8_to_cp866(join_msg_utf8);
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
        std::string msg_utf8 = buffer;
        std::string full_msg_utf8 = client_name_utf8 + ": " + msg_utf8;
        broadcast_message(full_msg_utf8, client_socket);
        // Для вывода в консоль сервера преобразуем в CP866
        std::cout << utf8_to_cp866(full_msg_utf8);
    }
}

int main() {
    // Устанавливаем кодировку консоли
    SetConsoleOutputCP(866);
    SetConsoleCP(866);

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

    std::cout << utf8_to_cp866("Сервер чата запущен на порту 8888") << std::endl;
    std::cout << utf8_to_cp866("Ожидание подключений...") << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << utf8_to_cp866("Ошибка accept") << std::endl;
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