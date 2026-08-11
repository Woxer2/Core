
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "server.h"

#define MAX_SERVERS 6000

#define LATENCY_TESTS 5
#define LATENCY_CONNECT_TIMEOUT_MS 2000L
#define LATENCY_TIMEOUT_MS 3000L

static Server servers[MAX_SERVERS];
static int server_count = 0;




static size_t discard_callback(
    void *contents,
    size_t size,
    size_t nmemb,
    void *userp
)
{
    (void)contents;
    (void)userp;

    return size * nmemb;
}




static double perform_latency_request(
    CURL *curl,
    const char *host
)
{
    if (!curl || !host)
    {
        return -1.0;
    }

    char url[512];

    int written = snprintf(
        url,
        sizeof(url),
        "http://%s/",
        host
    );

    if (
        written < 0 ||
        (size_t)written >= sizeof(url)
    )
    {
        return -1.0;
    }

    curl_easy_reset(curl);

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url
    );

  
    curl_easy_setopt(
        curl,
        CURLOPT_NOBODY,
        1L
    );

    

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_CONNECTTIMEOUT_MS,
        LATENCY_CONNECT_TIMEOUT_MS
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT_MS,
        LATENCY_TIMEOUT_MS
    );

    curl_easy_setopt(
        curl,
        CURLOPT_NOSIGNAL,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "Core-Speedtest/1.0"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        discard_callback
    );

    CURLcode result =
        curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        return -1.0;
    }

    long response_code = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &response_code
    );

    if (
        response_code < 200 ||
        response_code >= 400
    )
    {
        return -1.0;
    }

    double total_time = 0.0;

    curl_easy_getinfo(
        curl,
        CURLINFO_TOTAL_TIME,
        &total_time
    );

    return total_time * 1000.0;
}




static double calculate_average(
    const double *values,
    int count
)
{
    if (!values || count <= 0)
    {
        return 0.0;
    }

    double sum = 0.0;

    for (int i = 0; i < count; i++)
    {
        sum += values[i];
    }

    return sum / (double)count;
}




static double calculate_jitter(
    const double *values,
    int count
)
{
    if (!values || count < 2)
    {
        return 0.0;
    }

    double difference_sum = 0.0;

    for (int i = 1; i < count; i++)
    {
        difference_sum += fabs(
            values[i] - values[i - 1]
        );
    }

    return difference_sum / (double)(count - 1);
}




int load_servers(const char *filename)
{
    if (!filename)
    {
        return -1;
    }

    FILE *file = fopen(
        filename,
        "r"
    );

    if (!file)
    {
        printf(
            "Cannot open file: %s\n",
            filename
        );

        return -1;
    }

    if (
        fseek(
            file,
            0,
            SEEK_END
        ) != 0
    )
    {
        fclose(file);
        return -1;
    }

    long size = ftell(file);

    if (size < 0)
    {
        fclose(file);
        return -1;
    }

    rewind(file);

    char *buffer = malloc(
        (size_t)size + 1
    );

    if (!buffer)
    {
        fclose(file);
        return -1;
    }

    size_t read_size = fread(
        buffer,
        1,
        (size_t)size,
        file
    );

    buffer[read_size] = '\0';

    fclose(file);

    cJSON *json =
        cJSON_Parse(buffer);

    free(buffer);

    if (!json)
    {
        printf(
            "JSON parsing failed\n"
        );

        return -1;
    }

    if (!cJSON_IsArray(json))
    {
        printf(
            "Server JSON is not an array\n"
        );

        cJSON_Delete(json);

        return -1;
    }

    int count =
        cJSON_GetArraySize(json);

    server_count = 0;

    for (
        int i = 0;
        i < count &&
        server_count < MAX_SERVERS;
        i++
    )
    {
        cJSON *item =
            cJSON_GetArrayItem(
                json,
                i
            );

        if (!cJSON_IsObject(item))
        {
            continue;
        }

        cJSON *country =
            cJSON_GetObjectItem(
                item,
                "country"
            );

        cJSON *city =
            cJSON_GetObjectItem(
                item,
                "city"
            );

        cJSON *provider =
            cJSON_GetObjectItem(
                item,
                "provider"
            );

        cJSON *host =
            cJSON_GetObjectItem(
                item,
                "host"
            );

        cJSON *id =
            cJSON_GetObjectItem(
                item,
                "id"
            );

        if (
            !cJSON_IsString(country) ||
            !cJSON_IsString(city) ||
            !cJSON_IsString(provider) ||
            !cJSON_IsString(host) ||
            !cJSON_IsNumber(id)
        )
        {
            continue;
        }

        snprintf(
            servers[server_count].country,
            sizeof(
                servers[server_count].country
            ),
            "%s",
            country->valuestring
        );

        snprintf(
            servers[server_count].city,
            sizeof(
                servers[server_count].city
            ),
            "%s",
            city->valuestring
        );

        snprintf(
            servers[server_count].provider,
            sizeof(
                servers[server_count].provider
            ),
            "%s",
            provider->valuestring
        );

        snprintf(
            servers[server_count].host,
            sizeof(
                servers[server_count].host
            ),
            "%s",
            host->valuestring
        );

        servers[server_count].id =
            id->valueint;

        server_count++;
    }

    cJSON_Delete(json);

    printf(
        "Servers loaded: %d\n",
        server_count
    );

    return 0;
}




