
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include "upload.h"
#include "geolocation.h"
#include "server.h"
#include "download.h"

typedef enum
{
    MODE_NONE = 0,
    MODE_AUTO,
    MODE_LOCATION,
    MODE_SELECT_SERVER,
    MODE_DOWNLOAD,
    MODE_UPLOAD
} Mode;


static void print_usage(const char *prog_name)
{
    printf("Naudojimas: %s [veiksmas] [parametrai]\n\n", prog_name);
    printf("Veiksmai (pasirenkamas tik vienas):\n");
    printf("  -a            Atlikti visą testą automatizuotai (numatytoji veiksena)\n");
    printf("                seka: vietovė -> serverių sąrašas -> geriausias serveris ->\n");
    printf("                      download -> upload -> rezultatai\n");
    printf("  -l            Nustatyti tik vartotojo vietovę (šalį)\n");
    printf("  -s            Rasti / pasirinkti geriausią serverį pagal vietovę\n");
    printf("  -d            Atlikti tik duomenų parsisiuntimo testą\n");
    printf("  -u            Atlikti tik duomenų išsiuntimo testą\n");
    printf("  -h            Parodyti šią pagalbą\n\n");
    printf("Parametrai (nebūtini):\n");
    printf("  -H HOST       Naudoti konkretų serverį su -d arba -u\n");
    printf("                (jei nenurodyta, serveris parenkamas automatiškai:\n");
    printf("                 vietovė -> serverių sąrašas -> geriausias serveris)\n");
    printf("  -c COUNTRY    Naudoti konkrečią valstybę su -s, -d arba -u\n");
    printf("                (jei nenurodyta, vietovė nustatoma automatiškai)\n");
}



static int build_manual_server(
    const char *host,
    Server *server
)
{
    memset(server, 0, sizeof(*server));

    int written = snprintf(
        server->host,
        sizeof(server->host),
        "%s",
        host
    );

    if (written < 0 || (size_t)written >= sizeof(server->host))
    {
        printf("Serverio adresas per ilgas\n");
        return -1;
    }

    return 0;
}



static int resolve_server(
    const char *host_arg,
    const char *country_arg,
    Server *out_server
)
{
    if (host_arg && host_arg[0] != '\0')
    {
        return build_manual_server(host_arg, out_server);
    }

    char *detected_country = NULL;
    const char *country = country_arg;

    if (!country || country[0] == '\0')
    {
        printf("Detecting location...\n");

        detected_country = get_country();

        if (!detected_country)
        {
            printf("Location detection failed\n");
            return -1;
        }

        printf("Country: %s\n", detected_country);

        country = detected_country;
    }

    printf("\nLoading servers...\n");

    if (load_servers("data/speedtest_server_list.json") != 0)
    {
        printf("Server loading failed\n");
        free(detected_country);
        return -1;
    }

    Server best = find_best_server(country);

    free(detected_country);

    if (best.host[0] == '\0')
    {
        printf("No suitable server found\n");
        return -1;
    }

    *out_server = best;

    return 0;
}


static int run_location_only(void)
{
    printf("Detecting location...\n");

    char *country = get_country();

    if (!country)
    {
        printf("Location detection failed\n");
        return 1;
    }

    printf("\n");
    printf("Vartotojo vietovė: %s\n", country);

    free(country);

    return 0;
}


static int run_select_server_only(const char *country_arg)
{
    char *detected_country = NULL;
    const char *country = country_arg;

    if (!country || country[0] == '\0')
    {
        printf("Detecting location...\n");

        detected_country = get_country();

        if (!detected_country)
        {
            printf("Location detection failed\n");
            return 1;
        }

        printf("Country: %s\n", detected_country);

        country = detected_country;
    }

    printf("\nLoading servers...\n");

    if (load_servers("data/speedtest_server_list.json") != 0)
    {
        printf("Server loading failed\n");
        free(detected_country);
        return 1;
    }

    Server best = find_best_server(country);

    free(detected_country);

    if (best.host[0] == '\0')
    {
        printf("No suitable server found\n");
        return 1;
    }

    return 0;
}


static int run_download_only(
    const char *host_arg,
    const char *country_arg
)
{
    Server server;

    if (resolve_server(host_arg, country_arg, &server) != 0)
    {
        return 1;
    }

    double speed = download_test(&server);

    printf("\nServeris: %s\n", server.host);

    if (speed <= 0.0)
    {
        printf("Download test failed\n");
        return 1;
    }

    return 0;
}


static int run_upload_only(
    const char *host_arg,
    const char *country_arg
)
{
    Server server;

    if (resolve_server(host_arg, country_arg, &server) != 0)
    {
        return 1;
    }

    double speed = upload_test(&server);

    printf("\nServeris: %s\n", server.host);

    if (speed <= 0.0)
    {
        printf("Upload test failed\n");
        return 1;
    }

    return 0;
}


