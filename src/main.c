#include "cli.h"
#include "commands.h"

int main(int argc, char *argv[])
{
    Options options;

    int parse_result = parse_arguments(argc, argv, &options);

    if (parse_result == 2)
        return 0;

    if (parse_result != 0)
        return 1;

    if (validate_options(&options) != 0)
        return 1;

    return run_command(&options);
}
