/*
 * mDNS Server (multicast DNS) service discovery interface for the Hydrogen Server
 */

#ifndef MDNS_SERVER_H
#define MDNS_SERVER_H

/*
 * Include Organization:
 * - System headers: For threading and network types
 * - Project headers: Network interface abstraction
 */

// System Libraries
#include <src/globals.h>
#include <stdint.h>
#include <pthread.h>

// Project Libraries
#include <src/network/network.h>
#include <src/mdns/mdns_wire.h>

// DNS header structure (moved from mdns_server_announce.c for test access)
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

/*
 * Service Description:
 * Represents a single advertised network service (e.g., HTTP, WebSocket).
 * 
 * Why this design?
 * - Separates service identity from implementation
 * - Allows multiple instances of same service
 * - Flexible metadata through TXT records
 * - Memory-efficient string handling
 */
typedef struct {
    char *name;              // Service instance name (may gain " (N)" after conflict)
    char *type;              // Service type (e.g., _http._tcp)
    int port;                // Service port number
    char **txt_records;      // Array of TXT record strings
    size_t num_txt_records;  // Number of TXT records
    char *name_base;         // Original instance label for rename
    volatile int claimed;    // 1 after this instance survives probing
    volatile int probe_conflict;
    int name_attempts;
    uint64_t last_send_ms;   // Per-name rate limit (RFC 6762 s6: 1s minimum)
} mdns_server_service_t;

/*
 * mDNS Server State:
 * Core server structure maintaining all discovery-related state.
 * 
 * Design Considerations:
 * 1. Network Handling
 *    - Separate sockets for IPv4/IPv6
 *    - Optional IPv6 support
 *    - Socket state tracking
 * 
 * 2. Device Identity
 *    - Unique identification
 *    - Human-readable names
 *    - Version tracking
 *    - Security parameters
 * 
 * 3. Service Management
 *    - Multiple service support
 *    - Dynamic service updates
 *    - Resource cleanup
 */

// Per-interface socket information
typedef struct {
    char *if_name;         // Interface name
    int sockfd_v4;         // IPv4 socket for this interface
    int sockfd_v6;         // IPv6 socket for this interface
    char **ip_addresses;   // IP addresses for this interface
    size_t num_addresses;  // Number of IP addresses

    // Legacy interface-level tracking (maintained for compatibility)
    int consecutive_failures; // Count of consecutive announcement failures (all protocols)
    int disabled;          // Flag to indicate manual interface disable state

    // Protocol-level failure tracking
    int v4_consecutive_failures; // IPv4-specific consecutive failure count
    int v6_consecutive_failures; // IPv6-specific consecutive failure count
    int v4_disabled;       // IPv4 protocol disabled flag
    int v6_disabled;       // IPv6 protocol disabled flag
} mdns_server_interface_t;

typedef struct {
    // Network interfaces
    mdns_server_interface_t *interfaces;  // Array of interface sockets
    size_t num_interfaces;         // Number of interfaces
    int enable_ipv6;              // IPv6 support flag
    
    // Device identification
    char *hostname;         // Local hostname (host.local, may become host-N.local)
    char *service_name;     // Primary service name
    char *device_id;        // Unique device identifier
    char *friendly_name;    // Human-readable name
    char *secret_key;       // Authentication key
    
    // Device information
    char *model;           // Hardware model
    char *manufacturer;    // Device manufacturer
    char *sw_version;      // Software version
    char *hw_version;      // Hardware version
    char *config_url;      // Configuration interface URL
    
    // Service registry
    mdns_server_service_t *services;  // Array of advertised services
    size_t num_services;       // Number of services

    char *hostname_base;    // Host label without .local
    volatile int hostname_claimed;
    volatile int hostname_conflict;
    int hostname_attempts;
    volatile int probe_failed; // 1: all names unclaimed, no announce/answer/goodbye
    uint64_t last_multicast_ms;     // Last unique multicast (rate limit anchor)
    uint64_t hostname_last_send_ms; // Per-name rate limit for hostname
    volatile int probe_tiebreak_lose; // Set by responder when we lose s8.2 tiebreak
    uint64_t (*now_ms_fn)(void);    // Injectable time source (defaults to mdns_client_now_ms)
    uint32_t (*rand_delay_ms_fn)(uint32_t min_ms, uint32_t max_ms); // Injectable RNG for shared delay
} mdns_server_t;

