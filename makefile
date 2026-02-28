# Makefile для клиент-серверного чата
# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Os
LDFLAGS = -lws2_32 -static

# Исходные файлы
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

# Исполняемые файлы
SERVER_EXE = chat_server.exe
CLIENT_EXE = chat_client.exe

# Правила по умолчанию
all: $(SERVER_EXE) $(CLIENT_EXE)

# Сборка сервера
$(SERVER_EXE): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Сервер собран: $(SERVER_EXE)"

# Сборка клиента
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Клиент собран: $(CLIENT_EXE)"

# Очистка
clean:
	del /Q $(SERVER_EXE) $(CLIENT_EXE) 2>nul || rm -f $(SERVER_EXE) $(CLIENT_EXE)
	@echo "Очистка завершена"

# Установка (копирование в текущую папку)
install: all
	@echo "Готово к использованию"

# Запуск сервера
run-server: $(SERVER_EXE)
	$(SERVER_EXE)

# Запуск клиента
run-client: $(CLIENT_EXE)
	$(CLIENT_EXE)

# Информация
info:
	@echo "Доступные цели:"
	@echo "  all         - собрать сервер и клиент (по умолчанию)"
	@echo "  clean       - удалить исполняемые файлы"
	@echo "  run-server  - запустить сервер"
	@echo "  run-client  - запустить клиент"
	@echo "  install     - подготовить файлы"
	@echo "  info        - показать эту справку"

.PHONY: all clean install run-server run-client info