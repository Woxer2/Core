#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "upload.h"

#define UPLOAD_SIZE (10 * 1024 * 1024)

typedef struct
{
    char *data;
    size_t size;
    size_t position;
} UploadData;

static size_t read_callback(
    void *ptr,
    size_t size,
    size_t nmemb,
    void *userdata
)
{
    UploadData *upload = (UploadData *)userdata;

    size_t available = upload->size - upload->position;
    size_t requested = size * nmemb;

    size_t to_send =
        available < requested
            ? available
            : requested;

    if (to_send > 0)
    {
        memcpy(
            ptr,
            upload->data + upload->position,
            to_send
        );

        upload->position += to_send;
    }

    return to_send;
}

double upload_test(const Server *server)
{
    if (!server || server->host[0] == '\0')
    {
        printf("Invalid upload server\n");
        return 0.0;
    }

    CURL *curl = curl_easy_init();

    if (!curl)
    {
        printf("Failed to initialize libcurl\n");
        return 0.0;
    }

    char *buffer = malloc(UPLOAD_SIZE);

    if (!buffer)
    {
        printf("Failed to allocate upload buffer\n");
        curl_easy_cleanup(curl);
        return 0.0;
    }

    
     
    memset(buffer, 'A', UPLOAD_SIZE);

    UploadData upload;

    upload.data = buffer;
    upload.size = UPLOAD_SIZE;
    upload.position = 0;

    char url[512];

    snprintf(
        url,
        sizeof(url),
        "http://%s/upload",
        server->host
    );

    printf("Starting upload test...\n");
    printf("Upload URL: %s\n", url);

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url
    );

    
    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    
    curl_easy_setopt(
        curl,
        CURLOPT_READFUNCTION,
        read_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_READDATA,
        &upload
    );

    
    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE_LARGE,
        (curl_off_t)UPLOAD_SIZE
    );

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        2L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "Core-Speedtest/1.0"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_CONNECTTIMEOUT_MS,
        5000L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT_MS,
        15000L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_NOSIGNAL,
        1L
    );

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        printf(
            "Upload failed: %s\n",
            curl_easy_strerror(result)
        );

        free(buffer);
        curl_easy_cleanup(curl);

        return 0.0;
    }

    double total_time = 0.0;

    curl_easy_getinfo(
        curl,
        CURLINFO_TOTAL_TIME,
        &total_time
    );

    long response_code = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &response_code
    );

    printf(
        "HTTP response: %ld\n",
        response_code
    );

    printf(
        "Uploaded: %zu bytes\n",
        upload.position
    );

    printf(
        "Upload time: %.2f seconds\n",
        total_time
    );

    free(buffer);
    curl_easy_cleanup(curl);

    if (response_code < 200 || response_code >= 400)
    {
        printf(
            "Upload failed: unexpected HTTP status %ld\n",
            response_code
        );

        return 0.0;
    }

    if (upload.position == 0)
    {
        printf("Upload server received no data\n");
        return 0.0;
    }

    if (total_time <= 0.0)
    {
        printf("Invalid upload time\n");
        return 0.0;
    }

    double speed_mbps =
        ((double)upload.position * 8.0)
        / total_time
        / 1000000.0;

    printf(
        "Upload speed: %.2f Mbps\n",
        speed_mbps
    );

    return speed_mbps;
}