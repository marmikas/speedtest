#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include "speedtest.h"

#define UPLOAD_BUFFER_SIZE (1024 * 1024)
#define DOWNLOAD_TEST_DURATION 15.0
#define SPEED_TEST_DURATION DOWNLOAD_TEST_DURATION

typedef struct
{
    size_t bytes;
    struct timespec start;
} DownloadData;

typedef struct
{
    size_t bytes_sent;
    struct timespec start;

    const unsigned char *buffer;
    size_t buffer_size;
    size_t position;
} UploadData;

static double elapsed_seconds(const struct timespec *start)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start->tv_sec) + (double)(now.tv_nsec - start->tv_nsec) / 1000000000.0;
}

static size_t read_callback(char *ptr, size_t size, size_t num_of_elements, void *userdata)
{

    size_t capacity = size * num_of_elements;

    UploadData *data = userdata;

    size_t remaining = data->buffer_size - data->position;

    size_t to_copy = remaining < capacity ? remaining : capacity;

    memcpy(ptr, data->buffer + data->position, to_copy);

    data->position += to_copy;
    data->bytes_sent += to_copy;

    return to_copy;
}

static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;

    DownloadData *data = clientp;

    if (elapsed_seconds(&data->start) >= DOWNLOAD_TEST_DURATION)
    {
        return 1;
    }

    return 0;
}

static int upload_progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;

    UploadData *data = clientp;

    if (elapsed_seconds(&data->start) >= SPEED_TEST_DURATION)
    {
        return 1;
    }

    return 0;
}

static size_t write_callback(void *ptr, size_t size, size_t num_of_elements, void *userdata)
{

    (void)ptr; // dicarding downloaded content

    DownloadData *data = userdata;

    size_t total_bytes = size * num_of_elements;

    data->bytes += total_bytes;

    return total_bytes;
}

static size_t discard_callback(void *ptr, size_t size, size_t num_of_elements, void *userdata)
{

    (void)ptr; // dicarding downloaded content

    (void)userdata;

    return size * num_of_elements;
}

int download_test(const Server *server, double *speed_mbps)
{

    double elapsed_time = 0.0;

    if (server == NULL || speed_mbps == NULL)
    {
        fprintf(stderr, "Invalid download_test arguments\n");
        return 1;
    }

    DownloadData data = {0};

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    char url[512];

    int written = snprintf(url, sizeof(url), "http://%s/speedtest/random4000x4000.jpg", server->host);

    if (written < 0 || (size_t)written >= sizeof(url))
    {
        fprintf(stderr, "Server URL too long\n");
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &data);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest-c/1.0");

    // for debugging
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    clock_gettime(CLOCK_MONOTONIC, &data.start);

    while (elapsed_seconds(&data.start) < DOWNLOAD_TEST_DURATION)
    {

        double elapsed = elapsed_seconds(&data.start);
        double remaining = DOWNLOAD_TEST_DURATION - elapsed;

        if (remaining <= 0.0)
        {
            break;
        }

        long remaining_ms = (long)(remaining * 1000.0);

        if (remaining_ms < 1)
        {
            remaining_ms = 1;
        }

        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, remaining_ms);

        CURLcode result = curl_easy_perform(curl);

        long response_code = 0;

        CURLcode code_result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (code_result != CURLE_OK)
        {
            fprintf(stderr, "Failed to get HTTP response code: %s\n", curl_easy_strerror(code_result));

            curl_easy_cleanup(curl);
            return 1;
        }

        if (response_code != 200)
        {
            fprintf(stderr, "Server returned HTTP %ld\n", response_code);

            curl_easy_cleanup(curl);
            return 1;
        }

        if (result == CURLE_ABORTED_BY_CALLBACK || result == CURLE_OPERATION_TIMEDOUT)
        {
            break;
        }

        if (result != CURLE_OK)
        {
            fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(result));

            curl_easy_cleanup(curl);
            return 1;
        }
    }

    elapsed_time = elapsed_seconds(&data.start);

    if (data.bytes == 0 || elapsed_time <= 0.0)
    {
        fprintf(stderr, "No download data received\n");

        curl_easy_cleanup(curl);
        return 1;
    }

    *speed_mbps = ((double)data.bytes * 8.0) / (1000000.0 * elapsed_time);

    printf("Downloaded bytes: %zu\n", data.bytes);
    printf("Elapsed time: %.6f seconds\n", elapsed_time);
    printf("Download speed: %.6f Mbps\n", *speed_mbps);

    curl_easy_cleanup(curl);

    return 0;
}

