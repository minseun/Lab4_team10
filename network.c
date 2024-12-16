#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int sock;

#define BUFFER_SIZE 1024
#define DOWNLOAD_DIR "/home/min/test_lab4/" // 파일을 저장할 디렉토리 경로 설정

// 서버와 연결하는 함수
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

// 서버에 메시지 전송하는 함수
void send_message_to_server(const char *message) {
    if (send(sock, message, strlen(message), 0) == -1) {
        perror("Send failed");
    }
}

// 서버로부터 메시지를 받는 함수
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

// 디렉토리가 없으면 생성하는 함수
void ensure_directory_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == -1) {
            perror("Failed to create directory");
            exit(1);
        }
    }
}

// 파일을 서버에 업로드하는 함수
void upload_file_to_server(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("File open failed");
        return;
    }

    // 서버에 파일 업로드 시작을 알림
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "FILE_UPLOAD:%s", file_path);
    send_message_to_server(buffer); // 서버에 업로드 시작 메시지 보내기

    // 파일 데이터를 서버로 전송
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

// 서버로부터 파일을 다운로드하는 함수
void download_file_from_server(const char *file_name) {
    // 다운로드할 디렉토리가 없으면 생성
    ensure_directory_exists(DOWNLOAD_DIR);

    char buffer[BUFFER_SIZE];

    // 서버에 파일 다운로드 요청
    snprintf(buffer, sizeof(buffer), "fileType:download,fileName:%s;", file_name);
    send_message_to_server(buffer);  // 서버에 요청 전송

    // 서버로부터 파일 크기 받기 (헤더에서 파일 크기 전송)
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive file size or header");
        return;
    }

    // 헤더에서 파일 크기 추출
    size_t file_size = 0;
    if (sscanf(buffer, "fileType:download,fileName:%*s,fileSize:%zu;", &file_size) != 1) {
        printf("잘못된 헤더 형식이 수신되었습니다.\n");
        return;
    }

    // 파일 수신 준비
    FILE *file = fopen(file_name, "wb");
    if (!file) {
        perror("파일 생성 실패");
        return;
    }

    // 파일 데이터 수신
    size_t total_received = 0;
    while (total_received < file_size) {
        bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("파일 데이터 수신 실패");
            fclose(file);
            return;
        }
        if (bytes_received == 0) {
            printf("서버와의 연결이 끊어졌습니다.\n");
            fclose(file);
            return;
        }

        // 받은 데이터를 파일에 씁니다.
        size_t written = fwrite(buffer, 1, bytes_received, file);
        if (written != bytes_received) {
            perror("파일 쓰기 실패");
            fclose(file);
            return;
        }

        total_received += bytes_received;
    }

    fclose(file);
    printf("downloaded: %s\n", file_name);
}

