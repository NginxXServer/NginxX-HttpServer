#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handle_document_request(int client_sock, const char *doc_name) {
    char path[256];
    snprintf(path, sizeof(path), "./docs/%s", doc_name);

    // 파일 열기
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("파일이 없습니다.\n");
        return 1;
    }

    // 파일 내용 읽기
    char content[2048];
    size_t bytes_read = fread(content, 1, sizeof(content) - 1, file);
    fclose(file);
    content[bytes_read] = '\0';

    // HTTP 응답 생성
    char response[4096];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n\r\n"
             "{ \"docName\": \"%s\", \"content\": \"%s\" }",
             doc_name, content);
    
     printf("Response:\n%s\n", response);
}
