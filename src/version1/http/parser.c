#include "parser.h"
#include <stdio.h>
#include <string.h>

// HTTP 요청 파싱 함수
int parse_http_request(const char *request, char *doc_name, size_t max_len) {
    // GET 요청 확인
    if (strncmp(request, "GET ", 4) != 0) {
        printf("GET 요청이 아닙니다.\n");
        return 405;  // Method Not Allowed
    }

    // docName 찾기
    const char *key = "docName=";
    const char *start = strstr(request, key);
    if (!start) {
        printf("docName이 없습니다.\n");
        return 400;  // Bad Request
    }

    // docName 추출
    start += strlen(key);
    const char *end = strchr(start, ' ');

    if (!end) {
        printf("요청이 잘못되었습니다.\n");
        return 400;  // Bad Request
    }

    // docName 길이 확인
    size_t len = end - start;
    if (len >= max_len) {
        printf("docName이 최대 길이를 초과하였습니다.\n");
        return 414;  // URI Too Long
    }

    // docName 저장
    strncpy(doc_name, start, len);
    doc_name[len] = '\0';

    return 0;  // 성공
}
