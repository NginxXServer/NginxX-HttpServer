#include <stdio.h>
#include <string.h>

int parse_http_request(const char *request, char *doc_name, size_t max_len) {
    //GET 요청 확인
    if (strncmp(request, "GET ", 4) != 0) {
        printf("GET 요청이 아닙니다.\n");
        return 1;
    }

    //docName 찾기
    const char *key = "docName=";
    const char *start = strstr(request, key);
    if (!start) {
         printf("docName이 없습니다.\n");
        return 1;
    }

    //docName 추출
    start += strlen(key);
    const char *end = strchr(start, ' ');

    if (!end) {
         printf("요청이 잘못되었습니다.\n");
        return 1;  
    }

    //docName 길이 
    size_t len = end - start;
    if (len >= max_len) {
        printf("docName이 최대 길이를 초과하였습니다.\n");
        return 1;
    }

    //docName 저장
    strncpy(doc_name, start, len);
    doc_name[len] = '\0';
    
    return 0;
}

int main() {
    //test http 
    const char *http_request = "GET /?docName=example HTTP/1.1\r\nHost: 127.0.0.1:39020\r\n\r\n";
    char doc_name[128];

    if (parse_http_request(http_request, doc_name, sizeof(doc_name))) {
        return 1;
    } else {
        printf("docName: %s\n", doc_name);
    }

    return 0;
}