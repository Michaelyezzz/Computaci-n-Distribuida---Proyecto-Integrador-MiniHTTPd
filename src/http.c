#include "../include/http.h"
#include "../include/mime.h"
#include "../include/files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int http_parse_request(const char *raw_request, http_request_t *req) {
    if (!raw_request || !req) return -1;
    
    char *dup = strdup(raw_request);
    char *saveptr = NULL;
    
    char *line = strtok_r(dup, "\r\n", &saveptr);
    if (!line) {
        free(dup);
        return -1;
    }
    
    if (sscanf(line, "%9s %2047s %9s", req->method, req->uri, req->version) != 3) {
        free(dup);
        return -1;
    }
    
    memset(req->host, 0, sizeof(req->host));
    memset(req->connection, 0, sizeof(req->connection));
    memset(req->user_agent, 0, sizeof(req->user_agent));
    strcpy(req->connection, "close"); // default
    
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) && strlen(line) > 0) {
        char header[256], value[512];
        if (sscanf(line, "%255[^:]: %511[^\r\n]", header, value) == 2) {
            // Trim espacios
            while (*value == ' ') memmove(value, value + 1, strlen(value));
            
            if (strcasecmp(header, "Host") == 0) {
                strncpy(req->host, value, sizeof(req->host) - 1);
            } else if (strcasecmp(header, "Connection") == 0) {
                strncpy(req->connection, value, sizeof(req->connection) - 1);
            } else if (strcasecmp(header, "User-Agent") == 0) {
                strncpy(req->user_agent, value, sizeof(req->user_agent) - 1);
            }
        }
    }
    
    free(dup);
    return 0;
}

http_response_t* http_create_response(int status_code, const char *content_type, 
                                      const char *body, size_t body_len) {
    http_response_t *resp = malloc(sizeof(http_response_t));
    resp->status_code = status_code;
    resp->content_type = strdup(content_type ? content_type : "text/plain");
    resp->content_length = body_len;
    resp->body = malloc(body_len + 1);
    if (body) {
        memcpy(resp->body, body, body_len);
    }
    resp->body[body_len] = '\0';
    
    return resp;
}

const char* http_status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";      
        case 404: return "Not Found";
        case 405: return "Method Not Allowed"; 
        case 500: return "Internal Server Error";
        default:  return "Unknown Status";
    }
}

char* http_serialize_response(http_response_t *resp, size_t *total_len) {
    char *response = malloc(8192);
    
    const char *conn_status = (resp->status_code == 200) ? "keep-alive" : "close";

    int header_len = snprintf(response, 8192,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n"
        "\r\n",
        resp->status_code,
        http_status_text(resp->status_code),
        resp->content_type,
        resp->content_length,
        conn_status
    );
    
    *total_len = header_len + resp->content_length;
    char *full_response = malloc(*total_len + 1);
    
    memcpy(full_response, response, header_len);
    memcpy(full_response + header_len, resp->body, resp->content_length);
    full_response[*total_len] = '\0';
    
    free(response);
    return full_response;
}

void http_free_response(http_response_t *resp) {
    if (resp) {
        free(resp->content_type);
        free(resp->body);
        free(resp);
    }
}