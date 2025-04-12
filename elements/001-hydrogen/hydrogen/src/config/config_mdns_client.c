/*
 * mDNS Client Configuration Implementation
 *
 * Implements the configuration handlers for the mDNS client subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_mdns_client.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Load mDNS client configuration from JSON
bool load_mdns_client_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->mdns_client = (MDNSClientConfig){
        .enabled = true,
        .enable_ipv6 = false,
        .scan_interval = DEFAULT_MDNS_CLIENT_SCAN_INTERVAL,
        .max_services = DEFAULT_MDNS_CLIENT_MAX_SERVICES,
        .retry_count = DEFAULT_MDNS_CLIENT_RETRY_COUNT,
        .health_check_enabled = true,
        .health_check_interval = DEFAULT_MDNS_CLIENT_HEALTH_CHECK_INTERVAL,
        .service_types = NULL,
        .num_service_types = 0
    };

    // Process all config items in sequence
    bool success = true;

    // Process main section
    success = PROCESS_SECTION(root, "mDNSClient");
    success = success && PROCESS_BOOL(root, &config->mdns_client, enabled, "mDNSClient.Enabled", "mDNSClient");
    success = success && PROCESS_BOOL(root, &config->mdns_client, enable_ipv6, "mDNSClient.EnableIPv6", "mDNSClient");
    success = success && PROCESS_INT(root, &config->mdns_client, scan_interval, "mDNSClient.ScanIntervalMs", "mDNSClient");
    success = success && PROCESS_SIZE(root, &config->mdns_client, max_services, "mDNSClient.MaxServices", "mDNSClient");
    success = success && PROCESS_INT(root, &config->mdns_client, retry_count, "mDNSClient.RetryCount", "mDNSClient");

    // Process health check section
    if (success) {
        success = PROCESS_SECTION(root, "mDNSClient.HealthCheck");
        success = success && PROCESS_BOOL(root, &config->mdns_client, health_check_enabled, "mDNSClient.HealthCheck.Enabled", "mDNSClient");
        success = success && PROCESS_INT(root, &config->mdns_client, health_check_interval, "mDNSClient.HealthCheck.IntervalMs", "mDNSClient");
    }

    // Process service types array
    if (success) {
        json_t* service_types = json_object_get(root, "mDNSClient.ServiceTypes");
        if (json_is_array(service_types)) {
            size_t type_count = json_array_size(service_types);
            if (type_count > 0) {
                config->mdns_client.service_types = calloc(type_count, sizeof(MDNSServiceType));
                if (!config->mdns_client.service_types) {
                    log_this("Config-MDNSClient", "Failed to allocate service types array", LOG_LEVEL_ERROR);
                    return false;
                }
                config->mdns_client.num_service_types = type_count;

                for (size_t i = 0; i < type_count; i++) {
                    json_t* type = json_array_get(service_types, i);
                    if (json_is_string(type)) {
                        const char* type_str = json_string_value(type);
                        config->mdns_client.service_types[i].type = strdup(type_str);
                        if (!config->mdns_client.service_types[i].type) {
                            log_this("Config-MDNSClient", "Failed to allocate service type string", LOG_LEVEL_ERROR);
                            return false;
                        }
                        config->mdns_client.service_types[i].required = true;
                        config->mdns_client.service_types[i].auto_connect = true;
                        log_config_item("ServiceType", type_str, false, "MDNSClient");
                    }
                }
            }
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_mdns_client_validate(&config->mdns_client) == 0);
    }

    return success;
}

// Initialize mDNS client configuration with default values
int config_mdns_client_init(MDNSClientConfig* config) {
    if (!config) {
        log_this("Config-MDNSClient", "mDNS client config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = true;
    config->enable_ipv6 = false;
    config->scan_interval = DEFAULT_MDNS_CLIENT_SCAN_INTERVAL;
    config->max_services = DEFAULT_MDNS_CLIENT_MAX_SERVICES;
    config->retry_count = DEFAULT_MDNS_CLIENT_RETRY_COUNT;
    config->health_check_enabled = true;
    config->health_check_interval = DEFAULT_MDNS_CLIENT_HEALTH_CHECK_INTERVAL;
    config->service_types = NULL;
    config->num_service_types = 0;

    return 0;
}

// Free resources allocated for mDNS client configuration
void config_mdns_client_cleanup(MDNSClientConfig* config) {
    if (!config) {
        return;
    }

    // Free service types array
    if (config->service_types) {
        for (size_t i = 0; i < config->num_service_types; i++) {
            if (config->service_types[i].type) {
                free(config->service_types[i].type);
                config->service_types[i].type = NULL;
            }
        }
        free(config->service_types);
        config->service_types = NULL;
    }
    config->num_service_types = 0;

    // Zero out the structure
    memset(config, 0, sizeof(MDNSClientConfig));
}

// Validate mDNS client configuration values
int config_mdns_client_validate(const MDNSClientConfig* config) {
    if (!config) {
        log_this("Config-MDNSClient", "mDNS client config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate intervals
    if (config->scan_interval <= 0) {
        log_this("Config-MDNSClient", "Invalid scan interval (must be positive)", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->health_check_enabled && config->health_check_interval <= 0) {
        log_this("Config-MDNSClient", "Invalid health check interval (must be positive)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate service types if configured
    if (config->num_service_types > 0 && !config->service_types) {
        log_this("Config-MDNSClient", "Service types array is NULL but count is non-zero", LOG_LEVEL_ERROR);
        return -1;
    }

    for (size_t i = 0; i < config->num_service_types; i++) {
        if (!config->service_types[i].type || !config->service_types[i].type[0]) {
            log_this("Config-MDNSClient", "Invalid service type at index %zu (must not be empty)", LOG_LEVEL_ERROR, i);
            return -1;
        }
    }

    return 0;
}