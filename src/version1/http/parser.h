#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

int parse_http_request(const char *request, char *doc_name, size_t max_len);

#endif
