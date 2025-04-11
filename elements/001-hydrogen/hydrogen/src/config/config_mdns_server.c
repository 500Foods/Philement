/*
 * mDNS Server Configuration Implementation
 *
 * Implements the configuration handlers for the mDNS server subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "config_mdns_server.h"  // Must be first for mdns_server_service_t definition
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Forward declarations for static helper functions
static void cleanup_service(mdns_server_service_t* service);
static void cleanup_services(mdns_server_service_t* services, size_t count);

// Load mDNS server configuration from JSON
bool load_mdns_server_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->mdns_server = (MDNSServerConfig){
        .enabled = true,
        .enable_ipv6 = false,
        .device_id = NULL,
        .friendly_name = NULL,
        .model = NULL,
        .manufacturer = NULL,
        .version = NULL,
        .services = NULL,
        .num_services = 0
    };

    // Process all config items in sequence
    bool success = true;

    // Process main section
    success = PROCESS_SECTION(root, "mDNSServer");
    success = success && PROCESS_BOOL(root, &config->mdns_server, enabled, "mDNSServer.Enabled", "mDNSServer");
    success = success && PROCESS_BOOL(root, &config->mdns_server, enable_ipv6, "mDNSServer.EnableIPv6", "mDNSServer");
    success = success && PROCESS_STRING(root, &config->mdns_server, device_id, "mDNSServer.DeviceId", "mDNSServer");
    success = success && PROCESS_STRING(root, &config->mdns_server, friendly_name, "mDNSServer.FriendlyName", "mDNSServer");
    success = success && PROCESS_STRING(root, &config->mdns_server, model, "mDNSServer.Model", "mDNSServer");
    success = success && PROCESS_STRING(root, &config->mdns_server, manufacturer, "mDNSServer.Manufacturer", "mDNSServer");
    success = success && PROCESS_STRING(root, &config->mdns_server, version, "mDNSServer.Version", "mDNSServer");

    // Process services array if present
    if (success) {
        json_t* services = json_object_get(root, "mDNSServer.Services");
        if (json_is_array(services)) {
            config->mdns_server.num_services = json_array_size(services);
            char services_buffer[64];
            snprintf(services_buffer, sizeof(services_buffer), "%zu configured", 
                    config->mdns_server.num_services);
            log_config_item("Services", services_buffer, false);

            if (config->mdns_server.num_services > 0) {
                config->mdns_server.services = calloc(config->mdns_server.num_services, 
                                                    sizeof(mdns_server_service_t));
                if (!config->mdns_server.services) {
                    log_this("Config", "Failed to allocate memory for mDNS services array", LOG_LEVEL_ERROR);
                    config_mdns_server_cleanup(&config->mdns_server);
                    return false;
                }

                // Process each service
                for (size_t i = 0; i < config->mdns_server.num_services; i++) {
                    json_t* service = json_array_get(services, i);
                    if (!json_is_object(service)) {
                        continue;
                    }

                    // Get service properties
                    json_t* val;
                    val = json_object_get(service, "Name");
                    config->mdns_server.services[i].name = json_is_string(val) ? 
                        strdup(json_string_value(val)) : strdup("hydrogen");

                    val = json_object_get(service, "Type");
                    config->mdns_server.services[i].type = json_is_string(val) ? 
                        strdup(json_string_value(val)) : strdup("_http._tcp.local");

                    val = json_object_get(service, "Port");
                    config->mdns_server.services[i].port = json_is_integer(val) ? 
                        json_integer_value(val) : DEFAULT_WEB_PORT;

                    // Log service details
                    char service_buffer[512];
                    snprintf(service_buffer, sizeof(service_buffer), "%s: %s on port %d",
                            config->mdns_server.services[i].name,
                            config->mdns_server.services[i].type,
                            config->mdns_server.services[i].port);
                    log_config_item("Service", service_buffer, false);

                    // Handle TXT records
                    json_t* txt_records = json_object_get(service, "TxtRecords");
                    if (json_is_string(txt_records)) {
                        // Single TXT record
                        config->mdns_server.services[i].num_txt_records = 1;
                        config->mdns_server.services[i].txt_records = malloc(sizeof(char*));
                        if (!config->mdns_server.services[i].txt_records) {
                            log_this("Config", "Failed to allocate memory for TXT record", LOG_LEVEL_ERROR);
                            cleanup_service(&config->mdns_server.services[i]);
                            cleanup_services(config->mdns_server.services, i);
                            config_mdns_server_cleanup(&config->mdns_server);
                            return false;
                        }
                        config->mdns_server.services[i].txt_records[0] = strdup(json_string_value(txt_records));
                    } else if (json_is_array(txt_records)) {
                        // Array of TXT records
                        config->mdns_server.services[i].num_txt_records = json_array_size(txt_records);
                        config->mdns_server.services[i].txt_records = malloc(
                            config->mdns_server.services[i].num_txt_records * sizeof(char*));
                        if (!config->mdns_server.services[i].txt_records) {
                            log_this("Config", "Failed to allocate memory for TXT records array", LOG_LEVEL_ERROR);
                            cleanup_service(&config->mdns_server.services[i]);
                            cleanup_services(config->mdns_server.services, i);
                            config_mdns_server_cleanup(&config->mdns_server);
                            return false;
                        }

                        for (size_t j = 0; j < config->mdns_server.services[i].num_txt_records; j++) {
                            json_t* record = json_array_get(txt_records, j);
                            config->mdns_server.services[i].txt_records[j] = json_is_string(record) ?
                                strdup(json_string_value(record)) : strdup("");
                        }
                    } else {
                        config->mdns_server.services[i].num_txt_records = 0;
                        config->mdns_server.services[i].txt_records = NULL;
                    }
                }
            }
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_mdns_server_validate(&config->mdns_server) == 0);
    }

    return success;
}

// Initialize mDNS server configuration with default values
int config_mdns_server_init(MDNSServerConfig* config) {
    if (!config) {
        log_this("Config", "mDNS server config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = true;
    config->enable_ipv6 = false;

    // Allocate and set default strings
    config->device_id = strdup(DEFAULT_MDNS_SERVER_DEVICE_ID);
    config->friendly_name = strdup(DEFAULT_MDNS_SERVER_FRIENDLY_NAME);
    config->model = strdup(DEFAULT_MDNS_SERVER_MODEL);
    config->manufacturer = strdup(DEFAULT_MDNS_SERVER_MANUFACTURER);
    config->version = strdup(VERSION);

    // Check string allocations
    if (!config->device_id || !config->friendly_name || !config->model || 
        !config->manufacturer || !config->version) {
        log_this("Config", "Failed to allocate memory for mDNS server strings", LOG_LEVEL_ERROR);
        config_mdns_server_cleanup(config);
        return -1;
    }

    // Initialize services array
    config->services = NULL;
    config->num_services = 0;

    return 0;
}

// Helper function to cleanup a single service
static void cleanup_service(mdns_server_service_t* service) {
    if (!service) return;

    free(service->name);
    free(service->type);
    
    if (service->txt_records) {
        for (size_t i = 0; i < service->num_txt_records; i++) {
            free(service->txt_records[i]);
        }
        free(service->txt_records);
    }
}

// Helper function to cleanup services array up to index
static void cleanup_services(mdns_server_service_t* services, size_t count) {
    if (!services) return;

    for (size_t i = 0; i < count; i++) {
        cleanup_service(&services[i]);
    }
    free(services);
}

// Free resources allocated for mDNS server configuration
void config_mdns_server_cleanup(MDNSServerConfig* config) {
    if (!config) return;

    // Free string fields
    free(config->device_id);
    free(config->friendly_name);
    free(config->model);
    free(config->manufacturer);
    free(config->version);

    // Cleanup services array
    if (config->services) {
        cleanup_services(config->services, config->num_services);
    }

    // Zero out the structure
    memset(config, 0, sizeof(MDNSServerConfig));
}

// Validate mDNS server configuration values
int config_mdns_server_validate(const MDNSServerConfig* config) {
    if (!config) {
        log_this("Config", "mDNS server config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate required strings if enabled
    if (config->enabled) {
        if (!config->device_id || !config->device_id[0]) {
            log_this("Config", "Device ID is required when mDNS server is enabled", LOG_LEVEL_ERROR);
            return -1;
        }

        if (!config->friendly_name || !config->friendly_name[0]) {
            log_this("Config", "Friendly name is required when mDNS server is enabled", LOG_LEVEL_ERROR);
            return -1;
        }

        if (!config->model || !config->model[0]) {
            log_this("Config", "Model is required when mDNS server is enabled", LOG_LEVEL_ERROR);
            return -1;
        }

        if (!config->manufacturer || !config->manufacturer[0]) {
            log_this("Config", "Manufacturer is required when mDNS server is enabled", LOG_LEVEL_ERROR);
            return -1;
        }

        if (!config->version || !config->version[0]) {
            log_this("Config", "Version is required when mDNS server is enabled", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    // Validate services array if present
    if (config->num_services > 0) {
        if (!config->services) {
            log_this("Config", "Services array is NULL but count is non-zero", LOG_LEVEL_ERROR);
            return -1;
        }

        // Validate each service
        for (size_t i = 0; i < config->num_services; i++) {
            if (!config->services[i].name || !config->services[i].name[0]) {
                log_this("Config", "Service name is required", LOG_LEVEL_ERROR);
                return -1;
            }

            if (!config->services[i].type || !config->services[i].type[0]) {
                log_this("Config", "Service type is required", LOG_LEVEL_ERROR);
                return -1;
            }

            if (config->services[i].port <= 0 || config->services[i].port > 65535) {
                log_this("Config", "Invalid service port number", LOG_LEVEL_ERROR);
                return -1;
            }

            // Validate TXT records if present
            if (config->services[i].num_txt_records > 0 && !config->services[i].txt_records) {
                log_this("Config", "TXT records array is NULL but count is non-zero", LOG_LEVEL_ERROR);
                return -1;
            }
        }
    }

    return 0;
}