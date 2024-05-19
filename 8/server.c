#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define OBSERVERS 2
#define CLIENTS 3
#define ISLAND_SIZE 20
#define SWAPS 1000

int next_section_index = 0;
int sections[ISLAND_SIZE];
int client_sockets[CLIENTS];
int observer_sockets[OBSERVERS];
char observer_message[256];

void send_message_to_observers(char* message) {
    for (int i = 0; i < OBSERVERS; i++) {
        if (write(observer_sockets[i], message, strlen(message)) == -1) {
            perror("Failed to send message to observer");
            exit(1);
        }
    }
}

void initialize_sections() {
    for (int i = 0; i < ISLAND_SIZE; i++) {
        sections[i] = ISLAND_SIZE - i;
    }

    for (int i = 0; i < SWAPS; i++) {
        int j = rand() % ISLAND_SIZE;
        int k = rand() % ISLAND_SIZE;

        int temp = sections[j];
        sections[j] = sections[k];
        sections[k] = temp;
    }
}

int send_instructions_to_clients() {
    int instructions_sent = 0;

    for (int i = 0; i < CLIENTS; i++) {
        if (next_section_index == ISLAND_SIZE) {
            break;
        }

        char instructions[256];
        sprintf(instructions, "search section %d", sections[next_section_index]);
        next_section_index++;

        if (write(client_sockets[i], instructions, strlen(instructions)) == -1) {
            perror("Failed to send instructions");
            exit(1);
        }

        instructions_sent++;
        printf("Instructions \"%s\" sent to client %d\n", instructions, i + 1);
        sprintf(observer_message, "Client %d is searching section %d\n", i + 1, sections[next_section_index - 1]);
        send_message_to_observers(observer_message);
    }
    
    return instructions_sent;
}

void send_termination_message_to_clients() {
    for (int i = 0; i < CLIENTS; i++) {
        const char* message = "stop work";
        if (write(client_sockets[i], message, strlen(message)) == -1) {
            perror("Failed to send termination message");
            exit(1);
        }
        printf("Instructions \"%s\" sent to client %d\n", message, i + 1);
        sprintf(observer_message, "Client %d is stopping it's work\n", i + 1);
        send_message_to_observers(observer_message);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Server port not specified\n");
        exit(1);
    }

    int server_port = atoi(argv[1]);
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    srand(time(NULL));
    initialize_sections();

    // Создание сокета
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Привязка сокета к адресу сервера
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind socket");
        exit(1);
    }

    // Прослушивание входящих соединений
    if (listen(server_socket, CLIENTS + OBSERVERS) == -1) {
        perror("Failed to listen for connections");
        exit(1);
    }

    printf("Waiting for connections...\n");

    int num_clients = 0;
    int num_observers = 0;

    while ((num_clients < CLIENTS) || (num_observers < OBSERVERS)) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("Failed to accept connection");
            exit(1);
        }

        char message[256];
        int bytes_read = read(client_socket, message, sizeof(message));

        if (bytes_read == -1) {
            perror("Failed to receive message");
            exit(1);
        }

        message[bytes_read] = '\0';

        if (strcmp(message, "client") == 0) {
            // Подключился клиент
            client_sockets[num_clients] = client_socket;
            num_clients++;

            printf("Client %d connected\n", num_clients);
        }

        if (strcmp(message, "observer") == 0) {
            // Подключился наблюдатель
            observer_sockets[num_observers] = client_socket;
            num_observers++;

            printf("Observer %d connected\n", num_observers);
        }
    }

    int found_treasure = 0;    

    do {
        // Отправка инструкций
        int instructions_sent = send_instructions_to_clients();

        for (int i = 0; i < instructions_sent; i++) {
            char message[256];
            int bytes_read = read(client_sockets[i], message, sizeof(message));
            
            if (bytes_read == -1) {
                perror("Failed to receive message");
                exit(1);
            }

            message[bytes_read] = '\0';
            if (strcmp(message, "treasure found") == 0) {
                // Клад был найден
                printf("Treasure found by client %d\n", i + 1);
                sprintf(observer_message, "Client %d found the treasure!\n", i + 1);
                send_message_to_observers(observer_message);
                found_treasure = 1;
            }
            else {
                sprintf(observer_message, "Client %d did not find the treasure\n", i + 1);
                send_message_to_observers(observer_message);
            }
        }
    } while (!found_treasure);

    // Отправка сообщений о завершении работы
    sleep(3);
    send_termination_message_to_clients();
    sprintf(observer_message, "Stopping work\n");
    send_message_to_observers(observer_message);

    // Заврершение работы сервера
    printf("Stopping work\n");
    for (int i = 0; i < CLIENTS; i++) {
        close(client_sockets[i]);
    }
    for (int i = 0; i < OBSERVERS; i++) {
        close(observer_sockets[i]);
    }
    close(server_socket);

    return 0;
}
