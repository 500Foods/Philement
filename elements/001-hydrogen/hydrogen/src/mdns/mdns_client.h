/*
 * mDNS client browse/resolve cache, registry snapshot, TCP health.
 */

#ifndef MDNS_CLIENT_H
#define MDNS_CLIENT_H

#include <src/config/config_mdns_client.h>
#include <src/mdns/mdns_wire.h>
#include <jansson.h>
#include <net/if.h>
#include <pthread.h>
#include <stdint.h>

#define MDNS_CLIENT_MAX_ENDPOINTS 8
#define MDNS_CLIENT_TXT_GRACE_MS 3000
#define MDNS_CLIENT_PATH_MAX 128
#define MDNS_CLIENT_INFO_MAX 16

typedef struct {
    int family;
    uint8_t addr[16];
    uint8_t addrlen;
    unsigned ifindex;
} mdns_client_endpoint_t;

typedef struct {
    char instance[MDNS_NAME_MAX];
    char type[MDNS_NAME_MAX];
    char host[MDNS_NAME_MAX];
    char path[MDNS_CLIENT_PATH_MAX];
    uint16_t port;
    int have_srv;
    int have_txt;
    int found_logged;
    int srv_logged;
    int txt_logged;
    uint64_t found_ms;
    uint64_t last_seen_ms;
    uint32_t ttl;
    size_t nendpoints;
    mdns_client_endpoint_t endpoints[MDNS_CLIENT_MAX_ENDPOINTS];
    int healthy;
} mdns_client_service_t;

typedef enum {
    MDNS_CLIENT_EVT_FOUND = 1,
    MDNS_CLIENT_EVT_LOST = 2,
    MDNS_CLIENT_EVT_HEALTH = 3
} mdns_client_event_t;

typedef void (*mdns_client_on_change_fn)(mdns_client_event_t ev, const mdns_client_service_t *svc);

typedef struct {
    int sockfd_v4;
    int sockfd_v6;
    char if_name[IF_NAMESIZE];
    unsigned ifindex;
} mdns_client_iface_t;

typedef struct {
    pthread_mutex_t lock;
    mdns_client_service_t *services;
    size_t nservices;
    size_t max_services;
    char **browse_types;
    size_t ntypes;
    mdns_client_iface_t *ifaces;
    size_t nifaces;
    int enable_ipv4;
    int enable_ipv6;
    int scan_interval_ms;
    int health_check_enabled;
    int health_check_interval_ms;
    int health_check_timeout_ms;
    int health_check_retries;
    int own_services;
    pthread_t thread;
    int thread_started;
} mdns_client_t;

extern mdns_client_t *mdns_client_instance;
extern mdns_client_on_change_fn mdns_client_on_change;

mdns_client_t *mdns_client_create(const MDNSClientConfig *config);
void mdns_client_destroy(mdns_client_t *client);
int mdns_client_start(const MDNSClientConfig *config);
void mdns_client_stop(void);
void *mdns_client_thread(void *arg);
int mdns_client_handle_response(mdns_client_t *client, const uint8_t *msg,
                                size_t msglen, unsigned ifindex);
int mdns_client_send_query(mdns_client_t *client, const char *qname, uint16_t qtype);
int mdns_client_build_query(uint8_t *out, size_t cap, size_t *outlen,
                            const char *qname, uint16_t qtype);
int mdns_client_configured_type(const mdns_client_t *client, const char *type);
int mdns_client_add_endpoint(mdns_client_service_t *svc, int family,
                             const uint8_t *addr, uint8_t addrlen, unsigned ifindex);
int mdns_client_endpoint_rank(int family, const uint8_t *addr, uint8_t addrlen);
void mdns_client_sort_endpoints(mdns_client_service_t *svc);
void mdns_client_drop_service(mdns_client_t *client, size_t index, const char *reason);
uint64_t mdns_client_now_ms(void);
mdns_client_service_t *mdns_client_find_instance(mdns_client_t *client, const char *instance);
mdns_client_service_t *mdns_client_insert_instance(mdns_client_t *client, const char *instance,
                                                   const char *type);
void mdns_client_log_addr(const char *host, int family, const uint8_t *addr);
int mdns_client_handle_goodbye(mdns_client_t *client, const mdns_rr *rr, const uint8_t *msg,
                               size_t msglen);
int mdns_client_send_on_ifaces(mdns_client_t *client, const uint8_t *packet, size_t packet_len);
void mdns_client_browse_all(mdns_client_t *client);
void mdns_client_resolve_pending(mdns_client_t *client);
void mdns_client_close_ifaces(mdns_client_t *client);
int mdns_client_open_ifaces(mdns_client_t *client);
void mdns_client_recv_iface(mdns_client_t *client, unsigned ifindex, int fd);
int mdns_client_add_browse_type(mdns_client_t *client, const char *type);
int mdns_client_is_own_instance(const char *instance);
int mdns_client_service_visible(const mdns_client_t *client, const mdns_client_service_t *svc);
size_t mdns_client_count(void);
mdns_client_service_t *mdns_client_snapshot(size_t *count);
void mdns_client_snapshot_free(mdns_client_service_t *copy);
size_t mdns_client_lookup_by_type(const char *type, mdns_client_service_t **out);
int mdns_client_tcp_check(const mdns_client_endpoint_t *ep, uint16_t port, int timeout_ms);
void mdns_client_health_scan(mdns_client_t *client);
json_t *mdns_client_info_json(void);

#endif
