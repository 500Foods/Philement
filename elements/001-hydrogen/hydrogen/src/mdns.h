#ifndef MDNS_H
#define MDNS_H

// System Libraries
#include <stdint.h>
#include <pthread.h>

// Project Libraries
#include "network.h"

#define MDNS_PORT 5353
#define MDNS_GROUP_V4 "224.0.0.251"
#define MDNS_GROUP_V6 "ff02::fb"

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
#define MDNS_MAX_PACKET_SIZE 1500

typedef struct {
    char *name;
    char *type;
    int port;
    char **txt_records;  
    int num_txt_records;
} mdns_service_t;

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
    mdns_service_t *services;
    int num_services;
} mdns_t;

typedef struct {
    mdns_t *mdns;
    int port;
    const network_info_t *net_info;
    volatile int *running;
} mdns_thread_arg_t;


// Functions
mdns_t *mdns_init(const char *app_name, 
		  const char *id, 
		  const char *friendly_name, 
		  const char *model, 
		  const char *manufacturer, 
		  const char *sw_version, 
		  const char *hw_version, 
		  const char *config_url,
		  mdns_service_t *services,
		  int num_services);
void mdns_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname, const mdns_t *mdns, uint32_t ttl, const network_info_t *net_info);
void mdns_send_announcement(mdns_t *mdns, int port, const network_info_t *net_info);
void mdns_shutdown(mdns_t *mdns);
void *mdns_announce_loop(void *arg);
void *mdns_responder_loop(void *arg);

#endif // MDNS_H
