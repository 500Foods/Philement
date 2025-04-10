/*
 * mDNS Client Configuration
 *
 * Defines the configuration structure and handlers for the mDNS client subsystem.
 * This includes settings for:
 * - Service discovery
 * - Network scanning
 * - Health checks
 * - Auto-configuration
 *
 * Design Decisions:
 * - Regular service scanning enabled by default
 * - Health checks for discovered services
 * - IPv6 disabled by default for compatibility
 * - Memory management follows strict allocation/cleanup patterns
 */

#ifndef HYDROGEN_CONFIG_MDNS_CLIENT_H
#define HYDROGEN_CONFIG_MDNS_CLIENT_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"

// Default values
#define DEFAULT_MDNS_CLIENT_SCAN_INTERVAL 30  // seconds
#define DEFAULT_MDNS_CLIENT_HEALTH_CHECK_INTERVAL 60  // seconds
#define DEFAULT_MDNS_CLIENT_MAX_SERVICES 100
#define DEFAULT_MDNS_CLIENT_RETRY_COUNT 3

// Service type structure
typedef struct MDNSServiceType {
    char* type;           // Service type (e.g., "_http._tcp.local")
    int required;         // Whether this service type is required
    int auto_connect;     // Whether to automatically connect to discovered services
} MDNSServiceType;

// mDNS client configuration structure
typedef struct MDNSClientConfig {
    bool enabled;
    bool enable_ipv6;
    
    // Scanning configuration
    int scan_interval;           // Seconds between service scans
    size_t max_services;         // Maximum number of tracked services
    int retry_count;            // Number of retries for failed operations
    
    // Health check configuration
    bool health_check_enabled;
    int health_check_interval;   // Seconds between health checks
    
    // Service types to discover
    MDNSServiceType* service_types;
    size_t num_service_types;
} MDNSClientConfig;

/*
 * Load mDNS client configuration from JSON
 *
 * This function loads the mDNS client configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_mdns_client_config(json_t* root, AppConfig* config);

/*
 * Initialize mDNS client configuration with default values
 *
 * This function initializes a new MDNSClientConfig structure with default values.
 * It allocates memory for service types array and sets conservative defaults
 * for scanning and health check intervals.
 *
 * @param config Pointer to MDNSClientConfig structure to initialize
 * @return 0 on success, -1 on failure (memory allocation or null pointer)
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for service types array
 */
int config_mdns_client_init(MDNSClientConfig* config);

/*
 * Free resources allocated for mDNS client configuration
 *
 * This function frees all resources allocated by config_mdns_client_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to MDNSClientConfig structure to cleanup
 */
void config_mdns_client_cleanup(MDNSClientConfig* config);

/*
 * Validate mDNS client configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies intervals are within acceptable ranges
 * - Validates service type configurations
 * - Checks for invalid combinations of settings
 *
 * @param config Pointer to MDNSClientConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If intervals are invalid
 * - If service type configuration is invalid
 */
int config_mdns_client_validate(const MDNSClientConfig* config);

#endif /* HYDROGEN_CONFIG_MDNS_CLIENT_H */