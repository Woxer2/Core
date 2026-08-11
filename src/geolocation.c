#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "geolocation.h"

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;

    char **response = (char **)userp;

    size_t current = strlen(*response);

    char *new_response = realloc(*response, current + total + 1);

    if (!new_response)
    {
       



        return 0;
    }

    *response = new_response;

    memcpy(*response + current, contents, total);
    (*response)[current + total] = '\0';

    return total;
}


char *get_country(void)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        return NULL;
    
    char *response = calloc(1, 1);


    curl_easy_setopt(curl, CURLOPT_URL, "https://ipwho.is/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);


    CURLcode result = curl_easy_perform(curl);

curl_easy_cleanup(curl);


if (result != CURLE_OK)
{
    printf("Curl error: %s\n", curl_easy_strerror(result));
    free(response);
    return NULL;
}

// printf("API response:\n%s\n", response);

if (strstr(response, "\"error\": true"))
{
    // printf("Geolocation API error\n");
    free(response);
    return NULL;
}



  cJSON *json = cJSON_Parse(response);

free(response);

if (!json)
{
    printf("JSON parsing failed\n");
    return NULL;
}


cJSON *success = cJSON_GetObjectItem(json, "success");

if (!success || !cJSON_IsTrue(success))
{
    printf("Geolocation API returned error\n");
    cJSON_Delete(json);
    return NULL;
}


cJSON *country = cJSON_GetObjectItem(json, "country");


if (!country)
{
    printf("Country not found\n");
    cJSON_Delete(json);
    return NULL;
}


char *result_country = malloc(
    strlen(country->valuestring) + 1
);


if (!result_country)
{
    cJSON_Delete(json);
    return NULL;
}


strcpy(result_country, country->valuestring);


cJSON_Delete(json);


return result_country;

}
