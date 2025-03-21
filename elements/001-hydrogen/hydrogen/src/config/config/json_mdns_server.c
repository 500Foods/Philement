/*
 * mDNS Server configuration JSON parsing
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "json_mdns_server.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"

bool load_json_mdns_server(json_t* root, AppConfig* config) {
    // mDNS Server Configuration
    json_t* mdns_server = json_object_get(root, "mDNSServer");
    if (json_is_object(mdns_server)) {
        log_config_section_header("mDNSServer");
        
        json_t* enabled = json_object_get(mdns_server, "Enabled");
        config->mdns_server.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
                config->mdns_server.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(mdns_server, "EnableIPv6");
        config->mdns_server.enable_ipv6 = get_config_bool(enable_ipv6, 1);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL, "Config",
                config->mdns_server.enable_ipv6 ? "true" : "false");

        json_t* device_id = json_object_get(mdns_server, "DeviceId");
        config->mdns_server.device_id = get_config_string_with_env("DeviceId", device_id, "hydrogen-printer");
        log_config_section_item("DeviceId", "%s", LOG_LEVEL_STATE, !device_id, 0, NULL, NULL, "Config",
            config->mdns_server.device_id);

        json_t* friendly_name = json_object_get(mdns_server, "FriendlyName");
        config->mdns_server.friendly_name = get_config_string_with_env("FriendlyName", friendly_name, "Hydrogen 3D Printer");
        log_config_section_item("FriendlyName", "%s", LOG_LEVEL_STATE, !friendly_name, 0, NULL, NULL, "Config",
            config->mdns_server.friendly_name);

        json_t* model = json_object_get(mdns_server, "Model");
        config->mdns_server.model = get_config_string_with_env("Model", model, "Hydrogen");
        log_config_section_item("Model", "%s", LOG_LEVEL_STATE, !model, 0, NULL, NULL, "Config",
            config->mdns_server.model);

        json_t* manufacturer = json_object_get(mdns_server, "Manufacturer");
        config->mdns_server.manufacturer = get_config_string_with_env("Manufacturer", manufacturer, "Philement");
        log_config_section_item("Manufacturer", "%s", LOG_LEVEL_STATE, !manufacturer, 0, NULL, NULL, "Config",
            config->mdns_server.manufacturer);

        json_t* version = json_object_get(mdns_server, "Version");
        config->mdns_server.version = get_config_string_with_env("Version", version, VERSION);
        log_config_section_item("Version", "%s", LOG_LEVEL_STATE, !version, 0, NULL, NULL, "Config",
            config->mdns_server.version);
        
        json_t* services = json_object_get(mdns_server, "Services");
        if (json_is_array(services)) {
            config->mdns_server.num_services = json_array_size(services);
            log_config_section_item("Services", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config",
                config->mdns_server.num_services);
            
            config->mdns_server.services = calloc(config->mdns_server.num_services, sizeof(mdns_server_service_t));
            if (config->mdns_server.services == NULL) {
                log_this("Config", "Failed to allocate memory for mDNS services array", LOG_LEVEL_ERROR);
                return false;
            }
            
            for (size_t i = 0; i < config->mdns_server.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;
                
                // Get service properties
                json_t* name = json_object_get(service, "Name");
                config->mdns_server.services[i].name = get_config_string_with_env("Name", name, "hydrogen");
                
                json_t* type = json_object_get(service, "Type");
                config->mdns_server.services[i].type = get_config_string_with_env("Type", type, "_http._tcp.local");

                json_t* port = json_object_get(service, "Port");
                config->mdns_server.services[i].port = get_config_int(port, DEFAULT_WEB_PORT);
                
                // Log service details after all properties are populated
                log_config_section_item("Service", "%s: %s on port %d", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config",
                                       config->mdns_server.services[i].name,
                                       config->mdns_server.services[i].type,
                                       config->mdns_server.services[i].port);

                json_t* txt_records = json_object_get(service, "TxtRecords");
                if (json_is_string(txt_records)) {
                    config->mdns_server.services[i].num_txt_records = 1;
                    config->mdns_server.services[i].txt_records = malloc(sizeof(char*));
                    if (config->mdns_server.services[i].txt_records == NULL) {
                        log_this("Config", "Failed to allocate memory for TXT records", LOG_LEVEL_ERROR);
                        // Cleanup already allocated services
                        for (size_t j = 0; j < i; j++) {
                            free(config->mdns_server.services[j].name);
                            free(config->mdns_server.services[j].type);
                            for (size_t k = 0; k < config->mdns_server.services[j].num_txt_records; k++) {
                                free(config->mdns_server.services[j].txt_records[k]);
                            }
                            free(config->mdns_server.services[j].txt_records);
                        }
                        free(config->mdns_server.services);
                        free(config->mdns_server.device_id);
                        free(config->mdns_server.friendly_name);
                        free(config->mdns_server.model);
                        free(config->mdns_server.manufacturer);
                        free(config->mdns_server.version);
                        return false;
                    }
                    config->mdns_server.services[i].txt_records[0] = get_config_string_with_env("TxtRecord", txt_records, "");
                } else if (json_is_array(txt_records)) {
                    config->mdns_server.services[i].num_txt_records = json_array_size(txt_records);
                    config->mdns_server.services[i].txt_records = malloc(config->mdns_server.services[i].num_txt_records * sizeof(char*));
                    if (config->mdns_server.services[i].txt_records == NULL) {
                        log_this("Config", "Failed to allocate memory for TXT records array", LOG_LEVEL_ERROR);
                        // Cleanup already allocated services
                        for (size_t j = 0; j < i; j++) {
                            free(config->mdns_server.services[j].name);
                            free(config->mdns_server.services[j].type);
                            for (size_t k = 0; k < config->mdns_server.services[j].num_txt_records; k++) {
                                free(config->mdns_server.services[j].txt_records[k]);
                            }
                            free(config->mdns_server.services[j].txt_records);
                        }
                        free(config->mdns_server.services);
                        free(config->mdns_server.device_id);
                        free(config->mdns_server.friendly_name);
                        free(config->mdns_server.model);
                        free(config->mdns_server.manufacturer);
                        free(config->mdns_server.version);
                        return false;
                    }
                    
                    for (size_t j = 0; j < config->mdns_server.services[i].num_txt_records; j++) {
                        json_t* record = json_array_get(txt_records, j);
                        config->mdns_server.services[i].txt_records[j] = get_config_string_with_env("TxtRecord", record, "");
                    }
                } else {
                    config->mdns_server.services[i].num_txt_records = 0;
                    config->mdns_server.services[i].txt_records = NULL;
                }
            }
        } else {
            config->mdns_server.num_services = 0;
            config->mdns_server.services = NULL;
        }
    } else {
        log_config_section_header("mDNSServer");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        
        // Set default values
        config->mdns_server.enabled = 1;
        config->mdns_server.enable_ipv6 = 1;
        config->mdns_server.device_id = strdup("hydrogen-printer");
        config->mdns_server.friendly_name = strdup("Hydrogen 3D Printer");
        config->mdns_server.model = strdup("Hydrogen");
        config->mdns_server.manufacturer = strdup("Philement");
        config->mdns_server.version = strdup(VERSION);
        config->mdns_server.num_services = 0;
        config->mdns_server.services = NULL;
        
        // Check for allocation failures
        if (!config->mdns_server.device_id || !config->mdns_server.friendly_name || 
            !config->mdns_server.model || !config->mdns_server.manufacturer || 
            !config->mdns_server.version) {
            log_this("Config", "Failed to allocate memory for mDNS server default strings", LOG_LEVEL_ERROR);
            free(config->mdns_server.device_id);
            free(config->mdns_server.friendly_name);
            free(config->mdns_server.model);
            free(config->mdns_server.manufacturer);
            free(config->mdns_server.version);
            return false;
        }
    }
    
    return true;
}