int get_server_count(void)
{
    return server_count;
}




Server get_server(int index)
{
    Server empty = {0};

    if (
        index < 0 ||
        index >= server_count
    )
    {
        return empty;
    }

    return servers[index];
}




Server find_server_by_country(
    const char *country
)
{
    Server empty = {0};

    if (!country)
    {
        return empty;
    }

    for (
        int i = 0;
        i < server_count;
        i++
    )
    {
        if (
            strcmp(
                servers[i].country,
                country
            ) == 0
        )
        {
            return servers[i];
        }
    }

    return empty;
}




Server find_best_server(
    const char *country
)
{
    Server best = {0};

    double best_latency = 999999.0;

    if (!country)
    {
        return best;
    }

    CURL *curl =
        curl_easy_init();

    if (!curl)
    {
        printf(
            "Failed to initialize libcurl\n"
        );

        return best;
    }

    printf(
        "Searching for best server in %s...\n",
        country
    );

    for (
        int i = 0;
        i < server_count;
        i++
    )
    {
        if (
            strcmp(
                servers[i].country,
                country
            ) != 0
        )
        {
            continue;
        }

        double measurements[
            LATENCY_TESTS
        ];

        int successful_tests = 0;

        for (
            int j = 0;
            j < LATENCY_TESTS;
            j++
        )
        {
            double latency =
                perform_latency_request(
                    curl,
                    servers[i].host
                );

            if (latency < 0.0)
            {
                continue;
            }

            measurements[
                successful_tests
            ] = latency;

            successful_tests++;
        }

        if (successful_tests == 0)
        {
            printf(
                "%-35s FAILED\n",
                servers[i].host
            );

            continue;
        }

        double average =
            calculate_average(
                measurements,
                successful_tests
            );

        double jitter =
            calculate_jitter(
                measurements,
                successful_tests
            );

        printf(
            "%-35s %.2f ms "
            "(jitter %.2f ms)\n",
            servers[i].host,
            average,
            jitter
        );

        if (average < best_latency)
        {
            best_latency = average;
            best = servers[i];
        }
    }

    curl_easy_cleanup(curl);

    if (best.host[0] == '\0')
    {
        printf(
            "No working server found "
            "for country: %s\n",
            country
        );

        return best;
    }

    printf(
        "\nBest server found:\n"
    );

    printf(
        "City: %s\n",
        best.city
    );

    printf(
        "Provider: %s\n",
        best.provider
    );

    printf(
        "Host: %s\n",
        best.host
    );

    printf(
        "Latency: %.2f ms\n",
        best_latency
    );

    return best;
}



int measure_latency(
    const Server *server,
    double *latency,
    double *jitter
)
{
    if (
        !server ||
        !latency ||
        !jitter
    )
    {
        return -1;
    }

    CURL *curl =
        curl_easy_init();

    if (!curl)
    {
        return -1;
    }

    double measurements[
        LATENCY_TESTS
    ];

    int successful_tests = 0;

    printf("\n");

    for (
        int i = 0;
        i < LATENCY_TESTS;
        i++
    )
    {
        double value =
            perform_latency_request(
                curl,
                server->host
            );

        if (value < 0.0)
        {
            printf(
                "  %d: FAILED\n",
                i + 1
            );

            continue;
        }

        measurements[
            successful_tests
        ] = value;

        successful_tests++;

        printf(
            "  %d: %.2f ms\n",
            i + 1,
            value
        );
    }

    curl_easy_cleanup(curl);

    if (successful_tests == 0)
    {
        return -1;
    }

    *latency =
        calculate_average(
            measurements,
            successful_tests
        );

    *jitter =
        calculate_jitter(
            measurements,
            successful_tests
        );

    return 0;
}

