#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

SOCKET client_socket;
bool running = true;

// Функции преобразования кодировок
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

void receive_messages() {
    char buffer[1024];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << utf8_to_cp866("\nСоединение с сервером потеряно") << std::endl;
            running = false;
            break;
        }
        buffer[bytes] = '\0';
        // Полученные данные в UTF-8, конвертируем в CP866 для вывода
        std::string utf8_msg(buffer);
        std::string cp866_msg = utf8_to_cp866(utf8_msg);
        std::cout << cp866_msg << std::endl;
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

    std::cout << utf8_to_cp866("Подключение к серверу...") << std::endl;
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << utf8_to_cp866("Ошибка подключения") << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << utf8_to_cp866("Подключено к серверу") << std::endl;
    
    // Ввод имени (в консоли CP866)
    std::string name_cp866;
    std::cout << utf8_to_cp866("Введите ваше имя: ");
    std::getline(std::cin, name_cp866);
    
    // Преобразуем имя из CP866 в UTF-8 для отправки
    std::string name_utf8 = cp866_to_utf8(name_cp866);
    send(client_socket, name_utf8.c_str(), name_utf8.length(), 0);

    std::cout << utf8_to_cp866("Добро пожаловать в чат, ") << name_cp866 << "!" << std::endl;
    std::cout << utf8_to_cp866("Введите 'exit' для выхода") << std::endl;

    // Запускаем поток для получения сообщений
    std::thread receiver(receive_messages);

    // Отправка сообщений
    std::string message_cp866;
    while (running) {
        std::getline(std::cin, message_cp866);
        
        if (message_cp866 == "exit" || message_cp866 == "/quit") {
            running = false;
            break;
        }
        
        if (!message_cp866.empty()) {
            // Преобразуем сообщение из CP866 в UTF-8
            std::string message_utf8 = cp866_to_utf8(message_cp866);
            message_utf8 += "\n";
            send(client_socket, message_utf8.c_str(), message_utf8.length(), 0);
        }
    }

    closesocket(client_socket);
    WSACleanup();
    
    if (receiver.joinable()) {
        receiver.join();
    }
    
    return 0;
}