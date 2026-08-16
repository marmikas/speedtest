#ifndef BEST_H
#define BEST_H

#include <stddef.h>
#include "server.h"
#include <stdlib.h>

int measure_latency(const Server *server, double *latency_ms);

Server *find_best_server(Server *servers, size_t count, const char *country);

#endif