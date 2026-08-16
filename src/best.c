#include "best.h"

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

static size_t discard_callback(void *ptr, size_t size, size_t num_of_elements, void *userdata)
{
    (void)ptr;
    (void)userdata;

    return size * num_of_elements;
}
int measure_latency(const Server *server, double *latency_ms)
{
    if (server == NULL || latency_ms == NULL)
    {
        fprintf(stderr, "Invalid measure_latency arguments\n");
        return 1;
    }

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    char url[512];

    int written = snprintf(url, sizeof(url), "http://%s/speedtest/latency.txt", server->host);

    if (written < 0 || (size_t)written >= sizeof(url))
    {
        fprintf(stderr, "URL too long\n");
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_callback);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest-c/1.0");

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        fprintf(stderr, "Latency test failed: %s\n", curl_easy_strerror(result));

        curl_easy_cleanup(curl);
        return 1;
    }

    long response_code = 0;

    CURLcode code_result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (code_result != CURLE_OK)
    {
        fprintf(stderr, "Failed to get response code: %s\n", curl_easy_strerror(code_result));

        curl_easy_cleanup(curl);
        return 1;
    }

    if (response_code != 200)
    {
        fprintf(stderr, "Returned HTTP %ld\n", response_code);

        curl_easy_cleanup(curl);
        return 1;
    }

    double total_time = 0.0;

    CURLcode time_result = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

    if (time_result != CURLE_OK)
    {
        fprintf(stderr, "Failed to get latency time: %s\n", curl_easy_strerror(time_result));

        curl_easy_cleanup(curl);
        return 1;
    }

    *latency_ms = total_time * 1000.0;

    curl_easy_cleanup(curl);
    return 0;
}

Server *find_best_server(Server *servers, size_t count, const char *country)
{
    Server *best_server = NULL;
    double best_latency = 100000000.0;

    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(servers[i].country, country) != 0)
        {
            continue;
        }

        double latency = 0.0;

        if (measure_latency(&servers[i], &latency) != 0)
        {
            continue;
        }

        if (latency < best_latency)
        {
            best_latency = latency;
            best_server = &servers[i];
            printf("Testing %s - %s: %.2f ms\n", servers[i].city, servers[i].provider, latency);
        }
    }

    return best_server;
}