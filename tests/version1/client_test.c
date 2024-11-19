#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 39020
#define SERVER_ADDR "10.198.138.212" 
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_sent, bytes_received;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    const char *get_request = "GET /?docName=example HTTP/1.1\r\n"
                              "Host: 10.198.138.212\r\n"
                              "Connection: close\r\n\r\n";

    bytes_sent = send(sock, get_request, strlen(get_request), 0);
    if (bytes_sent < 0) {
        perror("send");
        close(sock);
        exit(1);
    }

    printf("서버 응답:\n");
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    if (bytes_received < 0) {
        perror("recv");
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
