#include "server.h"
#include "../document/document.h"
#include "../http/parser.h"
#include "../logger/logger.h"
#include "tpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h> 

#define BASEPORT 39020 // 베이스 서버 포트 번호
#define BUFSIZE 1024  // 버퍼 크기
#define NUM_PROCESS 5 // 프로세스 개수
#define MAX_EVENTS 100 // 최대 이벤트 수
#define NUM_THREADS 4 // 스레드풀 크기 

void handle_client(int client_sock, int server_port);

// 스레드 풀에서 실행할 함수
static void thread_handle_client(void *arg) {
    client_data *data = (client_data *)arg;
    handle_client(data->client_sock, data->server_port);
    free(data);
}

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
            int optvalue = 1; // 소켓 옵션 값
            
            // 스레드 풀 생성
            tpool *thread_pool = tpool_create(NUM_THREADS);
            if (!thread_pool) {
                perror("Thread pool creation failed");
                exit(1);
            }

            // 새로운 소켓 생성
            if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("socket");
                exit(1);
            }

            // SO_REUSEADDR 설정
            if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue)) == -1) {
                perror("setsockopt");
                close(sd);
                exit(1);
            }

            // SO_REUSEPORT 설정
            if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &optvalue, sizeof(optvalue)) == -1) {
                perror("setsockopt SO_REUSEPORT");
                close(sd);
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

            // epoll 초기화
            int epoll_fd = epoll_create1(0);
            if (epoll_fd == -1) {
                perror("epoll_create1");
                close(sd);
                exit(1);
            }

            // LT 모드로 변경 (EPOLLET 플래그 제거)
            struct epoll_event event, events[MAX_EVENTS];
            event.data.fd = sd;
            event.events = EPOLLIN; // LT(Level Trigger) 방식으로 설정
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sd, &event) == -1) {
                perror("epoll_ctl");
                close(sd);
                close(epoll_fd);
                exit(1);
            }

            // 클라이언트 요청 처리 루프
            while (1) {
                int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
                if (num_events == -1) {
                    perror("epoll_wait");
                    break;
                }

                for (int j = 0; j < num_events; j++) {
                    if (events[j].events & (EPOLLERR | EPOLLHUP)) {
                        // 에러 발생 시 소켓 닫기                        
                        close(events[j].data.fd);
                        continue;
                    }

                    if (events[j].data.fd == sd) {
                        // 새로운 클라이언트 연결 처리
                        struct sockaddr_in cli;
                        socklen_t clientlen = sizeof(cli);
                        int ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                        
                        if (ns == -1) {
                            perror("accept");
                            continue;
                        }

                        log_message(child_port, LOG_INFO, "New Connection from IP: %s", inet_ntoa(cli.sin_addr));

                        // 클라이언트 소켓을 epoll에 등록 (LT 모드)
                        event.data.fd = ns;
                        event.events = EPOLLIN; // Level Trigger 모드
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ns, &event) == -1) {
                            perror("epoll_ctl: add client");
                            close(ns);
                            continue;
                        }
                    } else if (events[j].events & EPOLLIN) {
                        // 클라이언트 요청을 스레드 풀에서 처리
                        client_data *data = malloc(sizeof(client_data));
                        if (!data) {
                            perror("malloc");
                            close(events[j].data.fd);
                            continue;
                        }
                        
                        data->client_sock = events[j].data.fd;
                        data->server_port = child_port;

                        // epoll에서 클라이언트 소켓 제거
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[j].data.fd, NULL);

                        // 작업을 스레드 풀에 추가
                        if (!tpool_add_work(thread_pool, thread_handle_client, data)) {
                            free(data);
                            close(events[j].data.fd);
                            log_message(child_port, LOG_ERROR, "Failed to add work to thread pool");
                        }
                    }
                }
            }

            // 정리
            tpool_destroy(thread_pool);
            close(sd);
            close(epoll_fd);
            exit(0);
        }
    }
    
    signal(SIGCHLD, SIG_IGN);
    while (1) {
        sleep(1);
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
    if (getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len) == -1) {
        perror("getpeername failed");
        close(client_sock);
        return;
    }
    
    const char *client_ip = inet_ntoa(client_addr.sin_addr);  // 여기에 client_ip 정의 추가

    int bytes_read = recv(client_sock, buf, BUFSIZE - 1, 0);

    if (bytes_read > 0) {
        buf[bytes_read] = '\0';

        // HTTP 요청 파싱
        int parse_result = parse_http_request(buf, doc_name, sizeof(doc_name));
        if (parse_result == 0) {
            // 문서 요청 처리
            handle_document_request(client_sock, server_port, client_ip, doc_name);
        } else {
            // 잘못된 요청에 대한 응답
            char response[1024];
            int status_code;
            const char *error_title;
            const char *error_message;

            switch (parse_result) {
                case 400:
                    status_code = 400;
                    error_title = "400 Bad Request";
                    error_message = "The server could not understand your request.";
                    break;
                case 405:
                    status_code = 405;
                    error_title = "405 Method Not Allowed";
                    error_message = "The server only supports the GET method.";
                    break;
                case 414:
                    status_code = 414;
                    error_title = "414 URI Too Long";
                    error_message = "The requested URI is too long for the server to handle.";
                    break;
                default:
                    status_code = 500;
                    error_title = "500 Internal Server Error";
                    error_message = "An unknown error occurred on the server.";
                    break;
            }

            // HTML 응답 생성
            snprintf(response, sizeof(response),
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: text/html\r\n\r\n"
                     "<!DOCTYPE html>"
                     "<html>"
                     "<head><title>%s</title></head>"
                     "<body>"
                     "<h1>%s</h1>"
                     "<p>%s</p>"
                     "</body>"
                     "</html>",
                     status_code, error_title, error_title, error_title, error_message);

            send(client_sock, response, strlen(response), 0);
            log_http_response(server_port, client_ip, status_code, response);
        }
    } else if (bytes_read == -1) {
        // 500 Internal Server Error: 서버 처리 중 오류가 발생할 경우
        perror("recv");
        const char *server_error =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>500 Internal Server Error</title></head>"
            "<body>"
            "<h1>500 Internal Server Error</h1>"
            "<p>An error occurred while processing your request.</p>"
            "</body>"
            "</html>";

        send(client_sock, server_error, strlen(server_error), 0);
        log_http_response(server_port, client_ip, 500, server_error);
    }

    close(client_sock); // 작업 완료 후 소켓 닫기
}