#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Server IP address and port not specified\n");
        exit(1);
    }

    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int client_socket;
    struct sockaddr_in server_addr;

    // Создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Установка соединения с сервером
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to connect to the server");
        exit(1);
    }

    char* first_message = "observer";
    if (write(client_socket, first_message, strlen(first_message)) == -1) {
        perror("Failed to send first message");
        exit(1);
    }

    printf("Connection to server %s:%d established\n", server_ip, server_port);

    // Цикл получения сообщений
    while (1) {
        // Получение инструкций от сервера
        char message[256];
        int bytes_read = read(client_socket, message, sizeof(message));
        if (bytes_read == -1) {
            perror("Failed to receive instructions");
            exit(1);
        }

        message[bytes_read] = '\0';

        printf("%s", message);

        if ((strcmp(message, "Stopping work\n") == 0) || (bytes_read == 0)) {
            break;
        }
    }

    // Закрытие соединения
    close(client_socket);

    return 0;
}
