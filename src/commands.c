#include "commands.h"

#include <stdio.h>

#include "best.h"
#include "location.h"
#include "server.h"
#include "speedtest.h"

#define SERVER_LIST_FILE "data/speedtest_server_list.json"

static int load_server_list(Server **servers, size_t *count)
{
    if (load_servers(SERVER_LIST_FILE, servers, count) != 0)
        return 1;

    return 0;
}

static int run_selected_server_test(const Options *options)
{
    Server *servers = NULL;
    size_t count = 0;

    if (load_server_list(&servers, &count) != 0)
        return 1;

    Server *selected_server = find_server_by_id(servers, count, options->server_id);

    if (selected_server == NULL)
    {
        fprintf(stderr, "[Error]: Server ID %d not found\n", options->server_id);
        free_servers(servers, count);
        return 1;
    }

    printf("Selected server:\n");
    print_server(selected_server);

    int result = 0;

    if (options->mode == MODE_DOWNLOAD)
    {
        double download_speed = 0.0;
        result = download_test(selected_server, &download_speed);
    }
    else
    {
        double upload_speed = 0.0;
        result = upload_test(selected_server, &upload_speed);
    }

    free_servers(servers, count);
    return result;
}

static int list_servers(void)
{
    Server *servers = NULL;
    size_t count = 0;

    if (load_server_list(&servers, &count) != 0)
        return 1;

    printf("Available servers (%zu):\n", count);
    printf("%-8s %-25s %-25s %-30s\n", "ID", "City", "Country", "Provider");
    printf("--------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < count; i++)
    {
        printf("%-8d %-25s %-25s %-30s\n",
               servers[i].id,
               servers[i].city,
               servers[i].country,
               servers[i].provider);
    }

    free_servers(servers, count);
    return 0;
}

static int show_location(void)
{
    char country[64];

    if (get_location(country, sizeof(country)) != 0)
        return 1;

    printf("Location: %s\n", country);
    return 0;
}

static int find_and_print_best_server(void)
{
    Server *servers = NULL;
    size_t count = 0;
    char country[64];

    if (load_server_list(&servers, &count) != 0)
        return 1;

    if (get_location(country, sizeof(country)) != 0)
    {
        free_servers(servers, count);
        return 1;
    }

    printf("[Result] Location: %s\n", country);

    Server *best_server = find_best_server(servers, count, country);

    if (best_server == NULL)
    {
        fprintf(stderr, "[Error]: No working server found in %s\n", country);
        free_servers(servers, count);
        return 1;
    }

    printf("[Result] Best server: %s, %s - %s\n",
           best_server->city,
           best_server->country,
           best_server->provider);

    free_servers(servers, count);
    return 0;
}

static int run_full_test(void)
{
    Server *servers = NULL;
    size_t count = 0;
    char country[64];
    double download_speed = 0.0;
    double upload_speed = 0.0;

    printf("[1/4] Determining location...\n");

    if (get_location(country, sizeof(country)) != 0)
        return 1;

    printf("[Result] Location: %s\n\n", country);
    printf("[2/4] Loading servers and finding best server...\n");

    if (load_server_list(&servers, &count) != 0)
        return 1;

    Server *best_server = find_best_server(servers, count, country);

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
    return 0;
}

int run_command(const Options *options)
{
    switch (options->mode)
    {
    case MODE_DOWNLOAD:
    case MODE_UPLOAD:
        return run_selected_server_test(options);

    case MODE_LIST_SERVERS:
        return list_servers();

    case MODE_LOCATION:
        return show_location();

    case MODE_BEST_SERVER:
        return find_and_print_best_server();

    case MODE_ALL:
        return run_full_test();

    case MODE_NONE:
    default:
        return 1;
    }
}
