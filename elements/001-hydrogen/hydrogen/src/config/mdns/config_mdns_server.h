/*
 * mDNS Server Configuration
 *
 * Defines the configuration structure and defaults for the mDNS server subsystem.
 * This includes settings for:
 * - Device identification
 * - Service advertisement
 * - Network protocols
 *
 * Design Decisions:
 * - Default values ensure basic device identification
 * - Services array allows dynamic service registration
 * - IPv6 disabled by default for compatibility
 * - Memory management follows strict allocation/cleanup patterns
 */

#ifndef HYDROGEN_CONFIG_MDNS_SERVER_H
#define HYDROGEN_CONFIG_MDNS_SERVER_H

// Core system headers
#include <stddef.h>

// Project headers
#include "../config_forward.h"
#include "../../mdns/mdns_server.h"  // For mdns_server_service_t

// Default values
#define DEFAULT_MDNS_SERVER_DEVICE_ID "hydrogen"
#define DEFAULT_MDNS_SERVER_FRIENDLY_NAME "Hydrogen Server"
#define DEFAULT_MDNS_SERVER_MODEL "Hydrogen"
#define DEFAULT_MDNS_SERVER_MANUFACTURER "Philement"

// mDNS server configuration structure
struct MDNSServerConfig {
    int enabled;
    int enable_ipv6;
    char* device_id;
    char* friendly_name;
    char* model;
    char* manufacturer;
    char* version;
    mdns_server_service_t* services;
    size_t num_services;
};
typedef struct MDNSServerConfig MDNSServerConfig;

/*
 * Initialize mDNS server configuration with default values
 *
 * This function initializes a new MDNSServerConfig structure with default values.
 * It allocates memory for all string fields and initializes the services array
 * to NULL. The configuration is enabled by default with IPv6 disabled.
 *
 * @param config Pointer to MDNSServerConfig structure to initialize
 * @return 0 on success, -1 on failure (memory allocation or null pointer)
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for any string field
 */
int config_mdns_server_init(MDNSServerConfig* config);

/*
 * Free resources allocated for mDNS server configuration
 *
 * This function frees all resources allocated by config_mdns_server_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * Note: This will also free the services array if allocated, but not
 * the individual services which should be managed by the mDNS server.
 *
 * @param config Pointer to MDNSServerConfig structure to cleanup
 */
void config_mdns_server_cleanup(MDNSServerConfig* config);

/*
 * Validate mDNS server configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all required string fields are present and non-empty
 * - Validates services array consistency if services are configured
 * - Checks for invalid combinations of settings
 *
 * @param config Pointer to MDNSServerConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required strings are missing or empty
 * - If num_services > 0 but services array is NULL
 */
int config_mdns_server_validate(const MDNSServerConfig* config);

#endif /* HYDROGEN_CONFIG_MDNS_SERVER_H */