/*
 * mDNS Client Configuration Implementation
 *
 * Implements configuration handlers for the mDNS client subsystem,
 * including JSON parsing, environment variable handling, and cleanup.
 */

#include <stdlib.h>
#include <string.h>
#include "config_mdns_client.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Load mDNS client configuration from JSON
bool load_mdns_client_config(json_t* root, AppConfig* config) {
    bool success = true;
    MDNSClientConfig* mdns_config = &config->mdns_client;

    // Initialize with defaults
    mdns_config->enabled = true;
    mdns_config->enable_ipv6 = false;
    mdns_config->scan_interval = 30;  // seconds
    mdns_config->max_services = 100;
    mdns_config->retry_count = 3;
    mdns_config->health_check_enabled = true;
    mdns_config->health_check_interval = 60;  // seconds
    mdns_config->service_types = NULL;
    mdns_config->num_service_types = 0;

    // Process main section
    success = PROCESS_SECTION(root, "mDNSClient");
    success = success && PROCESS_BOOL(root, mdns_config, enabled, "mDNSClient.Enabled", "MDNSClient");
    success = success && PROCESS_BOOL(root, mdns_config, enable_ipv6, "mDNSClient.EnableIPv6", "MDNSClient");
    success = success && PROCESS_INT(root, mdns_config, scan_interval, "mDNSClient.ScanIntervalMs", "MDNSClient");
    success = success && PROCESS_SIZE(root, mdns_config, max_services, "mDNSClient.MaxServices", "MDNSClient");
    success = success && PROCESS_INT(root, mdns_config, retry_count, "mDNSClient.RetryCount", "MDNSClient");

    // Process health check section
    if (success) {
        success = PROCESS_SECTION(root, "mDNSClient.HealthCheck");
        success = success && PROCESS_BOOL(root, mdns_config, health_check_enabled, "mDNSClient.HealthCheck.Enabled", "MDNSClient");
        success = success && PROCESS_INT(root, mdns_config, health_check_interval, "mDNSClient.HealthCheck.IntervalMs", "MDNSClient");
    }

    // Process service types array
    if (success) {
        json_t* service_types = json_object_get(root, "mDNSClient.ServiceTypes");
        if (json_is_array(service_types)) {
            size_t type_count = json_array_size(service_types);
            log_this("Config-MDNSClient", "――― Service Types: %zu configured", LOG_LEVEL_STATE, type_count);

            if (type_count > 0) {
                mdns_config->service_types = calloc(type_count, sizeof(MDNSServiceType));
                if (!mdns_config->service_types) {
                    log_this("Config-MDNSClient", "Failed to allocate service types array", LOG_LEVEL_ERROR);
                    return false;
                }
                mdns_config->num_service_types = type_count;

                for (size_t i = 0; i < type_count; i++) {
                    json_t* type = json_array_get(service_types, i);
                    if (!json_is_object(type)) continue;

                    // Process type string
                    json_t* type_str = json_object_get(type, "Type");
                    if (json_is_string(type_str)) {
                        mdns_config->service_types[i].type = strdup(json_string_value(type_str));
                        if (!mdns_config->service_types[i].type) {
                            success = false;
                            break;
                        }
                        log_this("Config-MDNSClient", "――――― Type: %s", LOG_LEVEL_STATE, 
                                mdns_config->service_types[i].type);
                    } else {
                        mdns_config->service_types[i].type = strdup("_http._tcp.local");
                        log_this("Config-MDNSClient", "――――― Type: %s (*)", LOG_LEVEL_STATE, 
                                mdns_config->service_types[i].type);
                    }

                    // Process required flag
                    json_t* required = json_object_get(type, "Required");
                    mdns_config->service_types[i].required = json_is_true(required);
                    log_this("Config-MDNSClient", "――――― Required: %s", LOG_LEVEL_STATE,
                            mdns_config->service_types[i].required ? "true" : "false");

                    // Process auto_connect flag
                    json_t* auto_connect = json_object_get(type, "AutoConnect");
                    mdns_config->service_types[i].auto_connect = json_is_true(auto_connect);
                    log_this("Config-MDNSClient", "――――― AutoConnect: %s", LOG_LEVEL_STATE,
                            mdns_config->service_types[i].auto_connect ? "true" : "false");
                }
            }
        }
    }

    if (!success) {
        cleanup_mdns_client_config(mdns_config);
    }

    return success;
}

// Clean up mDNS client configuration
void cleanup_mdns_client_config(MDNSClientConfig* config) {
    if (!config) return;

    // Free service types array
    if (config->service_types) {
        for (size_t i = 0; i < config->num_service_types; i++) {
            free(config->service_types[i].type);
        }
        free(config->service_types);
    }

    // Zero out the structure
    memset(config, 0, sizeof(MDNSClientConfig));
}

// Dump mDNS client configuration
void dump_mdns_client_config(const MDNSClientConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL mDNS client config");
        return;
    }

    // Dump basic configuration
    DUMP_BOOL2("――", "Enabled", config->enabled);
    DUMP_BOOL2("――", "IPv6 Enabled", config->enable_ipv6);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Scan Interval: %d seconds", config->scan_interval);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Max Services: %zu", config->max_services);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Retry Count: %d", config->retry_count);
    DUMP_TEXT("――", buffer);

    // Dump health check configuration
    DUMP_BOOL2("――", "Health Check Enabled", config->health_check_enabled);
    snprintf(buffer, sizeof(buffer), "Health Check Interval: %d seconds", config->health_check_interval);
    DUMP_TEXT("――", buffer);

    // Dump service types
    snprintf(buffer, sizeof(buffer), "Service Types (%zu)", config->num_service_types);
    DUMP_TEXT("――", buffer);

    for (size_t i = 0; i < config->num_service_types; i++) {
        const MDNSServiceType* service = &config->service_types[i];
        
        snprintf(buffer, sizeof(buffer), "Service Type %zu", i + 1);
        DUMP_TEXT("――――", buffer);
        
        snprintf(buffer, sizeof(buffer), "Type: %s", service->type);
        DUMP_TEXT("――――――", buffer);
        snprintf(buffer, sizeof(buffer), "Required: %s", service->required ? "true" : "false");
        DUMP_TEXT("――――――", buffer);
        snprintf(buffer, sizeof(buffer), "Auto Connect: %s", service->auto_connect ? "true" : "false");
        DUMP_TEXT("――――――", buffer);
    }
}