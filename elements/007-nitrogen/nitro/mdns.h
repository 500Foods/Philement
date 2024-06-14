#ifndef NITRO_MDNS_H
#define NITRO_MDNS_H

#include "network.h"
#include <pthread.h>

#define NITRO_MDNS_PORT 5353
#define NITRO_MDNS_GROUP_V4 "224.0.0.251"
#define NITRO_MDNS_GROUP_V6 "ff02::fb"

#define MDNS_TTL 255

// DNS Type Values
#define MDNS_TYPE_A 1     // IPv4 address
#define MDNS_TYPE_PTR 12  // Domain name pointer
#define MDNS_TYPE_TXT 16  // Text string
#define MDNS_TYPE_AAAA 28 // IPv6 address
#define MDNS_TYPE_SRV 33  // Server location
#define MDNS_TYPE_ANY 255 // Server location

#define MDNS_CLASS_IN 1
#define MDNS_FLAG_RESPONSE 0x8400
#define MDNS_FLAG_AUTHORITATIVE 0x0400
#define NITRO_MDNS_MAX_PACKET_SIZE 512

typedef struct {
    int sockfd_v4;
    int sockfd_v6;
    char *hostname;
    char *service_name;
    char *device_id;
    char *friendly_name;
    char *secret_key;
    char *model;
    char *manufacturer;
    char *sw_version;
    char *hw_version;
    char *config_url;
} nitro_mdns_t;

typedef struct {
    nitro_mdns_t *mdns;
    int port;
    const nitro_network_info_t *net_info;
    volatile int *running;
} nitro_mdns_thread_arg_t;

nitro_mdns_t *nitro_mdns_init(const char *app_name, const char *id, const char *friendly_name, const char *model, const char *manufacturer, const char *sw_version, const char *hw_version, const char *config_url);
void nitro_mdns_send_announcement(nitro_mdns_t *mdns, int port, const nitro_network_info_t *net_info);
void nitro_mdns_shutdown(nitro_mdns_t *mdns);
void *nitro_mdns_announce_loop(void *arg);
void *nitro_mdns_responder_loop(void *arg);

#endif // NITRO_MDNS_H

