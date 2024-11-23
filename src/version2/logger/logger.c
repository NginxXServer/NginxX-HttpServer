#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "logger.h"

#define LOG_FILE "http_server.log" // 로그 파일 이름

void log_message(int server_port, LogLevel level, const char *format, ...) {
    FILE *file = fopen(LOG_FILE, "ab");
    if (!file) return;

    time_t now = time(NULL);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char *level_str = (level == LOG_INFO) ? "INFO" : "ERROR";

    fprintf(file, "[%s][Port %d][%s] ", time_buf, server_port, level_str); // 파일 출력
    printf("[%s][Port %d][%s] ", time_buf, server_port, level_str);        // 콘솔 출력

    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    vfprintf(file, format, args); 
    fprintf(file, "\n");

    vprintf(format, args_copy);   
    printf("\n");

    va_end(args);
    va_end(args_copy);

    fclose(file);
}

// HTTP 응답 로그를 처리
void log_http_response(int server_port, const char *client_ip, int status_code, const char *response_body) {
    LogLevel level = (status_code >= 400) ? LOG_ERROR : LOG_INFO;

    char first_line[256] = {0};
    strncpy(first_line, response_body, sizeof(first_line) - 1);
    char *newline = strchr(first_line, '\n');
    if (newline) *newline = '\0';

    const char *json_start = strstr(response_body, "{");
    const char *json_end = strstr(response_body, "}");
    
    if (json_start && json_end) {
        int json_length = json_end - json_start + 1;
        char json_part[1024] = {0};
        strncpy(json_part, json_start, json_length);
        log_message(server_port, level, "Client IP: %s, Status: %d, Response: %s", client_ip, status_code, json_part);
    } else {
        log_message(server_port, level, "Client IP: %s, Status: %d, Response: %s", client_ip, status_code, first_line);
    }
}
