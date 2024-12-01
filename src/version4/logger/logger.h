#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOG_INFO,  // 일반 정보
    LOG_ERROR  // 에러
} LogLevel;

// 기본 로그 함수
void log_message(int server_port, LogLevel level, const char *format, ...);

// HTTP 응답 전용 로그 함수
void log_http_response(int server_port, const char *client_ip, int status_code, const char *response_body);

#endif
