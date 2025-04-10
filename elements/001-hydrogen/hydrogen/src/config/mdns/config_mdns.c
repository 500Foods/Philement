/*
 * mDNS Server Configuration Implementation
 *
 * This module handles the configuration for the mDNS server component,
 * which is responsible for service discovery and device identification.
 *
 * Design Decisions:
 * - Default values ensure basic device identification
 * - Services array allows dynamic service registration
 * - IPv6 disabled by default for compatibility
 * - Memory management follows strict allocation/cleanup patterns
 */

// Core system headers
#include <sys/types.h>

// Standard C headers
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

// Project headers
#include "config_mdns_server.h"
#include "../types/config_string.h"
#include "../config.h"  // For VERSION
#include "../../logging/logging.h"

int config_mdns_server_init(MDNSServerConfig* config) {
    if (!config) {
        log_this("mDNS", "Null config pointer provided", LOG_LEVEL_ERROR);
        return -1;
    }

    // Initialize basic settings
    config->enabled = 1;  // Enabled by default
    config->enable_ipv6 = 0;  // IPv6 disabled by default
    
    // Initialize strings with default values
    config->device_id = strdup(DEFAULT_MDNS_SERVER_DEVICE_ID);
    config->friendly_name = strdup(DEFAULT_MDNS_SERVER_FRIENDLY_NAME);
    config->model = strdup(DEFAULT_MDNS_SERVER_MODEL);
    config->manufacturer = strdup(DEFAULT_MDNS_SERVER_MANUFACTURER);
    config->version = strdup(VERSION);
    
    // Verify all string allocations succeeded
    if (!config->device_id || !config->friendly_name || 
        !config->model || !config->manufacturer || !config->version) {
        log_this("mDNS", "Failed to allocate configuration strings", LOG_LEVEL_ERROR);
        config_mdns_server_cleanup(config);
        return -1;
    }

    // Initialize services array
    config->services = NULL;
    config->num_services = 0;

    return 0;
}

void config_mdns_server_cleanup(MDNSServerConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->device_id);
    free(config->friendly_name);
    free(config->model);
    free(config->manufacturer);
    free(config->version);

    // Free services array if allocated
    if (config->services) {
        // Services themselves should be freed by the mDNS server
        free(config->services);
    }

    // Zero out the structure
    memset(config, 0, sizeof(MDNSServerConfig));
}

int config_mdns_server_validate(const MDNSServerConfig* config) {
    if (!config) {
        log_this("mDNS", "Null config pointer in validation", LOG_LEVEL_ERROR);
        return -1;
    }

    // If mDNS is enabled, validate required fields
    if (config->enabled) {
        // Check required strings
        if (!config->device_id || !config->friendly_name || 
            !config->model || !config->manufacturer || !config->version) {
            log_this("mDNS", "Missing required configuration strings", LOG_LEVEL_ERROR);
            return -1;
        }

        // Validate string contents
        if (strlen(config->device_id) == 0 || strlen(config->friendly_name) == 0 ||
            strlen(config->model) == 0 || strlen(config->manufacturer) == 0 ||
            strlen(config->version) == 0) {
            log_this("mDNS", "Empty configuration strings not allowed", LOG_LEVEL_ERROR);
            return -1;
        }

        // Services array is optional, but if present must be valid
        if (config->num_services > 0 && !config->services) {
            log_this("mDNS", "Services count > 0 but services array is NULL", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    return 0;
}