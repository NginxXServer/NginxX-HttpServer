#include <stdio.h>
#include "../../src/version1/http/parser.c"

int main() {
    const char *http_request = "GET /?docName=example HTTP/1.1\r\nHost: 127.0.0.1:39020\r\n\r\n";
    char doc_name[128];

    if (parse_http_request(http_request, doc_name, sizeof(doc_name))) {
        printf("테스트 실패: docName을 파싱하지 못했습니다.\n");
        return 1;
    } else {
        printf("테스트 성공: docName: %s\n", doc_name);
    }

    return 0;
}
