#ifndef SERVER_H
#define SERVER_H

typedef struct
{
    char country[64];
    char city[64];
    char provider[128];
    char host[256];
    int id;
} Server;

int load_servers(const char *filename);

int get_server_count(void);

Server get_server(int index);

Server find_server_by_country(const char *country);

Server find_best_server(const char *country);

/*
    0  - pavyko
  -1  - klaida     
 */
int measure_latency(
    const Server *server,
    double *latency_ms,
    double *jitter_ms
);

#endif

