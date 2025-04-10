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
#include "../logging/config_logging_utils.h"

bool load_json_mdns_client(json_t* root, AppConfig* config __attribute__((unused))) {
    // mDNS Client Configuration
    json_t* mdns_client = json_object_get(root, "mDNSClient");
    if (json_is_object(mdns_client)) {
        log_config_section_header("mDNSClient");
        
        json_t* enabled = json_object_get(mdns_client, "Enabled");
        bool mdns_client_enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
                mdns_client_enabled ? "true" : "false");
        
        json_t* enable_ipv6 = json_object_get(mdns_client, "EnableIPv6");
        bool mdns_client_ipv6 = get_config_bool(enable_ipv6, true);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL, "Config",
                mdns_client_ipv6 ? "true" : "false");
        
        json_t* scan_interval = json_object_get(mdns_client, "ScanIntervalMs");
        if (json_is_integer(scan_interval)) {
            log_config_section_item("ScanInterval", "%d", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", 
                json_integer_value(scan_interval));
        }
        
        json_t* service_types = json_object_get(mdns_client, "ServiceTypes");
        if (json_is_array(service_types)) {
            size_t type_count = json_array_size(service_types);
            log_config_section_item("ServiceTypes", "%zu Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", type_count);
            
            for (size_t i = 0; i < type_count; i++) {
                json_t* type = json_array_get(service_types, i);
                if (json_is_string(type)) {
                    log_config_section_item("Type", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", 
                        json_string_value(type));
                }
            }
        }
        
        json_t* health_check = json_object_get(mdns_client, "HealthCheck");
        if (json_is_object(health_check)) {
            log_config_section_item("HealthCheck", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* health_enabled = json_object_get(health_check, "Enabled");
            bool health_check_enabled = get_config_bool(health_enabled, true);
            log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !health_enabled, 1, NULL, NULL, "Config",
                    health_check_enabled ? "true" : "false");
            
            json_t* interval = json_object_get(health_check, "IntervalMs");
            if (json_is_integer(interval)) {
                log_config_section_item("Interval", "%d", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", 
                    json_integer_value(interval));
            }
        }
    } else {
        log_config_section_header("mDNSClient");
        log_config_section_item("Status", "Section missing", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    }
    
    // Always return true as there's no failure condition here
    return true;
}