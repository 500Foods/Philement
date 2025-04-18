/*
 * mDNS Server Configuration Implementation
 *
 * Implements configuration handlers for the mDNS server subsystem,
 * including JSON parsing, environment variable handling, and cleanup.
 */

#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "config_mdns_server.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"
#include "../utils/utils.h"

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

// Helper function to process a service's TXT records
static bool process_txt_records(json_t* txt_records, mdns_server_service_t* service) {
    if (json_is_string(txt_records)) {
        // Single TXT record
        service->num_txt_records = 1;
        service->txt_records = malloc(sizeof(char*));
        if (!service->txt_records) {
            log_this("Config-MDNSServer", "Failed to allocate memory for TXT record", LOG_LEVEL_ERROR);
            return false;
        }
        service->txt_records[0] = strdup(json_string_value(txt_records));
        return (service->txt_records[0] != NULL);
    } 
    else if (json_is_array(txt_records)) {
        // Array of TXT records
        service->num_txt_records = json_array_size(txt_records);
        service->txt_records = malloc(service->num_txt_records * sizeof(char*));
        if (!service->txt_records) {
            log_this("Config-MDNSServer", "Failed to allocate memory for TXT records array", LOG_LEVEL_ERROR);
            return false;
        }

        for (size_t j = 0; j < service->num_txt_records; j++) {
            json_t* record = json_array_get(txt_records, j);
            service->txt_records[j] = json_is_string(record) ?
                strdup(json_string_value(record)) : strdup("");
            if (!service->txt_records[j]) return false;
        }
        return true;
    }

    // No TXT records
    service->num_txt_records = 0;
    service->txt_records = NULL;
    return true;
}

// Load mDNS server configuration from JSON
bool load_mdns_server_config(json_t* root, AppConfig* config) {
    bool success = true;
    MDNSServerConfig* mdns_config = &config->mdns_server;

    // Initialize with defaults
    mdns_config->enabled = true;
    mdns_config->enable_ipv6 = false;
    mdns_config->device_id = strdup("hydrogen");
    mdns_config->friendly_name = strdup("Hydrogen Server");
    mdns_config->model = strdup("Hydrogen");
    mdns_config->manufacturer = strdup("Philement");
    mdns_config->version = strdup(VERSION);
    mdns_config->services = NULL;
    mdns_config->num_services = 0;

    // Process main section
    success = PROCESS_SECTION(root, "mDNSServer");
    
    // Process basic fields
    success = success && PROCESS_BOOL(root, mdns_config, enabled, "mDNSServer.Enabled", "MDNSServer");
    success = success && PROCESS_BOOL(root, mdns_config, enable_ipv6, "mDNSServer.EnableIPv6", "MDNSServer");
    success = success && PROCESS_STRING(root, mdns_config, device_id, "mDNSServer.DeviceId", "MDNSServer");
    success = success && PROCESS_STRING(root, mdns_config, friendly_name, "mDNSServer.FriendlyName", "MDNSServer");
    success = success && PROCESS_STRING(root, mdns_config, model, "mDNSServer.Model", "MDNSServer");
    success = success && PROCESS_STRING(root, mdns_config, manufacturer, "mDNSServer.Manufacturer", "MDNSServer");
    success = success && PROCESS_STRING(root, mdns_config, version, "mDNSServer.Version", "MDNSServer");

    // Process services array if present
    if (success) {
        json_t* services = json_object_get(root, "mDNSServer.Services");
        if (json_is_array(services)) {
            mdns_config->num_services = json_array_size(services);
            log_this("Config-MDNSServer", "――― Services: %zu configured", LOG_LEVEL_STATE, mdns_config->num_services);

            if (mdns_config->num_services > 0) {
                mdns_config->services = calloc(mdns_config->num_services, sizeof(mdns_server_service_t));
                if (!mdns_config->services) {
                    log_this("Config-MDNSServer", "Failed to allocate memory for mDNS services array", LOG_LEVEL_ERROR);
                    cleanup_mdns_server_config(mdns_config);
                    return false;
                }

                // Process each service
                for (size_t i = 0; i < mdns_config->num_services; i++) {
                    json_t* service = json_array_get(services, i);
                    if (!json_is_object(service)) continue;

                    // Process service properties
                    char service_path[128];
                    snprintf(service_path, sizeof(service_path), "mDNSServer.Services[%zu]", i);

                    // Process name
                    json_t* name = json_object_get(service, "Name");
                    if (json_is_string(name)) {
                        mdns_config->services[i].name = strdup(json_string_value(name));
                        if (!mdns_config->services[i].name) {
                            success = false;
                            break;
                        }
                        log_this("Config-MDNSServer", "――― Service[%zu].Name: %s", LOG_LEVEL_STATE, 
                                i, mdns_config->services[i].name);
                    } else {
                        mdns_config->services[i].name = strdup("hydrogen");
                        log_this("Config-MDNSServer", "――― Service[%zu].Name: %s (*)", LOG_LEVEL_STATE, 
                                i, mdns_config->services[i].name);
                    }

                    // Process type
                    json_t* type = json_object_get(service, "Type");
                    if (json_is_string(type)) {
                        mdns_config->services[i].type = strdup(json_string_value(type));
                        if (!mdns_config->services[i].type) {
                            success = false;
                            break;
                        }
                        log_this("Config-MDNSServer", "――― Service[%zu].Type: %s", LOG_LEVEL_STATE, 
                                i, mdns_config->services[i].type);
                    } else {
                        mdns_config->services[i].type = strdup("_http._tcp.local");
                        log_this("Config-MDNSServer", "――― Service[%zu].Type: %s (*)", LOG_LEVEL_STATE, 
                                i, mdns_config->services[i].type);
                    }

                    // Process port
                    json_t* port = json_object_get(service, "Port");
                    if (json_is_integer(port)) {
                        mdns_config->services[i].port = json_integer_value(port);
                        log_this("Config-MDNSServer", "――― Service[%zu].Port: %d", LOG_LEVEL_STATE, 
                                i, mdns_config->services[i].port);
                    } else {
                        mdns_config->services[i].port = 80;
                        log_this("Config-MDNSServer", "――― Service[%zu].Port: %d (*)", LOG_LEVEL_STATE, 
                                i, mdns_config->services[i].port);
                    }

                    // Process TXT records
                    json_t* txt_records = json_object_get(service, "TxtRecords");
                    if (txt_records && !process_txt_records(txt_records, &mdns_config->services[i])) {
                        success = false;
                        break;
                    }
                }
            }
        }
    }

    if (!success) {
        cleanup_mdns_server_config(mdns_config);
    }

    return success;
}

