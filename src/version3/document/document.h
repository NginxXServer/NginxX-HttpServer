#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

void handle_document_request(int client_sock, int server_port, const char *client_ip, const char *doc_name);
void find_mime(char *ct_type, const char *uri);

#endif