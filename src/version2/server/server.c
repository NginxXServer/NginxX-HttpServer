#include "server.h"
#include "../document/document.h"
#include "../http/parser.h"
#include "../logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BASEPORT 39020 // 베이스 서버 포트 번호
#define BUFSIZE 1024  // 버퍼 크기
#define NUM_PROCESS 5 // 프로세스 개수

void handle_client(int client_sock, int server_port);

int start_server(void) {
    int sd;
    struct sockaddr_in sin;
    int i;

    for (i = 0; i < NUM_PROCESS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) { // 자식 프로세스
            int child_port = BASEPORT + i; // 각 자식의 고유 포트 계산
            
            // 새로운 소켓 생성
            if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("socket");
                exit(1);
            }

            // 소켓 주소 설정
            memset((char *)&sin, '\0', sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons(child_port);
            sin.sin_addr.s_addr = INADDR_ANY;

            // 소켓 바인딩
            if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
                perror("bind");
                close(sd);
                exit(1);
            }

            // 연결 대기 상태로 설정
            if (listen(sd, 5) == -1) {
                perror("listen");
                close(sd);
                exit(1);
            }

            log_message(child_port, LOG_INFO, "Starting Server: Port %d", child_port);

            // 클라이언트 요청 처리 루프
            while (1) {
                struct sockaddr_in cli;
                socklen_t clientlen = sizeof(cli);
                int ns = accept(sd, (struct sockaddr *)&cli, &clientlen);

                if (ns == -1) {
                    perror("accept");
                    continue;
                }

                log_message(child_port, LOG_INFO, "Connect Client: IP %s", inet_ntoa(cli.sin_addr));
                handle_client(ns, child_port); // 요청 처리
                close(ns); // 소켓 닫기
            }

            close(sd);
            exit(0); // 자식 프로세스 종료
        }
    }

    // 부모 프로세스가 자식 프로세스의 종료 상태를 무시하도록 설정
    // SIGCHLD 시그널이 발생하면, 부모는 자식의 종료 상태를 기다리지 않고 무시
    // => 인해 자식 프로세스가 종료되었을 때 좀비 프로세스를 방지
    signal(SIGCHLD, SIG_IGN);
    while (1) {
        sleep(1); // 무한 대기
    }

    return 0;
}

// 클라이언트 요청 처리 함수
void handle_client(int client_sock, int server_port) {
    char buf[BUFSIZE];
    char doc_name[256];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 클라이언트 정보 가져오기
    getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len);
    const char *client_ip = inet_ntoa(client_addr.sin_addr);

    int bytes_read = recv(client_sock, buf, BUFSIZE - 1, 0);

    if (bytes_read > 0) {
        buf[bytes_read] = '\0'; // 요청 끝에 null 추가

        // HTTP 요청 파싱
        int parse_result = parse_http_request(buf, doc_name, sizeof(doc_name));
        if (parse_result == 0) {
            // 문서 요청 처리
            handle_document_request(client_sock, server_port, client_ip, doc_name);
        } else {
            // 잘못된 요청에 대한 응답
            char response[256];
            int status_code;

            switch (parse_result) {
                case 400:
                    status_code = 400;
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 400 Bad Request\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 400, \"error\": \"Invalid request format\" }");
                    break;
                case 405:
                    status_code = 405;
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 405 Method Not Allowed\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 405, \"error\": \"Only GET method is allowed\" }");
                    break;
                case 414:
                    status_code = 414;
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 414 URI Too Long\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 414, \"error\": \"Requested URI is too long\" }");
                    break;
                default:
                    status_code = 500;
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 500 Internal Server Error\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 500, \"error\": \"Unknown server error\" }");
                    break;
            }

            send(client_sock, response, strlen(response), 0);
            log_http_response(server_port, client_ip, status_code, response);
        }
    } else if (bytes_read == -1) {
        // 500 Internal Server Error: 서버 처리 중 오류가 발생할 경우
        perror("recv");
        const char *server_error =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{ \"status\": 500, \"error\": \"Server error occurred\" }";
        send(client_sock, server_error, strlen(server_error), 0);
        log_http_response(server_port, client_ip, 500, server_error);
    }
}