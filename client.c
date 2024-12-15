#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include "network.h"

#define PORT 8080

#define BUFFER_SIZE 1024

// 서버에 파일을 전송하는 함수
void send_file_to_server(int sock, const char *file_name) {
    send(sock, "FILE", 4, 0);  // 명령어 전송

    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("File opening failed");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sock, buffer, bytes_read, 0) == -1) {
            perror("Failed to send file data");
            fclose(file);
            return;
        }
    }

    // 파일 전송 종료 신호 전송
    send(sock, "FILE_DONE", 9, 0);

    fclose(file);
    printf("File sent successfully\n");
}

// 서버에서 파일을 다운로드하는 함수
void receive_file_from_server(int sock, const char *file_name) {
    FILE *file = fopen(file_name, "wb");
    if (file == NULL) {
        perror("File opening failed");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    if (bytes_received == -1) {
        perror("Failed to receive file data");
    }

    fclose(file);
    printf("File received successfully\n");
}


int main() {
    char buffer[BUFFER_SIZE];
    char name[50];
    pthread_t recv_thread;

    // 서버에 연결
    if (connect_to_server("127.0.0.1", PORT) == -1) {
        exit(EXIT_FAILURE);
    }

    printf("서버에 연결되었습니다. 사용자 이름을 입력하세요: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0'; // 개행 제거
    send_message_to_server(name);

    // 메시지 수신 스레드 생성
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Thread creation failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 메시지 입력 및 전송
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        send_message_to_server(buffer);
    }

    return 0;
}
