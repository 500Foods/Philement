#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include "config.h"
#include "mdns.h"
#include "network.h"
#include "utils.h"
#include "websocket_server.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define APP_NAME "nitro"
#define APP_MODEL "nitrogen"
#define APP_MANUFACTURER "Philement"
#define APP_SOFTWARE_VER "soft ver"
#define APP_HARDWARE_VER "hw ver"
#define APP_CONFIG_URL "welcome"

static volatile sig_atomic_t running = 1;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void sigint_handler(int signum) {
    (void)signum;  // Unused parameter
    printf("DEBUG: SIGINT received. Setting running to 0.\n");
    running = 0;
    printf("DEBUG: Broadcasting condition variable.\n");
    pthread_cond_broadcast(&cond);
    printf("DEBUG: SIGINT handler completed.\n");
}

int join_thread_with_timeout(pthread_t thread, const char *thread_name, volatile sig_atomic_t *running) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // 5 second timeout

    int result = 0;
    pthread_mutex_lock(&mutex);
    while (*running && result == 0) {
        result = pthread_cond_timedwait(&cond, &mutex, &ts);
    }
    pthread_mutex_unlock(&mutex);

    if (result == ETIMEDOUT) {
        printf("DEBUG: Timed out waiting for %s thread to exit.\n", thread_name);
    } else if (result == 0) {
        result = pthread_join(thread, NULL);
        if (result == 0) {
            printf("DEBUG: %s thread joined successfully.\n", thread_name);
        } else {
            printf("DEBUG: Error joining %s thread: %s\n", thread_name, strerror(result));
        }
    } else {
        printf("DEBUG: Error waiting for %s thread to exit: %s\n", thread_name, strerror(result));
    }

    return result;
}

void print_usage(char *prog_name) {
    fprintf(stderr, "Usage: %s [options]\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n, --name <name>  Set the friendly name\n");
    fprintf(stderr, "  -p, --port <port>  Set the WebSocket port\n");
}

int main(int argc, char *argv[]) {
    char *cli_friendly_name = NULL;
    int cli_port = 0;

    struct option long_opts[] = {
        {"name", required_argument, NULL, 'n'},
        {"port", required_argument, NULL, 'p'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "n:p:", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'n': cli_friendly_name = optarg; break;
            case 'p': cli_port = atoi(optarg); break;
            default: print_usage(argv[0]); return 1;
        }
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        nitro_log("ERROR", "Failed to set up signal handler");
        return 1;
    }

    nitro_config_t *config = nitro_config_load(APP_NAME);
    if (!config) {
        nitro_log("ERROR", "Failed to load config");
        return 1;
    }

    // Override defaults with CLI args if no config file
    if (access("nitro.json", F_OK) != 0) {
        if (cli_friendly_name) {
            free(config->name);
            config->name = strdup(cli_friendly_name);
        }
        if (cli_port > 0 && cli_port < 65536) {
            config->port = cli_port;
        }
    }
 nitro_network_info_t *net_info = nitro_get_network_info();
    if (!net_info || net_info->primary_index == -1) {
        nitro_log("ERROR", "Failed to get network info");
        nitro_config_free(config);
        return 1;
    }

    nitro_interface_t *primary = &net_info->interfaces[net_info->primary_index];
    nitro_log("INFO", "Primary interface: %s", primary->name);
    nitro_log("INFO", "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
              primary->mac[0], primary->mac[1], primary->mac[2],
              primary->mac[3], primary->mac[4], primary->mac[5]);

    int available_port = nitro_find_available_port(config->port);
    if (available_port == -1) {
        nitro_log("ERROR", "No available ports found");
        nitro_config_free(config);
        nitro_free_network_info(net_info);
        return 1;
    }
    config->port = available_port;


    nitro_mdns_t *mdns = nitro_mdns_init(APP_NAME, config->id, config->name, APP_MODEL, APP_MANUFACTURER, APP_SOFTWARE_VER, APP_HARDWARE_VER, APP_CONFIG_URL);
    if (!mdns) {
        nitro_log("ERROR", "Failed to initialize mDNS");
        nitro_config_free(config);
        nitro_free_network_info(net_info);
        return 1;
    }

      nitro_mdns_thread_arg_t thread_arg = {
        .mdns = mdns,
        .port = config->port,
        .net_info = net_info,
        .running = &running
    };

    pthread_t mdns_thread;
    if (pthread_create(&mdns_thread, NULL, nitro_mdns_announce_loop, &thread_arg) != 0) {
        nitro_log("ERROR", "Failed to create mDNS thread");
        nitro_mdns_shutdown(mdns);
        nitro_config_free(config);
        nitro_free_network_info(net_info);
        return 1;
    }

    pthread_t responder_thread;
    if (pthread_create(&responder_thread, NULL, nitro_mdns_responder_loop, &thread_arg) != 0) {
        nitro_log("ERROR", "Failed to create mDNS responder thread");
        running = 0;
        pthread_join(mdns_thread, NULL);
        nitro_mdns_shutdown(mdns);
        nitro_config_free(config);
        nitro_free_network_info(net_info);
        return 1;
    }

    websocket_thread_arg_t ws_thread_arg = {
        .port = config->port,
        .secret_key = mdns->secret_key,
        .running = &running
    };

    pthread_t websocket_thread;
    if (pthread_create(&websocket_thread, NULL, websocket_server_thread, &ws_thread_arg) != 0) {
        nitro_log("ERROR", "Failed to create WebSocket server thread");
        running = 0;
        pthread_join(mdns_thread, NULL);
        pthread_join(responder_thread, NULL);
        nitro_mdns_shutdown(mdns);
        nitro_config_free(config);
        nitro_free_network_info(net_info);
        return 1;
    }

    nitro_log("INFO", "Started %s on port %d (Ctrl+C to stop)", config->id, config->port);

    pthread_mutex_lock(&mutex);
    while (running) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    printf("DEBUG: Exited main loop. Joining threads.\n");

    // Signal threads to stop
    running = 0;

    printf("DEBUG: Joining mDNS thread.\n");
    join_thread_with_timeout(mdns_thread,"mDNS announcer thread", &running);
    printf("DEBUG: Joining mDNS responder thread.\n");
    join_thread_with_timeout(responder_thread, "mDNS responsder thread", &running);
    printf("DEBUG: Joining websocket thread.\n");
    join_thread_with_timeout(websocket_thread, "websocket main thread", &running);
 
    printf("DEBUG: Cleanup.\n");

    nitro_config_save(APP_NAME, config);
    nitro_config_free(config);
    nitro_free_network_info(net_info);
    nitro_mdns_shutdown(mdns);
    return 0;
}
