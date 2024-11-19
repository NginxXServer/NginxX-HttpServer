#include "server.h"
#include "../document/document.h"
#include "../http/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTNUM 39020 // 서버 포트 번호
#define BUFSIZE 1024  // 버퍼 크기

int start_server(void) {
    int sd, ns;
    struct sockaddr_in sin, cli;
    socklen_t clientlen = sizeof(cli);
    char buf[BUFSIZE];
    char doc_name[256];

    // 서버 소켓 생성
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 소켓 주소 설정
    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = INADDR_ANY;

    // 소켓 바인딩
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        perror("bind");
        close(sd);
        exit(1);
    }

    // 연결 대기 상태 설정
    if (listen(sd, 5) == -1) {
        perror("listen");
        close(sd);
        exit(1);
    }

    printf("Server Port %d\n", PORTNUM);

    // 클라이언트 요청
    while (1) {
        // 클라이언트 연결 수락
        if ((ns = accept(sd, (struct sockaddr *)&cli, &clientlen)) == -1) {
            perror("accept");
            continue;
        }

        printf("Client 연결 (%s)\n", inet_ntoa(cli.sin_addr));

        // 클라이언트 요청 수신
        int bytes_read = recv(ns, buf, BUFSIZE - 1, 0);
        if (bytes_read > 0) {
            buf[bytes_read] = '\0'; // 요청 끝에 null 추가
            printf("docName %s\n", buf);

            // 요청 파싱
            int parse_result = parse_http_request(buf, doc_name, sizeof(doc_name));
            if (parse_result == 0) {
                // 문서 요청 처리
                handle_document_request(ns, doc_name);
            } else {
                // 잘못된 요청 처리
                char response[256];
                switch (parse_result) {
                    case 400:
                        snprintf(response, sizeof(response),
                                 "HTTP/1.1 400 Bad Request\r\n"
                                 "Content-Type: application/json\r\n\r\n"
                                 "{ \"status\": 400, \"error\": \"Invalid request format\" }"
                        );
                        break;
                    case 405:
                        snprintf(response, sizeof(response),
                                 "HTTP/1.1 405 Method Not Allowed\r\n"
                                 "Content-Type: application/json\r\n\r\n"
                                 "{ \"status\": 405, \"error\": \"Only GET method is allowed\" }"
                        );
                        break;
                    case 414:
                        snprintf(response, sizeof(response),
                                 "HTTP/1.1 414 URI Too Long\r\n"
                                 "Content-Type: application/json\r\n\r\n"
                                 "{ \"status\": 414, \"error\": \"Requested URI is too long\" }"
                        );
                        break;
                    default:
                        snprintf(response, sizeof(response),
                                 "HTTP/1.1 500 Internal Server Error\r\n"
                                 "Content-Type: application/json\r\n\r\n"
                                 "{ \"status\": 500, \"error\": \"Unknown server error\" }"
                        );
                        break;
                }
                send(ns, response, strlen(response), 0);
            }
        } else if (bytes_read == -1) {
            // 500 Internal Server Error: 서버 처리 중 오류가 발생할 경우
            perror("recv");
            const char *server_error =
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: application/json\r\n\r\n"
                "{ \"status\": 500, \"error\": \"Server error occurred\" }";
            send(ns, server_error, strlen(server_error), 0);
        }

        close(ns); // 클라이언트 소켓 닫기
    }

    close(sd); // 서버 소켓 닫기
    return 0;
}
