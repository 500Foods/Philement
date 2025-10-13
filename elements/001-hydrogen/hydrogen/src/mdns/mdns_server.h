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
#include "../globals.h"
#include <stdint.h>
#include <pthread.h>

// Project Libraries
#include <src/network/network.h>

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
    char *name;              // Service instance name
    char *type;              // Service type (e.g., _http._tcp)
    int port;                // Service port number
    char **txt_records;      // Array of TXT record strings
    size_t num_txt_records;  // Number of TXT records
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
    char *hostname;         // Local hostname
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

// Test-exposed functions (not part of public API)
uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len);
uint8_t *write_dns_name(uint8_t *ptr, const char *name);
uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen);
uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl);
uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl);
uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl);
int create_multicast_socket(int family, const char *group, const char *if_name);
void mdns_server_send_announcement(mdns_server_t *mdns_server, const network_info_t *net_info);


// DNS utility functions (moved from mdns_server.c for better modularity)
uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len);
uint8_t *write_dns_name(uint8_t *ptr, const char *name);
uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen);
uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl);
uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl);
uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl);

// Functions made non-static for unit testing
void _mdns_server_build_interface_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                                             const mdns_server_t *mdns_server_instance, uint32_t ttl, const mdns_server_interface_t *iface);
network_info_t *create_single_interface_net_info(const mdns_server_interface_t *iface);
void free_single_interface_net_info(network_info_t *net_info_instance);

// DNS packet processing helper (made non-static for unit testing)
bool mdns_server_process_query_packet(mdns_server_t *mdns_server_instance,
                                       const network_info_t *net_info_instance,
                                       const uint8_t *buffer,
                                       ssize_t len);

#endif // MDNS_SERVER_H
