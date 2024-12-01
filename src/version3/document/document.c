#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include "document.h"
#include "../logger/logger.h"

#define CHUNK_SIZE (1024 * 1024) // 1MB

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

void handle_document_request(int client_sock, int server_port, const char *client_ip, const char *doc_name)
{
    char path[256];
    snprintf(path, sizeof(path), "../../docs/%s", doc_name);

    FILE *file = fopen(path, "rb");
    char response[9999];
    if (!file)
    {
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

    char content_type[50];
    find_mime(content_type, doc_name);

    // 응답 헤더 전송
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n\r\n",
             content_type);
    send(client_sock, response, strlen(response), 0);

    // 파일 내용 전송
    char *buffer = malloc(CHUNK_SIZE);
    if (!buffer)
    {
        fclose(file);
        return;
    }

    while (1)
    {
        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, file);
        if (bytes_read == 0)
            break; // EOF 또는 에러

        size_t total_sent = 0;
        while (total_sent < bytes_read)
        {
            ssize_t sent = send(client_sock, buffer + total_sent, bytes_read - total_sent, 0);
            if (sent < 0)
            {
                if (errno == 11)
                {
                    usleep(1000); // 1ms 대기
                    continue;
                }
                // 실제 에러 발생
                free(buffer);
                fclose(file);
                return;
            }
            total_sent += sent;
        }
    }

    free(buffer);
    fclose(file);
    log_http_response(server_port, client_ip, 200, response);
}