#ifndef SERVER_H
#define SERVER_H

#include <unistd.h>  // sysconf()를 위한 헤더

// 클라이언트 데이터를 위한 구조체 정의
typedef struct {
    int client_sock;
    int server_port;
} client_data;

// 서버 시작 함수
int start_server(void);

// 클라이언트 요청 처리 함수
void handle_client(int client_sock, int server_port);

// 소켓을 비동기로 설정하는 함수
int make_socket_non_blocking(int sock);

#endif