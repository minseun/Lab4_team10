#ifndef NETWORK_H
#define NETWORK_H

#define BUFFER_SIZE 1024 

extern int sock;  

// 서버 연결 함수
int connect_to_server(const char *ip, int port);

// 메시지 전송 함수
void send_message_to_server(const char *message);

// 메시지 수신 스레드 함수
void *receive_messages(void *arg);

// 파일 업로드 함수
void upload_file_to_server(const char *file_path);

// 파일 다운로드 함수
void download_file_from_server(const char *file_name);

// 디렉토리 존재 여부 확인 및 없으면 생성 함수
void ensure_directory_exists(const char *path);

#endif 
