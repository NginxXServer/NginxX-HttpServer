#include "document.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

// 문서 요청 처리
void handle_document_request(int client_sock, const char *doc_name) {
    char path[256];
    snprintf(path, sizeof(path), "../../docs/%s", doc_name); // 문서 경로 생성

    // 문서 파일 열기
    FILE *file = fopen(path, "r");
    char response[4096];
    if (!file) {
        // 404 Not Found: 파일이 없을 경우
        snprintf(response, sizeof(response),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json\r\n\r\n"
                 "{ \"status\": 404, \"error\": \"Document not found\" }");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    // 파일 내용 읽기
    char content[2048];
    size_t bytes_read = fread(content, 1, sizeof(content) - 1, file);
    fclose(file);
    content[bytes_read] = '\0'; // 파일 내용을 문자열로 처리

    // HTTP 응답 생성 
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n\r\n"
             "{ \"status\": 200, \"docName\": \"%s\", \"content\": \"%s\" }",
             doc_name, content);

    send(client_sock, response, strlen(response), 0); // 응답 전송
}
