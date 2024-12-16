#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "network.h"

#define PORT 8080

#define BUFFER_SIZE 1024

int main() {
    //char buffer[BUFFER_SIZE];
    char name[50];
    pthread_t recv_thread;
    char command[BUFFER_SIZE];
    struct sockaddr_in server;


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

    // 메시지,파일 입력 및 전송
    while (1) {
        // 채팅 메시지 입력 및 전송
        printf("Enter command (UPLOAD <file>, DOWNLOAD <file>, or message): ");
        fgets(command, sizeof(command), stdin);

        // 파일 업로드 명령 처리
        if (strncmp(command, "UPLOAD", 6) == 0) {
            char file_path[BUFFER_SIZE];
            sscanf(command + 7, "%s", file_path);  // 파일 경로 추출
            upload_file_to_server(file_path);     // 파일 업로드 함수 호출
        } 
        // 파일 다운로드 명령 처리
        else if (strncmp(command, "DOWNLOAD", 8) == 0) {
            char file_name[BUFFER_SIZE];
            sscanf(command + 9, "%s", file_name);  // 파일 이름 추출
            download_file_from_server(file_name); // 파일 다운로드 함수 호출
        } 
        // 일반 채팅 메시지 처리
        else {
            send_message_to_server(command);  // 채팅 메시지 전송
        }
    }

    return 0;
}
