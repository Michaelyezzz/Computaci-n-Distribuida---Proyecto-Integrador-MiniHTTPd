#include "../include/server.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

server_t *global_server;

void signal_handler(int sig){
    if (sig == SIGINT && global_server){
        printf("\nServidor Detenido\n");
        server_destroy(global_server);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    signal(SIGINT, signal_handler);
    
    global_server = server_create(port);
    if (!global_server) {
        fprintf(stderr, "Error al crear servidor\n");
        return 1;
    }
    
    if (server_init(global_server) == -1) {
        fprintf(stderr, "Error al inicializar servidor\n");
        server_destroy(global_server);
        return 1;
    }
    
    server_run(global_server);
    
    return 0;
}