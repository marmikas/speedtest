#include "cli.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static int set_mode(Options *options, Mode mode)
{
    if (options->mode != MODE_NONE)
    {
        fprintf(stderr, "[Error]: Multiple modes specified\n");
        return 1;
    }

    options->mode = mode;
    return 0;
}

void print_help(void)
{
    printf("To use: ./speedtest [options]\n");
    printf("-d          Download\n");
    printf("-u          Upload\n");
    printf("-l          Show location\n");
    printf("-L          List available servers and IDs\n");
    printf("--list-servers  List available servers and IDs\n");
    printf("-b          Find best server\n");
    printf("-s ID       Select server\n");
    printf("-a          Full test\n");
    printf("-h          Show help\n");
}

int parse_arguments(int argc, char *argv[], Options *options)
{
    if (options == NULL)
        return 1;

    options->mode = MODE_NONE;
    options->server_id = -1;

    int opt;
    int option_index = 0;

    static const struct option long_options[] = {
        {"download", no_argument, NULL, 'd'},
        {"upload", no_argument, NULL, 'u'},
        {"location", no_argument, NULL, 'l'},
        {"list-servers", no_argument, NULL, 'L'},
        {"best-server", no_argument, NULL, 'b'},
        {"server", required_argument, NULL, 's'},
        {"all", no_argument, NULL, 'a'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "dulLbs:ah", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'd':
            if (set_mode(options, MODE_DOWNLOAD) != 0)
                return 1;
            printf("Download selected\n");
            break;

        case 'u':
            if (set_mode(options, MODE_UPLOAD) != 0)
                return 1;
            printf("Upload selected\n");
            break;

        case 'l':
            if (set_mode(options, MODE_LOCATION) != 0)
                return 1;
            break;

        case 'L':
            if (set_mode(options, MODE_LIST_SERVERS) != 0)
                return 1;
            break;

        case 'b':
            if (set_mode(options, MODE_BEST_SERVER) != 0)
                return 1;
            printf("Best server selected\n");
            break;

        case 's':
            options->server_id = atoi(optarg);
            break;

        case 'a':
            if (set_mode(options, MODE_ALL) != 0)
                return 1;
            printf("Full test selected\n");
            break;

        case 'h':
            print_help();
            return 2;

        default:
            printf("Invalid option\n");
            return 1;
        }
    }

    return 0;
}

int validate_options(const Options *options)
{
    if (options == NULL)
        return 1;

    if (options->mode == MODE_ALL && options->server_id >= 0)
    {
        fprintf(stderr, "[Error]: Full test does not require a server\n");
        return 1;
    }

    if (options->mode == MODE_LIST_SERVERS && options->server_id >= 0)
    {
        fprintf(stderr, "[Error]: Server list mode does not require a server\n");
        return 1;
    }

    if (options->mode == MODE_LOCATION && options->server_id >= 0)
    {
        fprintf(stderr, "[Error]: Location mode does not require a server\n");
        return 1;
    }

    if (options->mode == MODE_DOWNLOAD && options->server_id == -1)
    {
        fprintf(stderr, "[Error]: Download test requires a server\n");
        return 1;
    }

    if (options->mode == MODE_UPLOAD && options->server_id == -1)
    {
        fprintf(stderr, "[Error]: Upload test requires a server\n");
        return 1;
    }

    if (options->mode == MODE_NONE)
    {
        fprintf(stderr, "[Error]: No test specified\n");
        return 1;
    }

    return 0;
}
