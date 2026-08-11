#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "download.h"
#include "server.h"

#define DOWNLOAD_CONNECTIONS 4
#define DOWNLOAD_SIZE 10000000ULL

typedef struct
{
    CURL *easy;
    curl_off_t downloaded;
    int finished;
    int success;
} DownloadTransfer;



static size_t write_callback(
    void *contents,
    size_t size,
    size_t nmemb,
    void *userp
)
{
    (void)contents;

    DownloadTransfer *transfer =
        (DownloadTransfer *)userp;

    size_t total = size * nmemb;

    transfer->downloaded += (curl_off_t)total;

    return total;
}



static int build_download_url(
    const Server *server,
    char *url,
    size_t url_size
)
{
    if (!server || !url || url_size == 0)
    {
        return -1;
    }

    int written = snprintf(
        url,
        url_size,
        "http://%s/download?size=%llu",
        server->host,
        (unsigned long long)DOWNLOAD_SIZE
    );

    if (written < 0 ||
        (size_t)written >= url_size)
    {
        return -1;
    }

    return 0;
}



double download_test(const Server *server)
{
    if (!server || server->host[0] == '\0')
    {
        printf("Invalid download server\n");
        return 0.0;
    }

    CURLM *multi = NULL;

    DownloadTransfer transfers[DOWNLOAD_CONNECTIONS];

    memset(
        transfers,
        0,
        sizeof(transfers)
    );

    char url[512];

    if (build_download_url(
            server,
            url,
            sizeof(url)
        ) != 0)
    {
        printf("Failed to build download URL\n");
        return 0.0;
    }

    printf("\nStarting parallel download test...\n");
    printf(
        "Download URL: %s\n",
        url
    );


    multi = curl_multi_init();

    if (!multi)
    {
        printf("Failed to initialize curl multi\n");
        return 0.0;
    }

    int created = 0;

    
    for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
    {
        CURL *easy = curl_easy_init();

        if (!easy)
        {
            printf(
                "Failed to initialize download connection %d\n",
                i + 1
            );

            break;
        }

        transfers[i].easy = easy;
        transfers[i].downloaded = 0;
        transfers[i].finished = 0;

        curl_easy_setopt(
            easy,
            CURLOPT_URL,
            url
        );

       

        curl_easy_setopt(
            easy,
            CURLOPT_FOLLOWLOCATION,
            1L
        );

      

        curl_easy_setopt(
            easy,
            CURLOPT_CONNECTTIMEOUT_MS,
            3000L
        );

        curl_easy_setopt(
            easy,
            CURLOPT_TIMEOUT_MS,
            15000L
        );

       

        curl_easy_setopt(
            easy,
            CURLOPT_NOSIGNAL,
            1L
        );

        

        curl_easy_setopt(
            easy,
            CURLOPT_WRITEFUNCTION,
            write_callback
        );

        curl_easy_setopt(
            easy,
            CURLOPT_WRITEDATA,
            &transfers[i]
        );

        curl_easy_setopt(
            easy,
            CURLOPT_USERAGENT,
            "Core-Speedtest/1.0"
        );

       

        curl_easy_setopt(
            easy,
            CURLOPT_TCP_KEEPALIVE,
            1L
        );

        CURLMcode add_result =
            curl_multi_add_handle(
                multi,
                easy
            );

        if (add_result != CURLM_OK)
        {
            printf(
                "Failed to add download connection %d: %s\n",
                i + 1,
                curl_multi_strerror(add_result)
            );

            curl_easy_cleanup(easy);
            transfers[i].easy = NULL;

            continue;
        }

        created++;
    }

    if (created == 0)
    {
        printf("No download connections created\n");

        curl_multi_cleanup(multi);

        return 0.0;
    }

    printf(
        "Connections: %d\n",
        created
    );

    printf(
        "Starting %d parallel download streams...\n",
        created
    );

   

    int still_running = 0;

    CURLMcode multi_result =
        curl_multi_perform(
            multi,
            &still_running
        );

    if (multi_result != CURLM_OK)
    {
        printf(
            "curl_multi_perform failed: %s\n",
            curl_multi_strerror(multi_result)
        );

        for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
        {
            if (transfers[i].easy)
            {
                curl_multi_remove_handle(
                    multi,
                    transfers[i].easy
                );

                curl_easy_cleanup(
                    transfers[i].easy
                );
            }
        }

        curl_multi_cleanup(multi);

        return 0.0;
    }

    

   

    while (still_running)
    {
        int numfds = 0;

        multi_result =
            curl_multi_poll(
                multi,
                NULL,
                0,
                100,
                &numfds
            );

        if (multi_result != CURLM_OK)
        {
            printf(
                "curl_multi_poll failed: %s\n",
                curl_multi_strerror(multi_result)
            );

            break;
        }

        multi_result =
            curl_multi_perform(
                multi,
                &still_running
            );

        if (multi_result != CURLM_OK)
        {
            printf(
                "curl_multi_perform failed: %s\n",
                curl_multi_strerror(multi_result)
            );

            break;
        }
    }

    

    int successful = 0;

    curl_off_t total_downloaded = 0;

    CURLMsg *message = NULL;

    while ((message = curl_multi_info_read(
                multi,
                &(int){0}
            )) != NULL)
    {
        if (message->msg != CURLMSG_DONE)
        {
            continue;
        }

        CURL *easy = message->easy_handle;

        CURLcode result =
            message->data.result;

        for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
        {
            if (transfers[i].easy != easy)
            {
                continue;
            }

            transfers[i].finished = 1;
           

            long response_code = 0;

            curl_easy_getinfo(
                easy,
                CURLINFO_RESPONSE_CODE,
                &response_code
            );

            if (result == CURLE_OK &&
                response_code >= 200 &&
                response_code < 400)
            {
                successful++;
                transfers[i].success = 1;

                printf(
                    "  Stream %d: OK - %lld bytes - HTTP %ld\n",
                    i + 1,
                    (long long)transfers[i].downloaded,
                    response_code
                );
            }
            else
            {
                printf(
                    "  Stream %d: FAILED - %s - HTTP %ld\n",
                    i + 1,
                    curl_easy_strerror(result),
                    response_code
                );
            }

            break;
        }
    }

    

    while (1)
    {
        int queue_left = 0;

        message =
            curl_multi_info_read(
                multi,
                &queue_left
            );

        if (!message)
        {
            break;
        }

        if (message->msg != CURLMSG_DONE)
        {
            continue;
        }

        CURL *easy = message->easy_handle;

        for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
        {
            if (transfers[i].easy == easy &&
                !transfers[i].finished)
            {
                transfers[i].finished = 1;

                CURLcode result =
                    message->data.result;

                long response_code = 0;

                curl_easy_getinfo(
                    easy,
                    CURLINFO_RESPONSE_CODE,
                    &response_code
                );

                if (result == CURLE_OK &&
                    response_code >= 200 &&
                    response_code < 400)
                {
                    successful++;
                    transfers[i].success = 1;

                    printf(
                        "  Stream %d: OK - %lld bytes - HTTP %ld\n",
                        i + 1,
                        (long long)transfers[i].downloaded,
                        response_code
                    );
                }
                else
                {
                    printf(
                        "  Stream %d: FAILED - %s - HTTP %ld\n",
                        i + 1,
                        curl_easy_strerror(result),
                        response_code
                    );
                }

                break;
            }
        }
    }

   





    total_downloaded = 0;

    for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
    {
        if (!transfers[i].success)
        {
            continue;
        }

        total_downloaded +=
            transfers[i].downloaded;
    }

  




    double elapsed = 0.0;

    for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
    {
        if (!transfers[i].easy)
        {
            continue;
        }

        double total_time = 0.0;

        CURLcode info_result =
            curl_easy_getinfo(
                transfers[i].easy,
                CURLINFO_TOTAL_TIME,
                &total_time
            );

        if (info_result == CURLE_OK &&
            total_time > elapsed)
        {
            elapsed = total_time;
        }
    }

    



    if (elapsed <= 0.0)
    {
        elapsed = 0.001;
    }

    



    double speed_mbps =
        ((double)total_downloaded * 8.0) /
        elapsed /
        1000000.0;

    printf(
        "HTTP successful streams: %d/%d\n",
        successful,
        created
    );

    printf(
        "Total downloaded: %lld bytes\n",
        (long long)total_downloaded
    );

    printf(
        "Download time: %.2f seconds\n",
        elapsed
    );

    printf(
        "Download speed: %.2f Mbps\n",
        speed_mbps
    );

   

    
    for (int i = 0; i < DOWNLOAD_CONNECTIONS; i++)
    {
        if (transfers[i].easy)
        {
            curl_multi_remove_handle(
                multi,
                transfers[i].easy
            );

            curl_easy_cleanup(
                transfers[i].easy
            );

            transfers[i].easy = NULL;
        }
    }

    curl_multi_cleanup(multi);

    if (successful == 0 ||
        total_downloaded == 0)
    {
        printf(
            "Download test failed\n"
        );

        return 0.0;
    }

    return speed_mbps;
}