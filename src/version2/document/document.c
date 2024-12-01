#include "document.h"
#include "../logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

// URI로부터 Content-Type 찾기
void find_mime(char *ct_type, const char *uri)
{
    const char *ext = strrchr(uri, '.'); // 확장자 찾기
    if (!ext)
    {
        strcpy(ct_type, "text/plain"); // 확장자가 없으면 기본값
        return;
    }

    if (!strcmp(ext, ".html"))
        strcpy(ct_type, "text/html");
    else if (!strcmp(ext, ".json"))
        strcpy(ct_type, "application/json");
    else if (!strcmp(ext, ".txt"))
        strcpy(ct_type, "text/plain");
    else if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg"))
        strcpy(ct_type, "image/jpeg");
    else if (!strcmp(ext, ".png"))
        strcpy(ct_type, "image/png");
    else
        strcpy(ct_type, "text/plain"); // 기본 MIME 타입
}

// 문서 요청 처리
void handle_document_request(int client_sock, int server_port, const char *client_ip, const char *doc_name)
{
    char path[256];
    snprintf(path, sizeof(path), "../../docs/%s", doc_name); // 문서 경로 생성

    // 문서 파일 열기
    FILE *file = fopen(path, "rb"); // 바이너리 모드로 열기
    char response[9999];
    if (!file)
    {
        // 404 Not Found: 파일이 없을 경우
        snprintf(response, sizeof(response),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/html\r\n\r\n"
                 "<!DOCTYPE html>"
                 "<html><head><title>404 Not Found</title></head>"
                 "<body><h1>404 Not Found</h1><p>Document not found.</p></body></html>");
        send(client_sock, response, strlen(response), 0);
        log_http_response(server_port, client_ip, 404, response);
        return;
    }

    // Content-Type 결정
    char content_type[50]; // Content-Type 저장할 버퍼
    find_mime(content_type, doc_name);

    // 응답 헤더 전송
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n\r\n",
             content_type);
    send(client_sock, response, strlen(response), 0);

    // 파일 내용 읽기 및 전송
    char buffer[8192];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        send(client_sock, buffer, bytes_read, 0);
    }

    fclose(file);
    log_http_response(server_port, client_ip, 200, response);
}
