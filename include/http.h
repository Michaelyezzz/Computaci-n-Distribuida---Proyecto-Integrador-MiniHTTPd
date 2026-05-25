#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

typedef struct {
    char method[10];      
    char uri[2048];
    char version[10];
    char host[256];
    char connection[20];
    char user_agent[256];
} http_request_t;

typedef struct {
    int status_code;
    char *content_type;
    size_t content_length;
    char *body;
} http_response_t;

int http_parse_request(const char *raw_request, http_request_t *req);
http_response_t* http_create_response(int status_code, const char *content_type, const char *body, size_t body_len);
char* http_serialize_response(http_response_t *resp, size_t *total_len);
void http_free_response(http_response_t *resp);

#endif