// Clean up mDNS server configuration
void cleanup_mdns_server_config(MDNSServerConfig* config) {
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

// Dump mDNS server configuration
void dump_mdns_server_config(const MDNSServerConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL mDNS server config");
        return;
    }

    // Dump basic configuration
    DUMP_BOOL2("――", "Enabled", config->enabled);
    DUMP_BOOL2("――", "IPv6 Enabled", config->enable_ipv6);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Device ID: %s", config->device_id);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Friendly Name: %s", config->friendly_name);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Model: %s", config->model);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Manufacturer: %s", config->manufacturer);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Version: %s", config->version);
    DUMP_TEXT("――", buffer);

    // Dump services
    snprintf(buffer, sizeof(buffer), "Services (%zu)", config->num_services);
    DUMP_TEXT("――", buffer);

    // Dump each service
    for (size_t i = 0; i < config->num_services; i++) {
        const mdns_server_service_t* service = &config->services[i];
        
        snprintf(buffer, sizeof(buffer), "Service %zu", i + 1);
        DUMP_TEXT("――――", buffer);
        
        snprintf(buffer, sizeof(buffer), "Name: %s", service->name);
        DUMP_TEXT("――――――", buffer);
        snprintf(buffer, sizeof(buffer), "Type: %s", service->type);
        DUMP_TEXT("――――――", buffer);
        snprintf(buffer, sizeof(buffer), "Port: %d", service->port);
        DUMP_TEXT("――――――", buffer);

        if (service->num_txt_records > 0) {
            snprintf(buffer, sizeof(buffer), "TXT Records (%zu)", service->num_txt_records);
            DUMP_TEXT("――――――", buffer);
            
            for (size_t j = 0; j < service->num_txt_records; j++) {
                snprintf(buffer, sizeof(buffer), "Record: %s", service->txt_records[j]);
                DUMP_TEXT("――――――――", buffer);
            }
        }
    }
}