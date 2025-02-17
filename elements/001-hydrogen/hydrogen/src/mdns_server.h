/*
 * mDNS (multicast DNS) service discovery interface for the Hydrogen printer.
 * 
 * This module implements zero-configuration network discovery, allowing Hydrogen
 * printers to be automatically discovered on local networks. Key features:
 * 
 * 1. Protocol Support
 *    - Full mDNS (RFC 6762) compliance
 *    - DNS-SD service registration (RFC 6763)
 *    - Dual-stack IPv4/IPv6 support
 *    - Multiple service type advertisement
 * 
 * 2. Discovery Features
 *    - Automatic printer detection
 *    - Service capability advertisement
 *    - Instance naming/renaming
 *    - Conflict resolution
 * 
 * 3. Security Considerations
 *    - Scope limitation to local network
 *    - Response rate limiting
 *    - Query validation
 *    - Resource exhaustion prevention
 * 
 * 4. Performance
 *    - Efficient packet construction
 *    - Minimal memory footprint
 *    - Cached responses
 *    - Announcement coalescing
 */

#ifndef MDNS_H
#define MDNS_H

/*
 * Include Organization:
 * - System headers: For threading and network types
 * - Project headers: Network interface abstraction
 */

// System Libraries
#include <stdint.h>
#include <pthread.h>

// Project Libraries
#include "network.h"

/*
 * Protocol Constants:
 * 
 * Network Parameters:
 * - MDNS_PORT: Standard mDNS port (5353)
 * - MDNS_GROUP_*: Multicast group addresses
 * - MDNS_TTL: Time-to-live for multicast packets
 * 
 * DNS Record Types:
 * - Used in packet construction
 * - Each type serves specific discovery purpose
 * - Follows RFC 1035 standards
 * 
 * Packet Construction:
 * - Flag values for response headers
 * - Size limits for UDP datagrams
 * - Class values for Internet records
 */
#define MDNS_PORT 5353                    // Standard mDNS port
#define MDNS_GROUP_V4 "224.0.0.251"      // IPv4 multicast group
#define MDNS_GROUP_V6 "ff02::fb"         // IPv6 multicast group

#define MDNS_TTL 255                      // Default TTL for announcements

// DNS Type Values (RFC 1035)
#define MDNS_TYPE_A 1      // Host address (IPv4)
#define MDNS_TYPE_PTR 12   // Domain name pointer (service discovery)
#define MDNS_TYPE_TXT 16   // Text strings (metadata)
#define MDNS_TYPE_AAAA 28  // Host address (IPv6)
#define MDNS_TYPE_SRV 33   // Service location
#define MDNS_TYPE_ANY 255  // Request for all records

#define MDNS_CLASS_IN 1                   // Internet class records
#define MDNS_FLAG_RESPONSE 0x8400         // Response packet flag
#define MDNS_FLAG_AUTHORITATIVE 0x0400    // Authoritative answer flag
#define MDNS_MAX_PACKET_SIZE 1500         // MTU-compatible size

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
} mdns_service_t;

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
typedef struct {
    // Network sockets
    int sockfd_v4;          // IPv4 multicast socket
    int sockfd_v6;          // IPv6 multicast socket
    int enable_ipv6;        // IPv6 support flag
    
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
    mdns_service_t *services;  // Array of advertised services
    size_t num_services;       // Number of services
} mdns_t;

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
    mdns_t *mdns;                    // Server state
    int port;                        // Service port
    const network_info_t *net_info;  // Network interface info
    volatile int *running;           // Thread control flag
} mdns_thread_arg_t;

/*
 * Core mDNS Functions:
 * Primary interface for mDNS service management.
 */

// Initialize mDNS server with device information and services
// Returns NULL on any initialization failure
mdns_t *mdns_init(const char *app_name,        // Application identifier
                  const char *id,               // Unique device ID
                  const char *friendly_name,    // Human-readable name
                  const char *model,            // Device model
                  const char *manufacturer,     // Device manufacturer
                  const char *sw_version,       // Software version
                  const char *hw_version,       // Hardware version
                  const char *config_url,       // Config interface URL
                  mdns_service_t *services,     // Array of services
                  size_t num_services,          // Number of services
                  int enable_ipv6);            // IPv6 support flag

// Construct announcement packet following RFC 6762
void mdns_build_announcement(uint8_t *packet,           // Output buffer
                           size_t *packet_len,          // Packet length
                           const char *hostname,        // Local hostname
                           const mdns_t *mdns,         // Server state
                           uint32_t ttl,               // Record TTL
                           const network_info_t *net_info); // Network info

// Broadcast service announcements on all interfaces
void mdns_send_announcement(mdns_t *mdns, const network_info_t *net_info);

// Clean shutdown of mDNS server
void mdns_shutdown(mdns_t *mdns);

// Background thread for periodic announcements
void *mdns_announce_loop(void *arg);

// Background thread for handling incoming queries
void *mdns_responder_loop(void *arg);

#endif // MDNS_H
