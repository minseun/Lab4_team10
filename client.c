#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "network.h"

#define PORT 8080

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
