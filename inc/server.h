#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>

typedef struct
{
    int id;
    char *country;
    char *city;
    char *provider;
    char *host;
} Server;

int load_servers(const char *filename, Server **servers, size_t *count);

void free_servers(Server *servers, size_t count);

Server *find_server_by_id(Server *servers, size_t count, int id);

void print_server(const Server *server);

#endif
