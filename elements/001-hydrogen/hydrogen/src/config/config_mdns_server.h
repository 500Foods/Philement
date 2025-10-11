/*
 * mDNS Server Configuration
 *
 * Defines the configuration structure and handlers for the mDNS server subsystem.
 * This includes settings for:
 * - Device identification
 * - Service advertisement
 * - Network protocols
 */

#ifndef HYDROGEN_CONFIG_MDNS_SERVER_H
#define HYDROGEN_CONFIG_MDNS_SERVER_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"
#include <src/mdns/mdns_server.h>  // For mdns_server_service_t

// mDNS server configuration structure
typedef struct MDNSServerConfig {
    bool enable_ipv4;                    // Whether mDNS server is enabled
    bool enable_ipv6;               // Whether IPv6 is enabled
    char* device_id;                // Device identifier
    char* friendly_name;            // Human-readable device name
    char* model;                    // Device model
    char* manufacturer;             // Device manufacturer
    char* version;                  // Software version
    int retry_count;                // Number of consecutive failures before disabling interface
    mdns_server_service_t* services;// Array of services to advertise
    size_t num_services;           // Number of services in array
} MDNSServerConfig;

/*
 * Load mDNS server configuration from JSON
 *
 * This function loads the mDNS server configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_mdns_server_config(json_t* root, AppConfig* config);

/*
 * Dump mDNS server configuration for debugging
 *
 * This function outputs the current mDNS server configuration settings
 * in a structured format with proper indentation.
 *
 * @param config Pointer to MDNSServerConfig structure to dump
 */
void dump_mdns_server_config(const MDNSServerConfig* config);

/*
 * Clean up mDNS server configuration
 *
 * This function frees all resources allocated for the mDNS server configuration.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to MDNSServerConfig structure to cleanup
 */
void cleanup_mdns_server_config(MDNSServerConfig* config);

#endif /* HYDROGEN_CONFIG_MDNS_SERVER_H */
