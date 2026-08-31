/*
 * mDNS Client Configuration Implementation
 *
 * Implements configuration handlers for the mDNS client subsystem,
 * including JSON parsing, environment variable handling, and cleanup.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_mdns_client.h"

// Load mDNS client configuration from JSON
bool load_mdns_client_config(json_t* root, AppConfig* config) {
    bool success = true;
    MDNSClientConfig* mdns_config = &config->mdns_client;

    // Initialize with defaults
    mdns_config->enable_ipv4 = true;
    mdns_config->enable_ipv6 = false;
    mdns_config->scan_interval = 30;  // seconds
    mdns_config->max_services = 100;
    mdns_config->retry_count = 3;
    mdns_config->health_check_enabled = true;
    mdns_config->health_check_interval = 60;  // milliseconds (JSON IntervalMs)
    mdns_config->health_check_timeout = 1000;
    mdns_config->health_check_retries = 3;
    mdns_config->own_services = true;
    mdns_config->printer_services = false;
    mdns_config->custom_services = NULL;
    mdns_config->num_custom_services = 0;
    mdns_config->service_types = NULL;
    mdns_config->num_service_types = 0;

    // Process main section
    success = PROCESS_SECTION(root, SR_MDNS_CLIENT);
    success = success && PROCESS_BOOL(root, mdns_config, enable_ipv4, SR_MDNS_CLIENT ".EnableIPv4", SR_MDNS_CLIENT);
    success = success && PROCESS_BOOL(root, mdns_config, enable_ipv6, SR_MDNS_CLIENT ".EnableIPv6", SR_MDNS_CLIENT);
    success = success && PROCESS_INT(root, mdns_config, scan_interval, SR_MDNS_CLIENT ".ScanIntervalMs", SR_MDNS_CLIENT);
    success = success && PROCESS_SIZE(root, mdns_config, max_services, SR_MDNS_CLIENT ".MaxServices", SR_MDNS_CLIENT);
    success = success && PROCESS_INT(root, mdns_config, retry_count, SR_MDNS_CLIENT ".RetryCount", SR_MDNS_CLIENT);

    // Process health check section
    if (success) {
        success = PROCESS_SECTION(root, SR_MDNS_CLIENT ".HealthCheck");
        success = success && PROCESS_BOOL(root, mdns_config, health_check_enabled, SR_MDNS_CLIENT ".HealthCheck.Enabled", SR_MDNS_CLIENT);
        success = success && PROCESS_INT(root, mdns_config, health_check_interval, SR_MDNS_CLIENT ".HealthCheck.IntervalMs", SR_MDNS_CLIENT);
        success = success && PROCESS_INT(root, mdns_config, health_check_timeout, SR_MDNS_CLIENT ".HealthCheck.TimeoutMs", SR_MDNS_CLIENT);
        success = success && PROCESS_INT(root, mdns_config, health_check_retries, SR_MDNS_CLIENT ".HealthCheck.RetryCount", SR_MDNS_CLIENT);
    }

    if (success) {
        json_t *mdns_section = json_object_get(root, SR_MDNS_CLIENT);
        json_t *monitored = json_object_get(root, SR_MDNS_CLIENT ".MonitoredServices");
        if (!json_is_object(monitored) && json_is_object(mdns_section)) {
            monitored = json_object_get(mdns_section, "MonitoredServices");
        }
        if (json_is_object(monitored)) {
            json_t *own = json_object_get(monitored, "OwnServices");
            json_t *printer = json_object_get(monitored, "PrinterServices");
            success = PROCESS_SECTION(root, SR_MDNS_CLIENT ".MonitoredServices");
            success = success && PROCESS_BOOL(root, mdns_config, own_services,
                                              SR_MDNS_CLIENT ".MonitoredServices.OwnServices", SR_MDNS_CLIENT);
            success = success && PROCESS_BOOL(root, mdns_config, printer_services,
                                              SR_MDNS_CLIENT ".MonitoredServices.PrinterServices", SR_MDNS_CLIENT);
            if (json_is_boolean(own)) {
                mdns_config->own_services = json_is_true(own);
            }
            if (json_is_boolean(printer)) {
                mdns_config->printer_services = json_is_true(printer);
            }
            json_t *custom = json_object_get(monitored, "CustomServices");
            if (json_is_array(custom)) {
                size_t n = json_array_size(custom);
                if (n > 0) {
                    size_t filled = 0;
                    mdns_config->custom_services = calloc(n, sizeof(char *));
                    if (!mdns_config->custom_services) {
                        return false;
                    }
                    for (size_t i = 0; i < n; i++) {
                        json_t *el = json_array_get(custom, i);
                        if (json_is_string(el)) {
                            mdns_config->custom_services[filled] = strdup(json_string_value(el));
                            if (!mdns_config->custom_services[filled]) {
                                success = false;
                                break;
                            }
                            filled++;
                        }
                    }
                    mdns_config->num_custom_services = filled;
                }
            }
            if (json_object_get(monitored, "LoadBalancers") != NULL) {
                log_this(SR_CONFIG, "MonitoredServices.LoadBalancers ignored", LOG_LEVEL_DEBUG, 0);
            }
        }
    }

    // Process service types array
    if (success) {
        json_t* service_types = json_object_get(root, SR_MDNS_CLIENT ".ServiceTypes");
        if (!json_is_array(service_types)) {
            json_t* mdns_section = json_object_get(root, SR_MDNS_CLIENT);
            if (json_is_object(mdns_section)) {
                service_types = json_object_get(mdns_section, "ServiceTypes");
            }
        }
        if (json_is_array(service_types)) {
            size_t type_count = json_array_size(service_types);
            log_this(SR_CONFIG, "――― Service Types: %zu configured", LOG_LEVEL_DEBUG, 1, type_count);

            if (type_count > 0) {
                mdns_config->service_types = calloc(type_count, sizeof(MDNSServiceType));
                if (!mdns_config->service_types) {
                    log_this(SR_CONFIG, "Failed to allocate service types array", LOG_LEVEL_ERROR, 0);
                    return false;
                }

                size_t filled = 0;
                for (size_t i = 0; i < type_count; i++) {
                    json_t* type = json_array_get(service_types, i);
                    MDNSServiceType *slot = &mdns_config->service_types[filled];

                    if (json_is_string(type)) {
                        slot->type = strdup(json_string_value(type));
                        if (!slot->type) {
                            success = false;
                            break;
                        }
                        slot->required = 0;
                        slot->auto_connect = 0;
                        log_this(SR_CONFIG, "――――― Type: %s", LOG_LEVEL_DEBUG, 1, slot->type);
                        filled++;
                        continue;
                    }
                    if (!json_is_object(type)) {
                        continue;
                    }

                    json_t* type_str = json_object_get(type, "Type");
                    if (json_is_string(type_str)) {
                        slot->type = strdup(json_string_value(type_str));
                        if (!slot->type) {
                            success = false;
                            break;
                        }
                        log_this(SR_CONFIG, "――――― Type: %s", LOG_LEVEL_DEBUG, 1, slot->type);
                    } else {
                        slot->type = strdup("_http._tcp.local");
                        log_this(SR_CONFIG, "――――― Type: %s (*)", LOG_LEVEL_DEBUG, 1, slot->type);
                    }

                    json_t* required = json_object_get(type, "Required");
                    slot->required = json_is_true(required);
                    log_this(SR_CONFIG, "――――― Required: %s", LOG_LEVEL_DEBUG, 1, slot->required ? "true" : "false");

                    json_t* auto_connect = json_object_get(type, "AutoConnect");
                    slot->auto_connect = json_is_true(auto_connect);
                    log_this(SR_CONFIG, "――――― AutoConnect: %s", LOG_LEVEL_DEBUG, 1, slot->auto_connect ? "true" : "false");
                    filled++;
                }
                mdns_config->num_service_types = filled;
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
    if (config->custom_services) {
        for (size_t i = 0; i < config->num_custom_services; i++) {
            free(config->custom_services[i]);
        }
        free(config->custom_services);
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
    DUMP_BOOL2("――", "IPv4 Enabled", config->enable_ipv4);
    DUMP_BOOL2("――", "IPv6 Enabled", config->enable_ipv6);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Scan Interval: %d ms", config->scan_interval);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Max Services: %zu", config->max_services);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Retry Count: %d", config->retry_count);
    DUMP_TEXT("――", buffer);

    // Dump health check configuration
    DUMP_BOOL2("――", "Health Check Enabled", config->health_check_enabled);
    snprintf(buffer, sizeof(buffer), "Health Check Interval: %d ms", config->health_check_interval);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Health Check Timeout: %d ms", config->health_check_timeout);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Health Check Retries: %d", config->health_check_retries);
    DUMP_TEXT("――", buffer);
    DUMP_BOOL2("――", "Own Services", config->own_services);
    DUMP_BOOL2("――", "Printer Services", config->printer_services);
    snprintf(buffer, sizeof(buffer), "Custom Services: %zu", config->num_custom_services);
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
