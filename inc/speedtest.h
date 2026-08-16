#ifndef SPEEDTEST_H
#define SPEEDTEST_H

#include "server.h"

int download_test(const Server *server, double *speed_mbps);

int upload_test(const Server *server, double *speed_mbps);

#endif