int upload_test(const Server *server, double *speed_mbps)
{

    double elapsed_time = 0.0;

    if (server == NULL || speed_mbps == NULL)
    {
        fprintf(stderr,
                "Invalid upload_test arguments\n");
        return 1;
    }

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    unsigned char *buffer = malloc(UPLOAD_BUFFER_SIZE);

    if (buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate upload buffer\n");
        curl_easy_cleanup(curl);
        return 1;
    }

    memset(buffer, 'A', UPLOAD_BUFFER_SIZE);

    UploadData data = {0};

    data.buffer = buffer;
    data.buffer_size = UPLOAD_BUFFER_SIZE;
    data.position = 0;
    data.bytes_sent = 0;

    char url[512];

    int written = snprintf(url, sizeof(url), "http://%s/speedtest/upload.php", server->host);

    if (written < 0 || (size_t)written >= sizeof(url))
    {
        fprintf(stderr, "Server URL too long\n");

        free(buffer);
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

    curl_easy_setopt(curl, CURLOPT_READDATA, &data);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)UPLOAD_BUFFER_SIZE);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_callback);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, upload_progress_callback);

    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &data);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest-c/1.0");

    clock_gettime(CLOCK_MONOTONIC, &data.start);

    while (elapsed_seconds(&data.start) < SPEED_TEST_DURATION)
    {
        data.position = 0;

        double elapsed = elapsed_seconds(&data.start);

        double remaining = SPEED_TEST_DURATION - elapsed;

        if (remaining <= 0.0)
        {
            break;
        }

        long remaining_ms = (long)(remaining * 1000.0);

        if (remaining_ms < 1)
        {
            remaining_ms = 1;
        }

        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, remaining_ms);

        CURLcode result = curl_easy_perform(curl);

        if (result == CURLE_ABORTED_BY_CALLBACK || result == CURLE_OPERATION_TIMEDOUT)
        {
            break;
        }

        if (result != CURLE_OK)
        {
            fprintf(stderr, "Upload failed: %s\n", curl_easy_strerror(result));

            free(buffer);
            curl_easy_cleanup(curl);
            return 1;
        }

        long response_code = 0;

        CURLcode code_result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (code_result != CURLE_OK)
        {
            fprintf(stderr, "Failed to get HTTP response code: %s\n", curl_easy_strerror(code_result));

            free(buffer);
            curl_easy_cleanup(curl);
            return 1;
        }

        if (response_code != 200)
        {
            fprintf(stderr, "Server returned HTTP %ld\n", response_code);

            free(buffer);
            curl_easy_cleanup(curl);
            return 1;
        }
    }

    elapsed_time = elapsed_seconds(&data.start);

    if (data.bytes_sent == 0 || elapsed_time <= 0.0)
    {

        fprintf(stderr, "No upload data\n");

        free(buffer);
        curl_easy_cleanup(curl);
        return 1;
    }

    *speed_mbps = ((double)data.bytes_sent * 8.0) / (1000000.0 * elapsed_time);

    printf("Bytes sent: %zu\n", data.bytes_sent);
    printf("Elapsed time: %f\n", elapsed_time);
    printf("Upload speed: %.6f Mbps\n", *speed_mbps);

    free(buffer);
    curl_easy_cleanup(curl);

    return 0;
}
