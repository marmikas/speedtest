#include "location.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <curl/curl.h>

typedef struct
{
    char *data;
    size_t size;
} ResponseBuffer;

static size_t response_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    ResponseBuffer *response = userdata;

    size_t total = size * nmemb;

    char *new_data = realloc(response->data, response->size + total + 1);

    if (new_data == NULL)
    {
        return 0;
    }

    response->data = new_data;

    memcpy(response->data + response->size, ptr, total);

    response->size += total;
    response->data[response->size] = '\0';

    return total;
}

int get_location(char *country, size_t country_size)
{
    if (country == NULL || country_size == 0)
    {
        fprintf(stderr, "Invalid get_location arguments\n");
        return 1;
    }

    ResponseBuffer response = {0};

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "http://ipwho.is/");

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_callback);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest-c/1.0");

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        fprintf(stderr, "Location request failed: %s\n", curl_easy_strerror(result));

        free(response.data);
        curl_easy_cleanup(curl);

        return 1;
    }

    long response_code = 0;

    CURLcode code_result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (code_result != CURLE_OK)
    {
        fprintf(stderr, "Failed to get HTTP response code: %s\n", curl_easy_strerror(code_result));

        free(response.data);
        curl_easy_cleanup(curl);
        return 1;
    }

    if (response_code != 200)
    {
        fprintf(stderr, "Location API returned HTTP %ld\n", response_code);

        free(response.data);
        curl_easy_cleanup(curl);
        return 1;
    }

    if (response.data == NULL || response.size == 0)
    {
        fprintf(stderr, "Location API returned no data\n");

        free(response.data);
        curl_easy_cleanup(curl);
        return 1;
    }

    cJSON *root = cJSON_Parse(response.data);

    if (root == NULL)
    {
        fprintf(stderr, "Failed to parse location JSON\n");

        free(response.data);
        curl_easy_cleanup(curl);

        return 1;
    }

    cJSON *success = cJSON_GetObjectItem(root, "success");

    cJSON *country_json = cJSON_GetObjectItem(root, "country");

    if (!cJSON_IsBool(success) || !cJSON_IsTrue(success) || !cJSON_IsString(country_json))
    {

        fprintf(stderr, "Invalid location response\n");

        cJSON_Delete(root);
        free(response.data);
        curl_easy_cleanup(curl);

        return 1;
    }

    if (strlen(country_json->valuestring) >= country_size)
    {
        fprintf(stderr, "Country name too long\n");

        cJSON_Delete(root);
        free(response.data);
        curl_easy_cleanup(curl);

        return 1;
    }

    strcpy(country, country_json->valuestring);

    cJSON_Delete(root);
    free(response.data);
    curl_easy_cleanup(curl);

    return 0;
}