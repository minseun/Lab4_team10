//network.c
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int sock;

int connect_to_server(const char *ip, int port) {
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    return sock;
}

void send_message_to_server(const char *message) {
    if (send(sock, message, strlen(message), 0) == -1) {
        perror("Send failed");
    }
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }

    if (bytes_read == 0) {
        printf("Server disconnected.\n");
    } else {
        perror("recv");
    }

    close(sock);
    exit(0);
    return NULL;
}

void upload_file_to_server(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("File open failed");
        return;
    }

    // 서버에 파일 업로드 시작을 알림
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "FILE_UPLOAD:%s", file_path);
    send_message_to_server(buffer);

    // 파일 데이터를 서버에 전송
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sock, buffer, bytes_read, 0) == -1) {
            perror("File send failed");
            fclose(file);
            return;
        }
    }
    fclose(file);
    printf("File upload complete: %s\n", file_path);
}

void download_file_from_server(const char *file_name) {
    // 서버에 파일 다운로드 요청
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "FILE_DOWNLOAD:%s", file_name);
    send_message_to_server(buffer);

    // 파일 수신 준비
    FILE *file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to create file");
        return;
    }

    ssize_t bytes_received;
    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    if (bytes_received < 0) {
        perror("Failed to receive file data");
    }

    fclose(file);
    printf("File download complete: %s\n", file_name);
}
