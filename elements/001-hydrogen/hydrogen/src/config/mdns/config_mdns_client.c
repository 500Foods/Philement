/*
 * mDNS Client Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "../config.h"
#include "config_mdns_client.h"
#include "../config_utils.h"
#include "../../logging/logging.h"

int config_mdns_client_init(struct MDNSClientConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default values
    config->enabled = 0;  // Disabled by default for security
    config->enable_ipv6 = 0;  // IPv6 disabled by default
    config->scan_interval = DEFAULT_MDNS_CLIENT_SCAN_INTERVAL;
    config->max_services = DEFAULT_MDNS_CLIENT_MAX_SERVICES;
    config->retry_count = DEFAULT_MDNS_CLIENT_RETRY_COUNT;
    config->health_check_enabled = 1;  // Enable health checks by default
    config->health_check_interval = DEFAULT_MDNS_CLIENT_HEALTH_CHECK_INTERVAL;

    // Initialize service types array
    config->service_types = NULL;
    config->num_service_types = 0;

    return 0;
}

void config_mdns_client_cleanup(struct MDNSClientConfig* config) {
    if (!config) {
        return;
    }

    // Free service types array if allocated
    if (config->service_types) {
        for (size_t i = 0; i < config->num_service_types; i++) {
            free(config->service_types[i].type);
        }
        free(config->service_types);
    }

    // Zero out the structure to prevent use after free
    memset(config, 0, sizeof(struct MDNSClientConfig));
}

int config_mdns_client_validate(const struct MDNSClientConfig* config) {
    if (!config) {
        return -1;
    }

    // Skip validation if mDNS client is disabled
    if (!config->enabled) {
        return 0;
    }

    // Validate scan interval
    if (config->scan_interval < 5 || config->scan_interval > 3600) {
        log_this("Config", "Invalid mDNS client scan interval", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate max services
    if (config->max_services < 1 || config->max_services > 1000) {
        log_this("Config", "Invalid mDNS client max services", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate retry count
    if (config->retry_count < 1 || config->retry_count > 10) {
        log_this("Config", "Invalid mDNS client retry count", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate health check interval if enabled
    if (config->health_check_enabled) {
        if (config->health_check_interval < 10 || config->health_check_interval > 3600) {
            log_this("Config", "Invalid mDNS client health check interval", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    // Validate service types if any are configured
    if (config->num_service_types > 0) {
        if (!config->service_types) {
            log_this("Config", "Invalid mDNS client service types configuration", LOG_LEVEL_ERROR);
            return -1;
        }

        // Validate each service type
        for (size_t i = 0; i < config->num_service_types; i++) {
            if (!config->service_types[i].type || strlen(config->service_types[i].type) == 0) {
                log_this("Config", "Invalid mDNS client service type entry", LOG_LEVEL_ERROR);
                return -1;
            }
        }
    }

    return 0;
}