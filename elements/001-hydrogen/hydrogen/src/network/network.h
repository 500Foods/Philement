/*
 * Network Management
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "../globals.h"
#include <stdbool.h>
#include <stdint.h>
#include <net/if.h>      
#include <netinet/in.h>

// Forward declarations
struct AppConfig;  // Forward declare to avoid include cycle

// Interface configuration functions
bool is_interface_configured(const char* interface_name, bool* is_available);

typedef struct {
    char name[IF_NAMESIZE];
    char mac[18];
    char ips[MAX_IPS][INET6_ADDRSTRLEN];
    int ip_count;
    double ping_ms[MAX_IPS];    // Response time in ms for each IP
    bool is_ipv6[MAX_IPS];      // Whether each IP is IPv6
} interface_t;

typedef struct {
    int primary_index;
    int count;
    interface_t interfaces[MAX_INTERFACES];
} network_info_t;

/*
 * Core network functions
 * Why These Functions?
 * - Port management for services
 * - Network interface discovery
 * - Resource cleanup
 * - Graceful shutdown
 * - Network testing and monitoring
 */
int find_available_port(int start_port);
network_info_t *get_network_info(void);
void free_network_info(network_info_t *info);
bool network_shutdown(void);  // Gracefully shuts down all network interfaces

// Test network interfaces with ping
// Returns true if successful, false on error
// Updates ping_ms and is_ipv6 fields in interface_t
bool test_network_interfaces(network_info_t *info);

#endif // NETWORK_H
