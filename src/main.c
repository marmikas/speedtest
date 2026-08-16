#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server.h"
#include "speedtest.h"
#include "location.h"
#include "best.h"

typedef enum
{
    MODE_NONE,
    MODE_DOWNLOAD,
    MODE_UPLOAD,
    MODE_LOCATION,
    MODE_BEST_SERVER,
    MODE_ALL
} Mode;

int main(int argc, char *argv[])
{
    Mode mode = MODE_NONE;
    int server_id = -1;
    Server *servers = NULL;
    size_t count = 0;

    int opt;

    while ((opt = getopt(argc, argv, "dulbs:ah")) != -1)
    {

        switch (opt)
        {

        case 'd':

            if (mode != MODE_NONE)
            {
                fprintf(stderr, "[Error]: Multiple modes specified\n");
                return 1;
            }

            mode = MODE_DOWNLOAD;
            printf("Download selected\n");
            break;

        case 'u':
            if (mode != MODE_NONE)
            {
                fprintf(stderr, "[Error]: Multiple modes specified\n");
                return 1;
            }

            mode = MODE_UPLOAD;
            printf("Upload selected\n");
            break;

        case 'l':

            if (mode != MODE_NONE)
            {
                fprintf(stderr, "[Error]: Multiple modes specified\n");
                return 1;
            }

            mode = MODE_LOCATION;
            break;
        case 'b':

            if (mode != MODE_NONE)
            {
                fprintf(stderr, "[Error]: Multiple modes specified\n");
                return 1;
            }

            mode = MODE_BEST_SERVER;
            printf("Best server selected\n");
            break;

        case 's':

            server_id = atoi(optarg);
            break;

        case 'a':

            if (mode != MODE_NONE)
            {
                fprintf(stderr, "[Error]: Multiple modes specified\n");
                return 1;
            }

            mode = MODE_ALL;
            printf("Full test selected\n");
            break;

        case 'h':
            printf("To use: ./speedtest [options]\n");
            printf("-d          Download\n");
            printf("-u          Upload\n");
            printf("-l          Location\n");
            printf("-b          Find best server\n");
            printf("-s ID       Select server\n");
            printf("-a          Full test\n");
            printf("-h          Show help\n");
            return 0;

        default:
            printf("Invalid option\n");
            return 1;
        }
    }
    if (mode == MODE_ALL && server_id >= 0)
    {
        fprintf(stderr, "[Error]: Full test does not require a server\n");
        return 1;
    }

    if (mode == MODE_LOCATION && server_id >= 0)
    {
        fprintf(stderr, "[Error]: Location mode does not require a server\n");
        return 1;
    }

    if (mode == MODE_DOWNLOAD && server_id == -1)
    {
        fprintf(stderr, "[Error]: Download test requires a server\n");
        return 1;
    }

    if (mode == MODE_UPLOAD && server_id == -1)
    {
        fprintf(stderr, "[Error]: Upload test requires a server\n");
        return 1;
    }
    if (mode == MODE_NONE)
    {
        fprintf(stderr, "[Error]: No test specified\n");
        return 1;
    }

    // DOWNLAOD AND UPLAOD MODE ONLY
    if (mode == MODE_DOWNLOAD || mode == MODE_UPLOAD)
    {

        if (load_servers("data/speedtest_server_list.json", &servers, &count) != 0)
        {
            return 1;
        }

        Server *selected_server = find_server_by_id(servers, count, server_id);

        if (selected_server == NULL)
        {
            fprintf(stderr, "[Error]: Server ID %d not found\n", server_id);

            free_servers(servers, count);
            return 1;
        }

        printf("Selected server:\n");
        print_server(selected_server);

        if (mode == MODE_DOWNLOAD)
        {
            double download_speed = 0.0;
            if (download_test(selected_server, &download_speed) != 0)
            {
                free_servers(servers, count);

                return 1;
            }
        }

        if (mode == MODE_UPLOAD)
        {
            double upload_speed = 0.0;
            if (upload_test(selected_server, &upload_speed) != 0)
            {
                free_servers(servers, count);

                return 1;
            }
        }

        free_servers(servers, count);
    }

    if (mode == MODE_LOCATION)
    {
        char country[64];

        if (get_location(country, sizeof(country)) != 0)
        {
            return 1;
        }

        printf("Location: %s\n", country);
    }

    if (mode == MODE_BEST_SERVER)
    {

        if (load_servers("data/speedtest_server_list.json", &servers, &count) != 0)
        {
            return 1;
        }

        char country[64];

        if (get_location(country, sizeof(country)) != 0)
        {
            free_servers(servers, count);
            return 1;
        }

        printf("[Result] Location: %s\n", country);

        Server *best_server = find_best_server(servers, count, country);

        printf("[Result] Best server: %s, %s - %s\n", best_server->city, best_server->country, best_server->provider);

        free_servers(servers, count);
    }

    if (mode == MODE_ALL)
    {
        char country[64];
        double download_speed = 0.0;
        double upload_speed = 0.0;

        printf("[1/4] Determining location...\n");

        if (get_location(country, sizeof(country)) != 0)
        {
            return 1;
        }

        printf("[Result] Location: %s\n\n", country);

        printf("[2/4] Loading servers and finding best server...\n");

        if (load_servers("data/speedtest_server_list.json", &servers, &count) != 0)
        {
            return 1;
        }

        Server *best_server =
            find_best_server(servers, count, country);

        if (best_server == NULL)
        {
            fprintf(stderr, "[Error]: No working server found in %s\n", country);

            free_servers(servers, count);
            return 1;
        }

        printf("\n[Result] Best server:\n");
        print_server(best_server);

        printf("[3/4] Testing download speed...\n");

        if (download_test(best_server, &download_speed) != 0)
        {
            free_servers(servers, count);
            return 1;
        }

        printf("\n[4/4] Testing upload speed...\n");

        if (upload_test(best_server, &upload_speed) != 0)
        {
            free_servers(servers, count);
            return 1;
        }

        printf("\n");
        printf("====================================\n");
        printf("          SPEEDTEST RESULTS\n");
        printf("====================================\n");
        printf("Location: %s\n", country);
        printf("Server:   %s - %s\n", best_server->city, best_server->provider);
        printf("Download: %.2f Mbps\n", download_speed);
        printf("Upload:   %.2f Mbps\n", upload_speed);
        printf("====================================\n");

        free_servers(servers, count);
    }

    return 0;
}