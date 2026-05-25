#include "../include/server.h"
#include "../include/http.h"
#include "../include/files.h"
#include "../include/mime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

server_t* server_create(int port) {
    server_t *server = malloc(sizeof(server_t));
    server->port = port;
    
    // Crear socket
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) {
        perror("socket");
        free(server);
        return NULL;
    }
    
    // Permitir reutilizar puerto
    int opt = 1;
    setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server->listen_fd);
        free(server);
        return NULL;
    }
    
    // Listen
    listen(server->listen_fd, 10);
    
    // Crear epoll
    server->epoll_fd = epoll_create1(0);
    if (server->epoll_fd < 0) {
        perror("epoll_create1");
        close(server->listen_fd);
        free(server);
        return NULL;
    }
    
    // Agregar socket al epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server->listen_fd;
    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->listen_fd, &ev);
    
    printf("Servidor escuchando en puerto %d\n", port);
    return server;
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void server_run(server_t *server) {
    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        int nfds = epoll_wait(server->epoll_fd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server->listen_fd) {
                // Nueva conexión
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                
                int client_fd = accept(server->listen_fd, 
                                      (struct sockaddr *)&client_addr, 
                                      &addr_len);
                if (client_fd >= 0) {
                    set_nonblocking(client_fd);
                    
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                    
                    printf("Cliente conectado: %s\n", 
                           inet_ntoa(client_addr.sin_addr));
                }
            } else {
                // Cliente existente
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                
                ssize_t nbytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (nbytes <= 0) {
                    epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                } else {
                    buffer[nbytes] = '\0';
                    
                    // Parsear solicitud
                    http_request_t req = {0};
                    if (http_parse_request(buffer, &req) == 0) {
                        http_response_t *resp = NULL;
                        
                        // Validar método
                        if (strcmp(req.method, "GET") != 0) {
                            resp = http_create_response(405, "text/plain", 
                                                       "Method Not Allowed", 18);
                        } 
                        // Obtener archivo
                        else {
                            // Usar www/ como raíz
                            char *filepath = file_normalize_path(
                                strcmp(req.uri, "/") == 0 ? "/index.html" : req.uri,
                                "./www"
                            );
                            
                            if (!filepath) {
                                resp = http_create_response(404, "text/plain",
                                                           "Not Found", 9);
                            } else if (!file_exists(filepath)) {
                                resp = http_create_response(404, "text/plain",
                                                           "Not Found", 9);
                                free(filepath);
                            } else {
                                size_t file_size;
                                char *file_content = file_read(filepath, &file_size);
                                
                                if (!file_content) {
                                    resp = http_create_response(500, "text/plain",
                                                               "Internal Server Error", 21);
                                } else {
                                    const char *mime = mime_get_type(filepath);
                                    resp = http_create_response(200, mime, 
                                                               file_content, file_size);
                                    free(file_content);
                                }
                                free(filepath);
                            }
                        }
                        
                        // Enviar respuesta
                        size_t resp_len;
                        char *serialized = http_serialize_response(resp, &resp_len);
                        send(client_fd, serialized, resp_len, 0);
                        
                        free(serialized);
                        http_free_response(resp);
                    }
                    
                    // Cerrar conexión
                    epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }
}

void server_destroy(server_t *server) {
    close(server->epoll_fd);
    close(server->listen_fd);
    free(server);
}