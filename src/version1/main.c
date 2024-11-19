#include <stdio.h>
#include <stdlib.h>

extern int start_server(void);

int main(void) {
    printf("서버를 시작합니다...\n");
    start_server();
    return 0;
}