/*
 * mDNS Client Configuration
 *
 * Defines the configuration structure and handlers for the mDNS client subsystem.
 * This includes settings for:
 * - Service discovery
 * - Network scanning
 * - Health checks
 * - Auto-configuration
 */

#ifndef HYDROGEN_CONFIG_MDNS_CLIENT_H
#define HYDROGEN_CONFIG_MDNS_CLIENT_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"

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
 * Dump mDNS client configuration for debugging
 *
 * This function outputs the current mDNS client configuration settings
 * in a structured format with proper indentation.
 *
 * @param config Pointer to MDNSClientConfig structure to dump
 */
void dump_mdns_client_config(const MDNSClientConfig* config);

/*
 * Clean up mDNS client configuration
 *
 * This function frees all resources allocated for the mDNS client configuration.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to MDNSClientConfig structure to cleanup
 */
void cleanup_mdns_client_config(MDNSClientConfig* config);

#endif /* HYDROGEN_CONFIG_MDNS_CLIENT_H */
