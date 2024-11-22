#include "server.h"
#include "../document/document.h"
#include "../http/parser.h"
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

void handle_client(int client_sock);

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

            printf("Process Index %d (PID %d): Starting on port %d\n", i + 1, getpid(), child_port);

            // 클라이언트 요청 처리 루프
            while (1) {
                struct sockaddr_in cli;
                socklen_t clientlen = sizeof(cli);
                int ns = accept(sd, (struct sockaddr *)&cli, &clientlen);

                if (ns == -1) {
                    perror("accept");
                    continue;
                }

                printf("Process %d: Client connected (%s) at port %d\n", i, inet_ntoa(cli.sin_addr), child_port);
                handle_client(ns); // 요청 처리
                close(ns);         // 소켓 닫기
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
void handle_client(int client_sock) {
    char buf[BUFSIZE];
    char doc_name[256];
    int bytes_read = recv(client_sock, buf, BUFSIZE - 1, 0);

    if (bytes_read > 0) {
        buf[bytes_read] = '\0'; // 요청 끝에 null 추가
        printf("요청 수신: %s\n", buf);

        // HTTP 요청 파싱
        int parse_result = parse_http_request(buf, doc_name, sizeof(doc_name));
        if (parse_result == 0) {
            // 문서 요청 처리
            handle_document_request(client_sock, doc_name);
        } else {
            // 잘못된 요청에 대한 응답
            char response[256];
            switch (parse_result) {
                case 400:
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 400 Bad Request\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 400, \"error\": \"Invalid request format\" }");
                    break;
                case 405:
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 405 Method Not Allowed\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 405, \"error\": \"Only GET method is allowed\" }");
                    break;
                case 414:
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 414 URI Too Long\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 414, \"error\": \"Requested URI is too long\" }");
                    break;
                default:
                    snprintf(response, sizeof(response),
                             "HTTP/1.1 500 Internal Server Error\r\n"
                             "Content-Type: application/json\r\n\r\n"
                             "{ \"status\": 500, \"error\": \"Unknown server error\" }");
                    break;
            }
            send(client_sock, response, strlen(response), 0);
        }
    } else if (bytes_read == -1) {
        // 500 Internal Server Error: 서버 처리 중 오류가 발생할 경우
        perror("recv");
        const char *server_error =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{ \"status\": 500, \"error\": \"Server error occurred\" }";
        send(client_sock, server_error, strlen(server_error), 0);
    }
}