/*
 * Thread Arguments:
 * Packaged data for announcement and responder threads.
 * 
 * Why separate threads?
 * - Decouples announcement from response handling
 * - Allows independent control of each function
 * - Simplifies shutdown coordination
 * - Enables different timing for each operation
 */
typedef struct {
    mdns_server_t *mdns_server;             // Server state
    int port;                        // Service port
    const network_info_t *net_info;  // Network interface info
    volatile int *running;           // Thread control flag
} mdns_server_thread_arg_t;

/*
 * Core mDNS Functions:
 * Primary interface for mDNS service management.
 */

// Initialize mDNS server with device information and services
// Returns NULL on any initialization failure
mdns_server_t *mdns_server_init(const char *app_name,        // Application identifier
                  const char *id,               // Unique device ID
                  const char *friendly_name,    // Human-readable name
                  const char *model,            // Device model
                  const char *manufacturer,     // Device manufacturer
                  const char *sw_version,       // Software version
                  const char *hw_version,       // Hardware version
                  const char *config_url,       // Config interface URL
                  mdns_server_service_t *services,     // Array of services
                  size_t num_services,          // Number of services
                  int enable_ipv6);            // IPv6 support flag

// Construct announcement packet following RFC 6762
void mdns_server_build_announcement(uint8_t *packet,           // Output buffer
                           size_t *packet_len,          // Packet length
                           const char *hostname,        // Local hostname
                           const mdns_server_t *mdns_server,         // Server state
                           uint32_t ttl,               // Record TTL
                           const network_info_t *net_info); // Network info

// Broadcast service announcements on all interfaces
void mdns_server_send_announcement(mdns_server_t *mdns_server, const network_info_t *net_info);

// Clean shutdown of mDNS server
void mdns_server_shutdown(mdns_server_t *mdns_server);

// Get configured retry count for interface failure detection
int get_mdns_server_retry_count(const AppConfig* config);

// Background thread for periodic announcements
void *mdns_server_announce_loop(void *arg);

// Background thread for handling incoming queries
void *mdns_server_responder_loop(void *arg);

/**
 * Close sockets and free interface resources
 * Made non-static for unit testing
 */
void close_mdns_server_interfaces(mdns_server_t *mdns_server);

int create_multicast_socket(int family, const char *group, const char *if_name);

// Functions made non-static for unit testing
void _mdns_server_build_interface_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                                             const mdns_server_t *mdns_server_instance, uint32_t ttl, const mdns_server_interface_t *iface);
network_info_t *create_single_interface_net_info(const mdns_server_interface_t *iface);
void free_single_interface_net_info(network_info_t *net_info_instance);

#define MDNS_W_PTR   0x01u
#define MDNS_W_SRV   0x02u
#define MDNS_W_TXT   0x04u
#define MDNS_W_A     0x08u
#define MDNS_W_AAAA  0x10u
#define MDNS_W_NSEC  0x20u
#define MDNS_W_SD    0x40u

#define MDNS_DNS_SD_NAME "_services._dns-sd._udp.local"
#define MDNS_LEGACY_TTL_CAP 10u
#define MDNS_SERVER_WANT_MAX_SVC 32
#define MDNS_PROBE_TRIES 3
#define MDNS_PROBE_GAP_MS 250
#define MDNS_MAX_NAME_ATTEMPTS 8
#define MDNS_RATE_LIMIT_MS 1000
#define MDNS_SHARED_DELAY_MIN 20
#define MDNS_SHARED_DELAY_MAX 120

typedef struct {
    uint32_t host_answer;
    uint32_t host_additional;
    uint32_t svc_answer[MDNS_SERVER_WANT_MAX_SVC];
    uint32_t svc_additional[MDNS_SERVER_WANT_MAX_SVC];
    size_t nsvc;
    int qu;
} mdns_server_want_t;

void mdns_server_format_instance_name(const mdns_server_service_t *svc, char *out, size_t cap);
void mdns_server_want_clear(mdns_server_want_t *w, size_t nsvc);
int mdns_server_want_empty(const mdns_server_want_t *w);
void mdns_server_want_add_question(mdns_server_want_t *w, const mdns_server_t *server, const mdns_rr *q);
void mdns_server_strip_known_answers(mdns_server_want_t *w, const mdns_server_t *server,
                                     const uint8_t *raw, size_t rawlen, const mdns_msg *msg);
