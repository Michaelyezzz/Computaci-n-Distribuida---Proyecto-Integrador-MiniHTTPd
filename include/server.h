#ifndef SERVER_H
#define SERVER_H

#include <sys/epoll.h>
#include <stdint.h>

#define MAX_CLIENTS 1024
#define MAX_EVENTS 64
#define BUFFER_SIZE 8192

typedef struct {
    int fd;
    char *buffer;
    size_t buffer_len;
    size_t buffer_pos;
} client_t;

typedef struct {
    int listen_fd;
    int epoll_fd;
    int port;
} server_t;

server_t* server_create(int port);
void server_run(server_t *server);
void server_destroy(server_t *server);

#endif