/*
 * mDNS Server Main Module for the Hydrogen printer.
 *
 * This file serves as the main entry point for the mDNS server functionality.
 * The actual implementation has been split into smaller, more manageable modules:
 *
 * - mdns_server_socket.c: Socket creation and management
 * - mdns_server_threads.c: Thread management (announce and responder loops)
 * - mdns_server_init.c: Server initialization
 * - mdns_server_shutdown.c: Server shutdown and cleanup
 *
 * This modular approach improves maintainability, testability, and code organization.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <poll.h>

#include "mdns_keys.h"
#include "mdns_server.h"
#include "mdns_dns_utils.h"

// External declarations for functions implemented in split modules
extern int create_multicast_socket(int family, const char *group, const char *if_name);
extern void *mdns_server_announce_loop(void *arg);
extern void *mdns_server_responder_loop(void *arg);
extern mdns_server_t *mdns_server_init(const char *app_name, const char *id, const char *friendly_name,
                  const char *model, const char *manufacturer, const char *sw_version,
                  const char *hw_version, const char *config_url,
                  mdns_server_service_t *services, size_t num_services, int enable_ipv6);
extern void close_mdns_server_interfaces(mdns_server_t *mdns_server_instance);
extern void mdns_server_shutdown(mdns_server_t *mdns_server_instance);

// Get configured retry count for interface failure detection
int get_mdns_server_retry_count(const AppConfig* config) {
    if (!config) return 1;
    // Ensure retry_count is at least 1 to prevent division by zero or infinite loop
    return (config->mdns_server.retry_count > 0) ? config->mdns_server.retry_count : 1;
}
