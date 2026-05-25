#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_EVENTS 64

int make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }
    return 0;
}

server_t* server_create(int port) {
    server_t *server = malloc(sizeof(server_t));
    if (!server) return NULL;

    server->port = port;
    server->listen_fd = -1;
    server->epoll_fd = -1;
    return server;
}

int server_init(server_t *server) {
    struct sockaddr_in addr;

    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd == -1) {
        perror("Error al crear socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error en setsockopt (SO_REUSEADDR)");
        close(server->listen_fd);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(server->port);

    if (bind(server->listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Error en bind");
        close(server->listen_fd);
        return -1;
    }

    if (listen(server->listen_fd, QLEN) == -1) {
        perror("Error en listen");
        close(server->listen_fd);
        return -1;
    }

    server->epoll_fd = epoll_create1(0);
    if (server->epoll_fd == -1) {
        perror("Error en epoll_create1");
        close(server->listen_fd);
        return -1;
    }

    if (make_socket_non_blocking(server->listen_fd) == -1) {
        close(server->listen_fd);
        close(server->epoll_fd);
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server->listen_fd;
    if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->listen_fd, &ev) == -1) {
        perror("Error en epoll_ctl ADD listen_fd");
        close(server->listen_fd);
        close(server->epoll_fd);
        return -1;
    }

    printf("Servidor HTTP inicializado con éxito en el puerto %d\n", server->port);
    return 0;
}

void server_run(server_t *server) {
    struct epoll_event events[MAX_EVENTS];

    printf("MiniHTTPd corriendo... Esperando conexiones asíncronas.\n");

    while (1) {

        int nfds = epoll_wait(server->epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue; // Reintentar si es interrumpido por una señal del sistema
            perror("Error en epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server->listen_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server->listen_fd, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("Error en accept");
                    }
                    continue;
                }

                if (make_socket_non_blocking(client_fd) == -1) {
                    close(client_fd);
                    continue;
                }

                struct epoll_event ev;
                ev.events = EPOLLIN; 
                ev.data.fd = client_fd;
                if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("Error en epoll_ctl ADD client_fd");
                    close(client_fd);
                }
            } else {

                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                
                ssize_t nbytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (nbytes <= 0) {
                    epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                } else {
                    buffer[nbytes] = '\0'; // Asegurar fin de cadena para el parser
                    
                    http_request_t req = {0};
                    if (http_parse_request(buffer, &req) == 0) {
                        http_response_t *resp = NULL;
                        
                        if (strcmp(req.method, "GET") != 0) {
                            resp = http_create_response(405, "text/plain", "Method Not Allowed", 18);
                        } 
                        // Procesar la solicitud del archivo estático
                        else {
                            // Si solicita la raíz "/", mapearla automáticamente a /index.html
                            char *filepath = file_normalize_path(
                                strcmp(req.uri, "/") == 0 ? "/index.html" : req.uri,
                                "./www"
                            );

                            if (!filepath) {
                                resp = http_create_response(403, "text/plain", "Forbidden", 9); // <--- Cambiado a 403
                                } else if (!file_exists(filepath)) {
                                    resp = http_create_response(404, "text/plain", "Not Found", 9);
                                    free(filepath);                               
                            } else {
                                size_t file_size;
                                char *file_content = file_read(filepath, &file_size);
                                
                                if (!file_content) {
                                    resp = http_create_response(500, "text/plain", "Internal Server Error", 21);
                                } else {
                                    const char *mime = mime_get_type(filepath);
                                    resp = http_create_response(200, mime, file_content, file_size);
                                    free(file_content);
                                }
                                free(filepath);
                            }
                        }

                        size_t resp_len;
                        char *serialized = http_serialize_response(resp, &resp_len);
                        send(client_fd, serialized, resp_len, 0);
                        
                        free(serialized);
                        if (strcasecmp(req.connection, "keep-alive") != 0 || resp->status_code != 200) {
                            epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            close(client_fd);
                        } 

                        http_free_response(resp);
                    } else {
                        http_response_t *err_resp = http_create_response(400, "text/plain", "Bad Request", 11);
                        size_t err_len;
                        char *err_serialized = http_serialize_response(err_resp, &err_len);
                        
                        send(client_fd, err_serialized, err_len, 0);
                        
                        free(err_serialized);
                        http_free_response(err_resp);

                        epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        close(client_fd);
                    }
                }
            }
        }
    }
}

void server_destroy(server_t *server) {
    if (!server) return;
    if (server->listen_fd != -1) close(server->listen_fd);
    if (server->epoll_fd != -1) close(server->epoll_fd);
    free(server);
}