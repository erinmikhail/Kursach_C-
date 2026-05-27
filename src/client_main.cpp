#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Неверный адрес / Адрес не поддерживается\n";
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Ошибка подключения к серверу СУБД. Убедитесь, что сервер запущен.\n";
        return -1;
    }

    std::cout << "Подключено к СУБД (127.0.0.1:8080). Введите '.exit' для выхода.\n";

    std::string input;
    char buffer[8192] = {0};

    while (true) {
        std::cout << "dbms-client > ";
        std::getline(std::cin, input);

        if (input == ".exit") break;
        if (input.empty()) continue;

        send(sock, input.c_str(), input.length(), 0);
        
        memset(buffer, 0, 8192);
        int valread = read(sock, buffer, 8192);
        if (valread > 0) {
            std::cout << buffer;
        }
    }

    close(sock);
    return 0;
}