static int run_automated_test(void)
{
    printf("Detecting location...\n");

    char *country = get_country();

    if (!country)
    {
        printf("Location detection failed\n");
        return 1;
    }

    printf("Country: %s\n", country);

    printf("\nLoading servers...\n");

    if (load_servers("data/speedtest_server_list.json") != 0)
    {
        printf("Server loading failed\n");
        free(country);
        return 1;
    }

    Server server = find_best_server(country);

    if (server.host[0] == '\0')
    {
        printf("No suitable server found\n");
        free(country);
        return 1;
    }

    printf("\nSelected server:\n");
    printf("City: %s\n", server.city);
    printf("Provider: %s\n", server.provider);
    printf("Host: %s\n", server.host);

    double latency = 0.0;
    double jitter = 0.0;

    printf("\nTesting latency...\n");

    if (measure_latency(&server, &latency, &jitter) != 0)
    {
        printf("Latency test failed\n");
        free(country);
        return 1;
    }

    double download_speed = download_test(&server);

    if (download_speed <= 0.0)
    {
        printf("Download test failed\n");
        free(country);
        return 1;
    }

    double upload_speed = upload_test(&server);

    if (upload_speed <= 0.0)
    {
        printf("Upload test failed\n");
        free(country);
        return 1;
    }

    printf("\n");
    printf("================================\n");
    printf("        SPEEDTEST RESULTS\n");
    printf("================================\n");

    printf("Location : %s\n", country);
    printf("Server   : %s\n", server.provider);
    printf("City     : %s\n", server.city);
    printf("Host     : %s\n", server.host);

    printf("--------------------------------\n");

    printf("Latency  : %.2f ms\n", latency);
    printf("Jitter   : %.2f ms\n", jitter);
    printf("Download : %.2f Mbps\n", download_speed);
    printf("Upload   : %.2f Mbps\n", upload_speed);

    printf("================================\n");

    free(country);

    return 0;
}


int main(int argc, char *argv[])
{
    Mode mode = MODE_NONE;

    const char *host_arg = NULL;
    const char *country_arg = NULL;

    int opt;

    opterr = 0;

    while ((opt = getopt(argc, argv, "alsduhc:H:")) != -1)
    {
        switch (opt)
        {
            case 'a':
            case 'l':
            case 's':
            case 'd':
            case 'u':
            {
                Mode requested;

                switch (opt)
                {
                    case 'a': requested = MODE_AUTO; break;
                    case 'l': requested = MODE_LOCATION; break;
                    case 's': requested = MODE_SELECT_SERVER; break;
                    case 'd': requested = MODE_DOWNLOAD; break;
                    default:  requested = MODE_UPLOAD; break;
                }

                if (mode != MODE_NONE && mode != requested)
                {
                    printf("Galima pasirinkti tik vieną veiksmą vienu metu\n\n");
                    print_usage(argv[0]);
                    return 1;
                }

                mode = requested;

                break;
            }

            case 'H':
                host_arg = optarg;
                break;

            case 'c':
                country_arg = optarg;
                break;

            case 'h':
                print_usage(argv[0]);
                return 0;

            case '?':
            default:
                printf("Nežinomas arba neteisingai naudojamas parametras\n\n");
                print_usage(argv[0]);
                return 1;
        }
    }

    if (mode == MODE_NONE)
    {
        mode = MODE_AUTO;
    }

    if (host_arg && mode != MODE_DOWNLOAD && mode != MODE_UPLOAD)
    {
        printf("-H galima naudoti tik su -d arba -u\n\n");
        print_usage(argv[0]);
        return 1;
    }

    if (country_arg &&
        mode != MODE_SELECT_SERVER &&
        mode != MODE_DOWNLOAD &&
        mode != MODE_UPLOAD)
    {
        printf("-c galima naudoti tik su -s, -d arba -u\n\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("================================\n");
    printf("        CORE SPEEDTEST\n");
    printf("================================\n\n");

    

    
    CURLcode global_result = curl_global_init(CURL_GLOBAL_DEFAULT);

    if (global_result != CURLE_OK)
    {
        printf(
            "curl_global_init failed: %s\n",
            curl_easy_strerror(global_result)
        );

        return 1;
    }

    int result;

    switch (mode)
    {
        case MODE_LOCATION:
            result = run_location_only();
            break;

        case MODE_SELECT_SERVER:
            result = run_select_server_only(country_arg);
            break;

        case MODE_DOWNLOAD:
            result = run_download_only(host_arg, country_arg);
            break;

        case MODE_UPLOAD:
            result = run_upload_only(host_arg, country_arg);
            break;

        case MODE_AUTO:
        default:
            result = run_automated_test();
            break;
    }

    curl_global_cleanup();

    return result;
}
