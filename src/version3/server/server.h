#ifndef SERVER_H
#define SERVER_H

// 서버 시작 함수
int start_server(void);

// 클라이언트 요청 처리 함수
void handle_client(int client_sock, int server_port);

#endif
