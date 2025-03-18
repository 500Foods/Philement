/*
 * Network configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"
#include "../config_utils.h"
#include "../network/config_network.h"
#include "json_network.h"

bool load_json_network(json_t* root, AppConfig* config) {
    // Network Configuration
    json_t* network = json_object_get(root, "Network");
    if (json_is_object(network)) {
        log_config_section_header("Network");
        
        // Interface and IP Settings
        json_t* interfaces = json_object_get(network, "Interfaces");
        if (json_is_object(interfaces)) {
            log_config_section_item("Interfaces", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(interfaces, "MaxInterfaces");
            config->network.max_interfaces = get_config_size(val, DEFAULT_MAX_INTERFACES);
            log_config_section_item("MaxInterfaces", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->network.max_interfaces);
            
            val = json_object_get(interfaces, "MaxIPsPerInterface");
            config->network.max_ips_per_interface = get_config_size(val, DEFAULT_MAX_IPS_PER_INTERFACE);
            log_config_section_item("MaxIPsPerInterface", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->network.max_ips_per_interface);
            
            val = json_object_get(interfaces, "MaxInterfaceNameLength");
            config->network.max_interface_name_length = get_config_size(val, DEFAULT_MAX_INTERFACE_NAME_LENGTH);
            log_config_section_item("MaxInterfaceNameLength", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->network.max_interface_name_length);
            
            val = json_object_get(interfaces, "MaxIPAddressLength");
            config->network.max_ip_address_length = get_config_size(val, DEFAULT_MAX_IP_ADDRESS_LENGTH);
            log_config_section_item("MaxIPAddressLength", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->network.max_ip_address_length);
        }
        
        // Port Settings
        json_t* ports = json_object_get(network, "Ports");
        if (json_is_object(ports)) {
            log_config_section_item("Ports", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(ports, "StartPort");
            config->network.start_port = get_config_int(val, DEFAULT_START_PORT);
            log_config_section_item("StartPort", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->network.start_port);
            
            val = json_object_get(ports, "EndPort");
            config->network.end_port = get_config_int(val, DEFAULT_END_PORT);
            log_config_section_item("EndPort", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->network.end_port);
            
            // Reserved Ports
            json_t* reserved = json_object_get(ports, "ReservedPorts");
            if (json_is_array(reserved)) {
                size_t index;
                json_t* port_value;
                
                // First clean up any existing reserved ports
                if (config->network.reserved_ports) {
                    free(config->network.reserved_ports);
                    config->network.reserved_ports = NULL;
                    config->network.reserved_ports_count = 0;
                }
                
                size_t count = json_array_size(reserved);
                log_config_section_item("ReservedPorts", "Count: %zu", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", count);
                
                if (count > 0) {
                    // Allocate memory for reserved ports
                    config->network.reserved_ports = malloc(count * sizeof(int));
                    if (config->network.reserved_ports) {
                        config->network.reserved_ports_count = 0;
                        
                        // Add each reserved port
                        json_array_foreach(reserved, index, port_value) {
                            if (json_is_integer(port_value)) {
                                int port = json_integer_value(port_value);
                                if (port >= MIN_PORT && port <= MAX_PORT) {
                                    config->network.reserved_ports[config->network.reserved_ports_count++] = port;
                                    log_config_section_item("ReservedPort", "%d", LOG_LEVEL_STATE, 0, 2, NULL, NULL, "Config", port);
                                } else {
                                    log_config_section_item("ReservedPort", "Invalid: %d", LOG_LEVEL_ERROR, 0, 2, NULL, NULL, "Config", port);
                                }
                            }
                            
                            // Stop if we've filled the array
                            if (config->network.reserved_ports_count >= count) break;
                        }
                    } else {
                        log_config_section_item("ReservedPorts", "Memory allocation failed", LOG_LEVEL_ERROR, 0, 1, NULL, NULL, "Config");
                    }
                }
            }
        }
        
        // Validate configuration
        if (config_network_validate(&config->network) != 0) {
            log_config_section_item("Status", "Invalid configuration, using defaults", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            config_network_cleanup(&config->network);
            config_network_init(&config->network);
        }
        
        return true;
    } else {
        // Set defaults if no network configuration is provided
        config_network_init(&config->network);
        
        log_config_section_header("Network");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        return true;
    }
}