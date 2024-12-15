#ifndef NETWORK_H
#define NETWORK_H

#define BUFFER_SIZE 1024  // 이 부분을 정의

extern int sock;  // sock 변수 선언

// 서버 연결 함수
int connect_to_server(const char *ip, int port);

// 메시지 전송 함수
void send_message_to_server(const char *message);

// 메시지 수신 스레드 함수
void *receive_messages(void *arg);

#endif // NETWORK_H
