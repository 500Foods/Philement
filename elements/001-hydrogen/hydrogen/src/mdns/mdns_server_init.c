/*
 * mDNS Server Initialization for the Hydrogen printer.
 *
 * Contains initialization functionality split from the main mDNS server
 * implementation. This includes server setup and resource allocation.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "mdns_keys.h"
#include "mdns_server.h"

extern AppConfig *app_config;

int create_multicast_socket(int family, const char *group, const char *if_name);

// Function prototypes for helper functions - made non-static for testing
mdns_server_t *mdns_server_allocate(void);
network_info_t *mdns_server_get_network_info(mdns_server_t *server);
int mdns_server_allocate_interfaces(mdns_server_t *server, const network_info_t *net_info_instance);
int mdns_server_init_interfaces(mdns_server_t *server, const network_info_t *net_info_instance);
int mdns_server_validate_services(const mdns_server_service_t *services, size_t num_services);
int mdns_server_allocate_services(mdns_server_t *server, mdns_server_service_t *services, size_t num_services);
int mdns_server_init_services(mdns_server_t *server, const mdns_server_service_t *services, size_t num_services);
int mdns_server_setup_hostname(mdns_server_t *server);
int mdns_server_init_service_info(mdns_server_t *server, const char *app_name, const char *id,
                                  const char *friendly_name, const char *model, const char *manufacturer,
                                  const char *sw_version, const char *hw_version, const char *config_url);
void mdns_server_cleanup(mdns_server_t *server, network_info_t *net_info_instance);

// Helper functions for mdns_server_init - made non-static for testing
mdns_server_t *mdns_server_allocate(void) {
    mdns_server_t *mdns_server_instance = malloc(sizeof(mdns_server_t));
    if (!mdns_server_instance) {
        log_this(SR_MDNS_SERVER, "Out of memory", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    return mdns_server_instance;
}

network_info_t *mdns_server_get_network_info(mdns_server_t *server __attribute__((unused))) {
    // Get raw network info first
    network_info_t *raw_net_info = get_network_info();
    if (!raw_net_info) {
        log_this(SR_MDNS_SERVER, "Failed to get network info", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Filter interfaces based on Network.Available configuration
    network_info_t *net_info_instance = filter_enabled_interfaces(raw_net_info, app_config);
    free_network_info(raw_net_info); // Clean up raw network info

    if (!net_info_instance || net_info_instance->count == 0) {
        log_this(SR_MDNS_SERVER, "No enabled interfaces found after filtering", LOG_LEVEL_DEBUG, 0);
        if (net_info_instance) {
            free_network_info(net_info_instance);
        }
        return NULL;
    }

    return net_info_instance;
}

int mdns_server_allocate_interfaces(mdns_server_t *server, const network_info_t *net_info_instance) {
    // Allocate space for interfaces
    size_t interface_count = (net_info_instance->count >= 0) ? (size_t)net_info_instance->count : 0;
    server->interfaces = malloc(sizeof(mdns_server_interface_t) * interface_count);
    server->num_interfaces = 0;
    if (!server->interfaces) {
        log_this(SR_MDNS_SERVER, "Out of memory for interfaces", LOG_LEVEL_DEBUG, 0);
        return -1;
    }
    return 0;
}

int mdns_server_init_interfaces(mdns_server_t *server, const network_info_t *net_info_instance) {
    // Initialize each interface
    for (int i = 0; i < net_info_instance->count; i++) {
        const interface_t *iface = &net_info_instance->interfaces[i];

        // Skip loopback and interfaces without IPs
        if (strcmp(iface->name, "lo") == 0 || iface->ip_count == 0) {
            continue;
        }

        mdns_server_interface_t *mdns_server_if = &server->interfaces[server->num_interfaces];
        mdns_server_if->if_name = strdup(iface->name);
        if (iface->ip_count > 0) {
            size_t ip_count = (size_t)iface->ip_count;
            mdns_server_if->ip_addresses = malloc(sizeof(char*) * ip_count);
            mdns_server_if->num_addresses = ip_count;
        } else {
            mdns_server_if->ip_addresses = NULL;
            mdns_server_if->num_addresses = 0;
        }

        if (!mdns_server_if->if_name || (iface->ip_count > 0 && !mdns_server_if->ip_addresses)) {
            log_this(SR_MDNS_SERVER, "Out of memory for interface %s", LOG_LEVEL_DEBUG, 1, iface->name);
            return -1;
        }

        // Copy IP addresses
        for (int j = 0; j < iface->ip_count; j++) {
            mdns_server_if->ip_addresses[j] = strdup(iface->ips[j]);
            if (!mdns_server_if->ip_addresses[j]) {
                log_this(SR_MDNS_SERVER, "Out of memory for IP address", LOG_LEVEL_DEBUG, 0);
                return -1;
            }
        }

        // Create sockets
        mdns_server_if->sockfd_v4 = create_multicast_socket(AF_INET, MDNS_GROUP_V4, mdns_server_if->if_name);
        mdns_server_if->sockfd_v6 = server->enable_ipv6 ? create_multicast_socket(AF_INET6, MDNS_GROUP_V6, mdns_server_if->if_name) : -1;

        if (mdns_server_if->sockfd_v4 < 0 && (!server->enable_ipv6 || mdns_server_if->sockfd_v6 < 0)) {
            log_this(SR_MDNS_SERVER, "Failed to create sockets for interface %s", LOG_LEVEL_DEBUG, 1, mdns_server_if->if_name);
            return -1;
        }

        // Initialize failure tracking for automatic disable/enable
        mdns_server_if->consecutive_failures = 0;
        mdns_server_if->disabled = 0;

        // Initialize protocol-level failure tracking
        mdns_server_if->v4_consecutive_failures = 0;
        mdns_server_if->v6_consecutive_failures = 0;
        mdns_server_if->v4_disabled = 0;
        mdns_server_if->v6_disabled = 0;

        server->num_interfaces++;
    }

    if (server->num_interfaces == 0) {
        log_this(SR_MDNS_SERVER, "No usable interfaces found", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    return 0;
}

int mdns_server_validate_services(const mdns_server_service_t *services, size_t num_services) {
    if (num_services > 0 && !services) {
        log_this(SR_MDNS_SERVER, "Services array is NULL but num_services > 0", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    return 0;
}

int mdns_server_allocate_services(mdns_server_t *server, mdns_server_service_t *services __attribute__((unused)), size_t num_services) {
    server->num_services = num_services;
    if (num_services > 0) {
        server->services = malloc(sizeof(mdns_server_service_t) * num_services);
        if (!server->services) {
            log_this(SR_MDNS_SERVER, "Out of memory for services", LOG_LEVEL_DEBUG, 0);
            return -1;
        }
    } else {
        server->services = NULL;
    }
    return 0;
}

int mdns_server_init_services(mdns_server_t *server, const mdns_server_service_t *services, size_t num_services) {
    for (size_t i = 0; i < num_services; i++) {
        server->services[i].name = strdup(services[i].name);
        server->services[i].type = strdup(services[i].type);
        server->services[i].port = services[i].port;
        server->services[i].num_txt_records = services[i].num_txt_records;
        if (services[i].num_txt_records > 0) {
            size_t txt_count = (size_t)services[i].num_txt_records;
            server->services[i].txt_records = malloc(sizeof(char*) * txt_count);
            if (!server->services[i].txt_records) {
                log_this(SR_MDNS_SERVER, "Out of memory for TXT records", LOG_LEVEL_DEBUG, 0);
                return -1;
            }
        } else {
            server->services[i].txt_records = NULL;
        }
        for (size_t j = 0; j < (size_t)services[i].num_txt_records; j++) {
            server->services[i].txt_records[j] = strdup(services[i].txt_records[j]);
            if (!server->services[i].txt_records[j]) {
                log_this(SR_MDNS_SERVER, "Out of memory for TXT record string", LOG_LEVEL_DEBUG, 0);
                return -1;
            }
        }
    }
    return 0;
}

int mdns_server_setup_hostname(mdns_server_t *server) {
    // Initialize hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to get hostname: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        strncpy(hostname, "unknown", sizeof(hostname));
    }
    char *dot = strchr(hostname, '.');
    if (dot) *dot = '\0';

    server->hostname = malloc(strlen(hostname) + 7);
    if (!server->hostname) {
        log_this(SR_MDNS_SERVER, "Out of memory", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    sprintf(server->hostname, "%s.local", hostname);
    return 0;
}

int mdns_server_init_service_info(mdns_server_t *server, const char *app_name, const char *id,
                                  const char *friendly_name, const char *model, const char *manufacturer,
                                  const char *sw_version, const char *hw_version, const char *config_url) {
    // Initialize service info
    server->service_name = strdup(app_name);
    server->device_id = strdup(id);
    server->friendly_name = strdup(friendly_name);
    server->model = strdup(model);
    server->manufacturer = strdup(manufacturer);
    server->sw_version = strdup(sw_version);
    server->hw_version = strdup(hw_version);
    server->config_url = strdup(config_url);
    server->secret_key = generate_secret_mdns_key();

    if (!server->service_name || !server->device_id || !server->friendly_name ||
        !server->model || !server->manufacturer || !server->sw_version ||
        !server->hw_version || !server->config_url || !server->secret_key) {
        log_this(SR_MDNS_SERVER, "Out of memory", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    return 0;
}

void mdns_server_cleanup(mdns_server_t *server, network_info_t *net_info_instance) {
    if (server) {
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                mdns_server_interface_t *mdns_server_if = &server->interfaces[i];
                if (mdns_server_if->if_name) free(mdns_server_if->if_name);
                if (mdns_server_if->ip_addresses) {
                    for (size_t j = 0; j < mdns_server_if->num_addresses; j++) {
                        if (mdns_server_if->ip_addresses[j]) free(mdns_server_if->ip_addresses[j]);
                    }
                    free(mdns_server_if->ip_addresses);
                }
                if (mdns_server_if->sockfd_v4 >= 0) close(mdns_server_if->sockfd_v4);
                if (mdns_server_if->sockfd_v6 >= 0) close(mdns_server_if->sockfd_v6);
            }
            free(server->interfaces);
        }
        if (server->services) {
            for (size_t i = 0; i < server->num_services; i++) {
                if (server->services[i].name) free(server->services[i].name);
                if (server->services[i].type) free(server->services[i].type);
                if (server->services[i].txt_records) {
                    for (size_t j = 0; j < server->services[i].num_txt_records; j++) {
                        if (server->services[i].txt_records[j]) free(server->services[i].txt_records[j]);
                    }
                    free(server->services[i].txt_records);
                }
            }
            free(server->services);
        }
        if (server->hostname) free(server->hostname);
        if (server->service_name) free(server->service_name);
        if (server->device_id) free(server->device_id);
        if (server->friendly_name) free(server->friendly_name);
        if (server->model) free(server->model);
        if (server->manufacturer) free(server->manufacturer);
        if (server->sw_version) free(server->sw_version);
        if (server->hw_version) free(server->hw_version);
        if (server->config_url) free(server->config_url);
        if (server->secret_key) free(server->secret_key);
        free(server);
    }
    if (net_info_instance) free_network_info(net_info_instance);
}

mdns_server_t *mdns_server_init(const char *app_name, const char *id, const char *friendly_name,
                  const char *model, const char *manufacturer, const char *sw_version,
                  const char *hw_version, const char *config_url,
                  mdns_server_service_t *services, size_t num_services, int enable_ipv6) {
    mdns_server_t *server = NULL;
    network_info_t *net_info_instance = NULL;

    // Step 1: Allocate server structure
    server = mdns_server_allocate();
    if (!server) {
        return NULL;
    }
    server->enable_ipv6 = enable_ipv6;

    // Step 2: Get network information
    net_info_instance = mdns_server_get_network_info(server);
    if (!net_info_instance) {
        mdns_server_cleanup(server, NULL);
        return NULL;
    }

    // Step 3: Allocate interface array
    if (mdns_server_allocate_interfaces(server, net_info_instance) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Step 4: Initialize interfaces
    if (mdns_server_init_interfaces(server, net_info_instance) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Step 5: Validate services
    if (mdns_server_validate_services(services, num_services) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Step 6: Allocate services
    if (mdns_server_allocate_services(server, services, num_services) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Step 7: Initialize services
    if (mdns_server_init_services(server, services, num_services) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Step 8: Setup hostname
    if (mdns_server_setup_hostname(server) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Step 9: Initialize service info
    if (mdns_server_init_service_info(server, app_name, id, friendly_name, model,
                                      manufacturer, sw_version, hw_version, config_url) != 0) {
        mdns_server_cleanup(server, net_info_instance);
        return NULL;
    }

    // Success
    log_this(SR_MDNS_SERVER, "mDNS Server initialized with hostname: %s", LOG_LEVEL_STATE, 1, server->hostname);
    free_network_info(net_info_instance);
    return server;
}