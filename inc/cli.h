#ifndef CLI_H
#define CLI_H

typedef enum
{
    MODE_NONE,
    MODE_DOWNLOAD,
    MODE_UPLOAD,
    MODE_LOCATION,
    MODE_LIST_SERVERS,
    MODE_BEST_SERVER,
    MODE_ALL
} Mode;

typedef struct
{
    Mode mode;
    int server_id;
} Options;

int parse_arguments(int argc, char *argv[], Options *options);
int validate_options(const Options *options);
void print_help(void);

#endif