int mdns_server_should_unicast(int legacy, int qu);
int mdns_server_should_multicast(int legacy, int qu);
uint16_t mdns_server_sockaddr_port(const void *src_addr, uint32_t src_len);
const mdns_server_interface_t *mdns_server_iface_for_sock(const mdns_server_t *server, int sockfd);
uint32_t mdns_server_response_ttl(uint32_t base, int legacy);
int mdns_server_iface_has_af(const mdns_server_interface_t *iface, int family);
void mdns_server_want_apply_missing_family(mdns_server_want_t *w, const mdns_server_interface_t *iface);
int mdns_server_put_host_addrs(mdns_buf *b, const char *hostname, const mdns_server_interface_t *iface,
                               uint32_t ttl, int flush, uint32_t bits, uint16_t *count);
int mdns_server_put_host_nsec(mdns_buf *b, const char *hostname, const mdns_server_interface_t *iface,
                              uint32_t ttl, int flush, uint16_t *count);
int mdns_server_put_service_bits(mdns_buf *b, const mdns_server_t *server, size_t si, const char *hostname,
                                 uint32_t bits, uint32_t shared_ttl, uint32_t host_ttl, int flush, uint16_t *count);
void mdns_server_build_query_response(uint8_t *packet, size_t *packet_len,
                                      const mdns_server_t *server,
                                      const mdns_server_interface_t *iface,
                                      const mdns_msg *query,
                                      const mdns_server_want_t *want,
                                      int legacy);
void mdns_server_send_query_response(int sockfd, const mdns_server_t *server, const void *src_addr,
                                      uint32_t src_len, int legacy, int qu, const uint8_t *packet, size_t packet_len,
                                      const mdns_server_want_t *want);

bool mdns_server_process_query_packet(mdns_server_t *mdns_server_instance,
                                        const network_info_t *net_info_instance,
                                        const uint8_t *buffer,
                                        ssize_t len,
                                        int sockfd,
                                        const void *src_addr,
                                        uint32_t src_len);

void mdns_server_next_instance_name(const char *base, unsigned attempt, char *out, size_t cap);
void mdns_server_next_hostname(const char *base, unsigned attempt, char *out, size_t cap);
int mdns_server_all_claimed(const mdns_server_t *server);
int mdns_server_any_claimed(const mdns_server_t *server);
int mdns_server_any_probe_conflict(const mdns_server_t *server);
void mdns_server_clear_probe_conflicts(mdns_server_t *server);
void mdns_server_claim_unclaimed(mdns_server_t *server);
void mdns_server_probe_fail(mdns_server_t *server);
int mdns_server_apply_probe_renames(mdns_server_t *server);
int mdns_server_rr_conflicts_probe(const mdns_server_t *server, const mdns_rr *rr);
void mdns_server_note_probe_conflicts(mdns_server_t *server, const mdns_msg *msg);
void mdns_server_want_mask_unclaimed(mdns_server_want_t *w, const mdns_server_t *server);
void mdns_server_build_probe(uint8_t *packet, size_t *packet_len, const mdns_server_t *server,
                             const mdns_server_interface_t *iface);
void mdns_server_send_probe(mdns_server_t *server);
int mdns_server_run_probe(mdns_server_t *server);
int mdns_server_rr_conflicts_claimed(const mdns_server_t *server, const mdns_rr *rr,
                                     const uint8_t *msg, size_t msglen);
void mdns_server_defend_claimed(mdns_server_t *server, const mdns_msg *msg,
                                const uint8_t *raw, size_t rawlen);
int mdns_server_addr_is_ours(const mdns_server_t *server, int family,
                             const uint8_t *addr, size_t addrlen);
int mdns_server_txt_rdata_matches(const mdns_server_service_t *svc,
                                  const uint8_t *rdata, size_t rdlen);
int mdns_server_srv_rdata_matches(const mdns_server_t *server, const mdns_server_service_t *svc,
                                  const uint8_t *msg, size_t msglen, const mdns_rr *rr);
int mdns_server_want_is_shared_only(const mdns_server_want_t *w);
int mdns_server_rate_fresh(const mdns_server_t *server, const char *name, uint64_t last_ms);
uint32_t mdns_server_default_rand_delay(uint32_t min_ms, uint32_t max_ms);
int mdns_server_rr_cmp(const mdns_rr *a, const mdns_rr *b,
                       const uint8_t *msg_a, const uint8_t *msg_b,
                       size_t msglen_a, size_t msglen_b);
int mdns_server_check_tiebreak(mdns_server_t *server, int sockfd, const mdns_msg *msg,
                               const uint8_t *raw, size_t rawlen);

#endif // MDNS_SERVER_H
