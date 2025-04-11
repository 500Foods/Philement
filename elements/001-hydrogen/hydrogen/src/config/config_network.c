/*
 * Network Configuration Implementation
 *
 * Implements the configuration handlers for network operations,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_network.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Initial capacity for reserved ports array
#define INITIAL_RESERVED_PORTS_CAPACITY 16

// Forward declarations for internal functions
static int is_port_in_range(int port, int start_port, int end_port);
static int compare_ints(const void* a, const void* b);
static int has_duplicate_ports(const int* ports, size_t count);

// Load network configuration from JSON
bool load_network_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->network = (NetworkConfig){
        .max_interfaces = MAX_INTERFACES,
        .max_ips_per_interface = 8,
        .max_interface_name_length = 16,
        .max_ip_address_length = 40,  // Accommodates IPv6
        .start_port = 1024,
        .end_port = 65535,
        .reserved_ports = NULL,
        .reserved_ports_count = 0,
        .available_interfaces = NULL,
        .available_interfaces_count = 0
    };

    // Allocate initial reserved ports array
    config->network.reserved_ports = malloc(INITIAL_RESERVED_PORTS_CAPACITY * sizeof(int));
    if (!config->network.reserved_ports) {
        log_this("Config", "Failed to allocate reserved ports array", LOG_LEVEL_ERROR);
        return false;
    }

    // Process all config items in sequence
    bool success = true;

    // One line per key/value using macros
    success = PROCESS_SECTION(root, "Network");
    
    if (success) {
        // Interface and IP Settings
        json_t* interfaces = json_object_get(json_object_get(root, "Network"), "Interfaces");
        if (json_is_object(interfaces)) {
            log_config_item("Interfaces", "Configured", false);
            
            success = success && PROCESS_SIZE(interfaces, &config->network, max_interfaces, "MaxInterfaces", "Network.Interfaces");
            success = success && PROCESS_SIZE(interfaces, &config->network, max_ips_per_interface, "MaxIPsPerInterface", "Network.Interfaces");
            success = success && PROCESS_SIZE(interfaces, &config->network, max_interface_name_length, "MaxInterfaceNameLength", "Network.Interfaces");
            success = success && PROCESS_SIZE(interfaces, &config->network, max_ip_address_length, "MaxIPAddressLength", "Network.Interfaces");
        }

        // Port Settings
        json_t* ports = json_object_get(json_object_get(root, "Network"), "Ports");
        if (json_is_object(ports)) {
            log_config_item("Ports", "Configured", false);
            
            success = success && PROCESS_INT(ports, &config->network, start_port, "StartPort", "Network.Ports");
            success = success && PROCESS_INT(ports, &config->network, end_port, "EndPort", "Network.Ports");

            // Reserved Ports
            json_t* reserved = json_object_get(ports, "ReservedPorts");
            if (json_is_array(reserved)) {
                size_t index;
                json_t* port_value;
                
                size_t count = json_array_size(reserved);
                char count_buffer[32];
                snprintf(count_buffer, sizeof(count_buffer), "Count: %s", format_int_buffer((int)count));
                log_config_item("- ReservedPorts", count_buffer, false);
                
                json_array_foreach(reserved, index, port_value) {
                    if (json_is_integer(port_value)) {
                        int port = json_integer_value(port_value);
                        if (config_network_add_reserved_port(&config->network, port) != 0) {
                            log_config_item("-- ReservedPort", "Failed to add port", false);
                            success = false;
                            break;
                        }
                        log_config_item("-- ReservedPort", format_int_buffer(port), false);
                    }
                }
            }
        }

        // Interface Availability Settings
        json_t* available = json_object_get(json_object_get(root, "Network"), "Available");
        if (json_is_object(available)) {
            log_config_item("Available", "Configured", false);
            
            size_t count = json_object_size(available);
            if (count > 0) {
                config->network.available_interfaces = malloc(count * sizeof(*config->network.available_interfaces));
                if (config->network.available_interfaces) {
                    memset(config->network.available_interfaces, 0, count * sizeof(*config->network.available_interfaces));
                    
                    const char* key;
                    json_t* value;
                    size_t index = 0;
                    
                    json_object_foreach(available, key, value) {
                        if (json_is_boolean(value)) {
                            size_t key_len = strlen(key) + 1;
                            config->network.available_interfaces[index].interface_name = malloc(key_len);
                            if (config->network.available_interfaces[index].interface_name) {
                                strcpy(config->network.available_interfaces[index].interface_name, key);
                                config->network.available_interfaces[index].available = json_boolean_value(value);
                                
                                char status_buffer[256];
                                snprintf(status_buffer, sizeof(status_buffer), "%s: %s", key,
                                       config->network.available_interfaces[index].available ? "enabled" : "disabled");
                                log_config_item("- Interface", status_buffer, false);
                                
                                index++;
                            }
                        }
                    }
                    config->network.available_interfaces_count = index;
                }
            }
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_network_validate(&config->network) == 0);
    }

    // Clean up on failure
    if (!success) {
        config_network_cleanup(&config->network);
    }

    return success;
}

// Initialize network configuration with default values
int config_network_init(NetworkConfig* config) {
    if (!config) {
        log_this("Config", "Network config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Initialize with defaults
    config->max_interfaces = MAX_INTERFACES;
    config->max_ips_per_interface = 8;
    config->max_interface_name_length = 16;
    config->max_ip_address_length = 40;
    config->start_port = 1024;
    config->end_port = 65535;

    // Initialize arrays
    config->reserved_ports = malloc(INITIAL_RESERVED_PORTS_CAPACITY * sizeof(int));
    if (!config->reserved_ports) {
        log_this("Config", "Failed to allocate reserved ports array", LOG_LEVEL_ERROR);
        return -1;
    }
    config->reserved_ports_count = 0;
    
    config->available_interfaces = NULL;
    config->available_interfaces_count = 0;

    return 0;
}

// Free resources allocated for network configuration
void config_network_cleanup(NetworkConfig* config) {
    if (!config) {
        return;
    }

    // Free reserved ports array
    free(config->reserved_ports);
    
    // Free available interfaces array
    if (config->available_interfaces) {
        for (size_t i = 0; i < config->available_interfaces_count; i++) {
            free(config->available_interfaces[i].interface_name);
        }
        free(config->available_interfaces);
    }

    // Zero out the structure
    memset(config, 0, sizeof(NetworkConfig));
}

// Check if a port is within the specified range
static int is_port_in_range(int port, int start_port, int end_port) {
    return port >= start_port && port <= end_port;
}

// Compare function for qsort
static int compare_ints(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

// Check for duplicate ports in a sorted array
static int has_duplicate_ports(const int* ports, size_t count) {
    if (count < 2) {
        return 0;
    }

    for (size_t i = 1; i < count; i++) {
        if (ports[i] == ports[i-1]) {
            return 1;
        }
    }
    return 0;
}

// Validate network configuration values
int config_network_validate(const NetworkConfig* config) {
    if (!config) {
        log_this("Config", "Network config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate interface and IP limits
    if (config->max_interfaces < MIN_INTERFACES ||
        config->max_interfaces > MAX_INTERFACES ||
        config->max_ips_per_interface < MIN_IPS_PER_INTERFACE ||
        config->max_ips_per_interface > MAX_IPS_PER_INTERFACE ||
        config->max_interface_name_length < MIN_INTERFACE_NAME_LENGTH ||
        config->max_interface_name_length > MAX_INTERFACE_NAME_LENGTH ||
        config->max_ip_address_length < MIN_IP_ADDRESS_LENGTH ||
        config->max_ip_address_length > MAX_IP_ADDRESS_LENGTH) {
        log_this("Config", "Invalid network interface limits", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate port range
    if (config->start_port < MIN_PORT ||
        config->start_port > MAX_PORT ||
        config->end_port < MIN_PORT ||
        config->end_port > MAX_PORT ||
        config->start_port >= config->end_port) {
        log_this("Config", "Invalid port range configuration", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate reserved ports
    if (config->reserved_ports_count > 0) {
        if (!config->reserved_ports) {
            log_this("Config", "Reserved ports array is NULL but count > 0", LOG_LEVEL_ERROR);
            return -1;
        }

        // Create a sorted copy for validation
        int* sorted_ports = malloc(config->reserved_ports_count * sizeof(int));
        if (!sorted_ports) {
            log_this("Config", "Failed to allocate memory for port validation", LOG_LEVEL_ERROR);
            return -1;
        }
        memcpy(sorted_ports, config->reserved_ports,
               config->reserved_ports_count * sizeof(int));
        qsort(sorted_ports, config->reserved_ports_count,
              sizeof(int), compare_ints);

        // Check for duplicates and range
        for (size_t i = 0; i < config->reserved_ports_count; i++) {
            if (!is_port_in_range(sorted_ports[i],
                                config->start_port,
                                config->end_port)) {
                free(sorted_ports);
                log_this("Config", "Reserved port outside valid range", LOG_LEVEL_ERROR);
                return -1;
            }
        }

        if (has_duplicate_ports(sorted_ports, config->reserved_ports_count)) {
            free(sorted_ports);
            log_this("Config", "Duplicate reserved ports found", LOG_LEVEL_ERROR);
            return -1;
        }

        free(sorted_ports);
    }

    return 0;
}

// Add a reserved port to the configuration
int config_network_add_reserved_port(NetworkConfig* config, int port) {
    if (!config) {
        return -1;
    }

    // Validate port number
    if (!is_port_in_range(port, config->start_port, config->end_port)) {
        return -1;
    }

    // Check if port is already reserved
    for (size_t i = 0; i < config->reserved_ports_count; i++) {
        if (config->reserved_ports[i] == port) {
            return -1;
        }
    }

    // Resize array if needed
    if (config->reserved_ports_count % INITIAL_RESERVED_PORTS_CAPACITY == 0) {
        size_t new_size = (config->reserved_ports_count + INITIAL_RESERVED_PORTS_CAPACITY)
                         * sizeof(int);
        int* new_array = realloc(config->reserved_ports, new_size);
        if (!new_array) {
            return -1;
        }
        config->reserved_ports = new_array;
    }

    // Add the new port
    config->reserved_ports[config->reserved_ports_count++] = port;
    return 0;
}

// Check if a port is reserved
int config_network_is_port_reserved(const NetworkConfig* config, int port) {
    if (!config || !config->reserved_ports) {
        return -1;
    }

    // Validate port number
    if (!is_port_in_range(port, config->start_port, config->end_port)) {
        return -1;
    }

    // Check if port is in reserved list
    for (size_t i = 0; i < config->reserved_ports_count; i++) {
        if (config->reserved_ports[i] == port) {
            return 1;
        }
    }

    return 0;
}