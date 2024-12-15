#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char name[50];
} Client;

Client clients[MAX_CLIENTS] = {0}; // 클라이언트 정보 저장
pthread_mutex_t lock;

int is_client_name_taken(const char *client_name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket > 0 && strcmp(clients[i].name, client_name) == 0) {
            return 1; // 이름이 이미 존재
        }
    }
    return 0; // 이름이 사용되지 않음
}

void broadcast_message(const char *message, int sender_socket) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket > 0 && clients[i].socket != sender_socket) {
            if (send(clients[i].socket, message, strlen(message), 0) == -1) {
                perror("Send failed");
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 50];
    int bytes_read;
    char client_name[50] = "";


    // HTTP 요청 무시 처리
    if (strncmp(buffer, "GET ", 4) == 0 || strncmp(buffer, "POST ", 5) == 0) {
        printf("HTTP request detected and ignored.\n");
        return NULL;  // HTTP 요청 감지 시 종료
    }



    // 클라이언트 이름 받기
    if (recv(client_socket, client_name, sizeof(client_name), 0) <= 0) {
        perror("Failed to receive client name");
        close(client_socket);
        free(arg);
        return NULL;
    }

    // 이름 중복 체크
    if (is_client_name_taken(client_name)) {
        const char *error_message = "The client name is already taken.\n";
        send(client_socket, error_message, strlen(error_message), 0);
        close(client_socket);
        free(arg);
        return NULL;
    }

    printf("Received client name: '%s'\n", client_name);

    // 클라이언트 정보 저장
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            strncpy(clients[i].name, client_name, sizeof(clients[i].name) - 1);
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    printf("%s joined the chat\n", client_name);
    snprintf(message, sizeof(message), "%s has joined the chat\n", client_name);
    broadcast_message(message, client_socket);

    // 메시지 수신 및 브로드캐스트
    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the received string

        // 최대 출력 길이를 계산하고 안전하게 설정
        int remaining_space = sizeof(message) - strlen(client_name) - 5; // " > " + '\0' 포함
        snprintf(message, sizeof(message), "%s > %.*s", client_name, remaining_space, buffer);

        printf("%s", message);
        broadcast_message(message, client_socket);
    }

    // 클라이언트 종료 처리
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket) {
            clients[i].socket = 0;
            memset(clients[i].name, 0, sizeof(clients[i].name));
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    printf("%s left the chat\n", client_name);
    snprintf(message, sizeof(message), "%s has left the chat\n", client_name);
    broadcast_message(message, client_socket);

    close(client_socket);
    free(arg);
    return NULL;
}

int main() {
    int server_socket, client_socket, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread;

    pthread_mutex_init(&lock, NULL);

    // 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인딩
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // 소켓 리스닝
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // 최대 클라이언트 수 초과 처리
        pthread_mutex_lock(&lock);
        int client_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket > 0) {
                client_count++;
            }
        }
        pthread_mutex_unlock(&lock);

        if (client_count >= MAX_CLIENTS) {
            const char *error_message = "Server is full. Try again later.\n";
            send(client_socket, error_message, strlen(error_message), 0);
            close(client_socket);
            continue;
        }

        printf("New connection: Client %d\n", client_socket);

        // 스레드 생성
        new_sock = malloc(sizeof(int));
        *new_sock = client_socket;
        if (pthread_create(&thread, NULL, handle_client, (void *)new_sock) != 0) {
            perror("Thread creation failed");
            free(new_sock);
        }
        pthread_detach(thread); // 독립적인 스레드
    }

    close(server_socket);
    pthread_mutex_destroy(&lock);
    return 0;
}
