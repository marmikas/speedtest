#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "server.h"

// Function to print servers
void print_server(const Server *server)
{

    if (server == NULL)
        return;

    printf("ID: %d\n", server->id);
    printf("City: %s\n", server->city);
    printf("Country: %s\n", server->country);
    printf("Provider: %s\n", server->provider);
    printf("Host: %s\n", server->host);
    printf("\n");
}

// Function to load servers from a file
// Params:
//  const char *filename - pointer to the file name string
//  Server **servers - double pointer to allocate memory and change the pointer in main()
//  size_t *count - pointer to count in main()
// Returns:
//  0 - successfully loaded servers from a file
//  1 - failed
int load_servers(const char *filename, Server **servers, size_t *count)
{

    // Checks the arguments
    if (filename == NULL || servers == NULL || count == NULL)
    {
        fprintf(stderr, "Invalid load_servers arguments\n");
        return 1;
    }

    // init outputs
    *servers = NULL;
    *count = 0;

    // Opening the file
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        // OS error message
        perror("fopen");

        return 1;
    }

    // Move file position to the end of the file
    if (fseek(file, 0, SEEK_END) != 0)
    {
        perror("fseek");
        fclose(file);

        return 1;
    }

    // Return the current position
    long size = ftell(file);
    if (size == -1L)
    {
        perror("ftell");
        fclose(file);

        return 1;
    }

    // Rewind to the start
    rewind(file);

    // Allocating the file buffer to store file contents (+1 because strings terminate with \0)
    char *data = malloc(size + 1);
    if (data == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        fclose(file);
        return 1;
    }

    // reading from file to data (returns the amount of elements read)
    size_t bytes = fread(data, 1, size, file);

    if (bytes != (size_t)size)
    {
        if (ferror(file))
            perror("fread");
        else
            fprintf(stderr, "Unexpected end of file\n");

        free(data);
        fclose(file);
        return 1;
    }

    // Adding a null terminator
    data[bytes] = '\0';

    // Parsing
    cJSON *root = cJSON_Parse(data);

    if (root == NULL)
    {
        fprintf(stderr, "Failed to parse JSON\n");

        free(data);
        fclose(file);

        return 1;
    }

    // Server JSON is expected to be an array
    if (!cJSON_IsArray(root))
    {
        fprintf(stderr, "JSON is not an array\n");
        cJSON_Delete(root);
        free(data);
        fclose(file);

        return 1;
    }

    // Getting JSON array size
    int json_count = cJSON_GetArraySize(root);

    if (json_count <= 0)
    {
        fprintf(stderr, "Server list is empty\n");

        cJSON_Delete(root);
        free(data);
        fclose(file);

        return 1;
    }

    // printf("Number of servers: %d\n", json_count);

    // Allocating space for structures
    // calloc to initialise memory to 0s/NULL because garbage values crash program when free()ing
    *servers = calloc(json_count, sizeof(Server));

    if (*servers == NULL)
    {
        fprintf(stderr, "Failed to allocate server array\n");

        cJSON_Delete(root);
        free(data);
        fclose(file);

        return 1;
    }

    // Looping throught the JSON array items
    for (int i = 0; i < json_count; i++)
    {
        cJSON *server_json = cJSON_GetArrayItem(root, i);
        // printf("Processing server %d\n", i);

        // Verify that it's an object
        if (server_json == NULL || !cJSON_IsObject(server_json))
        {
            fprintf(stderr, "Invalid server entry\n");

            free_servers(*servers, (size_t)json_count);
            *servers = NULL;

            cJSON_Delete(root);
            free(data);
            fclose(file);

            return 1;
        }

        cJSON *country = cJSON_GetObjectItem(server_json, "country");
        cJSON *city = cJSON_GetObjectItem(server_json, "city");
        cJSON *provider = cJSON_GetObjectItem(server_json, "provider");
        cJSON *host = cJSON_GetObjectItem(server_json, "host");
        cJSON *id = cJSON_GetObjectItem(server_json, "id");

        // type check
        if (!cJSON_IsString(country) || !cJSON_IsString(city) || !cJSON_IsString(provider) || !cJSON_IsString(host) || !cJSON_IsNumber(id))
        {
            fprintf(stderr, "Invalid server entry\n");

            free_servers(*servers, (size_t)json_count);
            *servers = NULL;

            cJSON_Delete(root);
            free(data);
            fclose(file);

            return 1;
        }

        // Dereference cJSON structure fields and copy them into our structure i
        (*servers)[i].id = id->valueint;
        (*servers)[i].country = strdup(country->valuestring);
        (*servers)[i].city = strdup(city->valuestring);
        (*servers)[i].provider = strdup(provider->valuestring);
        (*servers)[i].host = strdup(host->valuestring);

        // Check allocation
        if ((*servers)[i].country == NULL || (*servers)[i].city == NULL || (*servers)[i].provider == NULL || (*servers)[i].host == NULL)
        {

            fprintf(stderr, "Failed to allocate server strings\n");

            free_servers(*servers, (size_t)json_count);
            *servers = NULL;

            cJSON_Delete(root);
            free(data);
            fclose(file);

            return 1;
        }
    }

    // Return array size to caller in main()
    *count = (size_t)json_count;

    // CLEANUP
    cJSON_Delete(root);
    free(data);
    fclose(file);

    return 0;
}

// Free the structure pointers because free(servers) won't free them
void free_servers(Server *servers, size_t count)
{
    if (servers == NULL)
        return;

    for (size_t i = 0; i < count; i++)
    {
        free(servers[i].country);
        free(servers[i].city);
        free(servers[i].provider);
        free(servers[i].host);
    }

    free(servers);
}

// Function to find server by id
// Params:
//  Server *servers - pointer to first server
//  size_t count - number of servers
//  int id - server id
Server *find_server_by_id(Server *servers, size_t count, int id)
{

    for (size_t i = 0; i < count; i++)
    {

        if (servers[i].id == id)
        {
            // return address of the server
            return &servers[i];
        }
    }

    return NULL;
}