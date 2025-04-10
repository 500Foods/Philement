/*
 * Network configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include <string.h>
#include "../config.h"
#include "../config_utils.h"
#include "../network/config_network.h"
#include "json_network.h"
#include "../logging/config_logging_utils.h"

// Define a type that matches the anonymous structure in NetworkConfig
typedef struct {
    char* interface_name;      // Name of the interface (e.g., "eth0")
    bool available;            // Whether the interface is available for use
} interface_availability_t;

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
        
        // Interface Availability Settings
        json_t* available = json_object_get(network, "Available");
        if (json_is_object(available)) {
            log_config_section_item("Available", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            // First clean up any existing available interfaces
            if (config->network.available_interfaces) {
                for (size_t i = 0; i < config->network.available_interfaces_count; i++) {
                    if (config->network.available_interfaces[i].interface_name) {
                        free(config->network.available_interfaces[i].interface_name);
                    }
                }
                free(config->network.available_interfaces);
                config->network.available_interfaces = NULL;
                config->network.available_interfaces_count = 0;
            }
            
            // Count the number of interfaces in the Available object
            size_t count = json_object_size(available);
            if (count > 0) {
                // Allocate memory for available interfaces
                config->network.available_interfaces = malloc(count * sizeof(*config->network.available_interfaces));
                if (config->network.available_interfaces) {
                    // Initialize the array
                    memset(config->network.available_interfaces, 0, count * sizeof(*config->network.available_interfaces));
                    
                    // Add each available interface
                    const char* key;
                    json_t* value;
                    size_t index = 0;
                    
                    json_object_foreach(available, key, value) {
                        if (json_is_boolean(value)) {
                            // Allocate memory for the interface name
                            size_t key_len = strlen(key) + 1;
                            config->network.available_interfaces[index].interface_name = malloc(key_len);
                            if (config->network.available_interfaces[index].interface_name) {
                                strcpy(config->network.available_interfaces[index].interface_name, key);
                                // Set the availability
                                config->network.available_interfaces[index].available = json_boolean_value(value);
                                
                                // Log the interface availability
                                log_config_section_item("Interface", "%s: %s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config",
                                                       key, config->network.available_interfaces[index].available ? "enabled" : "disabled");
                                
                                index++;
                            }
                        }
                    }
                    
                    // Update the count
                    config->network.available_interfaces_count = index;
                } else {
                    log_config_section_item("Available", "Memory allocation failed", LOG_LEVEL_ERROR, 0, 1, NULL, NULL, "Config");
                }
            }
        }
        
        // Save a copy of the available interfaces before validation
        interface_availability_t* saved_interfaces = NULL;
        size_t saved_count = 0;
        
        if (config->network.available_interfaces && config->network.available_interfaces_count > 0) {
            // Allocate memory for the saved interfaces
            saved_interfaces = malloc(config->network.available_interfaces_count * sizeof(interface_availability_t));
            if (saved_interfaces) {
                // Initialize the array
                memset(saved_interfaces, 0, config->network.available_interfaces_count * sizeof(interface_availability_t));
                
                // Copy each interface
                for (size_t i = 0; i < config->network.available_interfaces_count; i++) {
                    if (config->network.available_interfaces[i].interface_name) {
                        size_t name_len = strlen(config->network.available_interfaces[i].interface_name) + 1;
                        saved_interfaces[i].interface_name = malloc(name_len);
                        if (saved_interfaces[i].interface_name) {
                            strcpy(saved_interfaces[i].interface_name, config->network.available_interfaces[i].interface_name);
                            saved_interfaces[i].available = config->network.available_interfaces[i].available;
                            saved_count++;
                        }
                    }
                }
            }
        }
        
        // Validate configuration
        int validation_result = config_network_validate(&config->network);
        
        if (validation_result != 0) {
            log_config_section_item("Status", "Invalid configuration, using defaults", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            
            config_network_cleanup(&config->network);
            config_network_init(&config->network);
            
            // Restore the saved interfaces
            if (saved_interfaces && saved_count > 0) {
                // Free any existing interfaces
                if (config->network.available_interfaces) {
                    for (size_t i = 0; i < config->network.available_interfaces_count; i++) {
                        if (config->network.available_interfaces[i].interface_name) {
                            free(config->network.available_interfaces[i].interface_name);
                        }
                    }
                    free(config->network.available_interfaces);
                }
                
                // Set the saved interfaces with a cast to handle type incompatibility
                config->network.available_interfaces = (void*)saved_interfaces;
                config->network.available_interfaces_count = saved_count;
            }
        } else {
            // Free the saved interfaces if validation succeeded
            if (saved_interfaces) {
                for (size_t i = 0; i < saved_count; i++) {
                    if (saved_interfaces[i].interface_name) {
                        free(saved_interfaces[i].interface_name);
                    }
                }
                free(saved_interfaces);
            }
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