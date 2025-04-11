/*
 * mDNS Client configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include <string.h>
#include "../../logging/logging.h"
#include "../config.h"
#include "../config_utils.h"
#include "json_mdns_client.h"

bool load_json_mdns_client(json_t* root, AppConfig* config __attribute__((unused))) {
    // mDNS Client Configuration
    json_t* mdns_client = json_object_get(root, "mDNSClient");
    bool using_defaults = !json_is_object(mdns_client);
    
    log_config_section("mDNSClient", using_defaults);
    
    if (!using_defaults) {
        json_t* enabled = json_object_get(mdns_client, "Enabled");
        bool mdns_client_enabled = get_config_bool(enabled, true);
        log_config_item("Enabled", mdns_client_enabled ? "true" : "false", !enabled, 0);
        
        json_t* enable_ipv6 = json_object_get(mdns_client, "EnableIPv6");
        bool mdns_client_ipv6 = get_config_bool(enable_ipv6, true);
        log_config_item("EnableIPv6", mdns_client_ipv6 ? "true" : "false", !enable_ipv6, 0);
        
        json_t* scan_interval = json_object_get(mdns_client, "ScanIntervalMs");
        if (json_is_integer(scan_interval)) {
            char interval_buffer[64];
            snprintf(interval_buffer, sizeof(interval_buffer), "%sms",
                    format_int_buffer(json_integer_value(scan_interval)));
            log_config_item("ScanInterval", interval_buffer, false, 0);
        }
        
        json_t* service_types = json_object_get(mdns_client, "ServiceTypes");
        if (json_is_array(service_types)) {
            size_t type_count = json_array_size(service_types);
            char count_buffer[64];
            snprintf(count_buffer, sizeof(count_buffer), "%s Configured",
                    format_int_buffer(type_count));
            log_config_item("ServiceTypes", count_buffer, false, 0);
            
            for (size_t i = 0; i < type_count; i++) {
                json_t* type = json_array_get(service_types, i);
                if (json_is_string(type)) {
                    log_config_item("Type", json_string_value(type), false, 1);
                }
            }
        }
        
        json_t* health_check = json_object_get(mdns_client, "HealthCheck");
        if (json_is_object(health_check)) {
            log_config_item("HealthCheck", "Configured", false, 0);
            
            json_t* health_enabled = json_object_get(health_check, "Enabled");
            bool health_check_enabled = get_config_bool(health_enabled, true);
            log_config_item("Enabled", health_check_enabled ? "true" : "false", !health_enabled, 1);
            
            json_t* interval = json_object_get(health_check, "IntervalMs");
            if (json_is_integer(interval)) {
                char interval_buffer[64];
                snprintf(interval_buffer, sizeof(interval_buffer), "%sms",
                        format_int_buffer(json_integer_value(interval)));
                log_config_item("Interval", interval_buffer, false, 1);
            }
        }
    } else {
        log_config_item("Status", "Section missing, using defaults", true, 0);
    }
    
    // Always return true as there's no failure condition here
    return true;
}