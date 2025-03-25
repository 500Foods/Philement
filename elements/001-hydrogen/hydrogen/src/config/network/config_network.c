/*
 * Network Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_network.h"

// Initial capacity for reserved ports array
#define INITIAL_RESERVED_PORTS_CAPACITY 16

int config_network_init(NetworkConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize interface and IP limits
    config->max_interfaces = DEFAULT_MAX_INTERFACES;
    config->max_ips_per_interface = DEFAULT_MAX_IPS_PER_INTERFACE;
    config->max_interface_name_length = DEFAULT_MAX_INTERFACE_NAME_LENGTH;
    config->max_ip_address_length = DEFAULT_MAX_IP_ADDRESS_LENGTH;

    // Initialize port range
    config->start_port = DEFAULT_START_PORT;
    config->end_port = DEFAULT_END_PORT;

    // Initialize reserved ports array
    config->reserved_ports = malloc(INITIAL_RESERVED_PORTS_CAPACITY * sizeof(int));
    if (!config->reserved_ports) {
        config_network_cleanup(config);
        return -1;
    }
    config->reserved_ports_count = 0;
    
    // Initialize available interfaces array
    config->available_interfaces = NULL;
    config->available_interfaces_count = 0;

    return 0;
}

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

static int is_port_in_range(int port, int start_port, int end_port) {
    return port >= start_port && port <= end_port;
}

static int compare_ints(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

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

int config_network_validate(const NetworkConfig* config) {
    if (!config) {
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
        return -1;
    }

    // Validate port range
    if (config->start_port < MIN_PORT ||
        config->start_port > MAX_PORT ||
        config->end_port < MIN_PORT ||
        config->end_port > MAX_PORT ||
        config->start_port >= config->end_port) {
        return -1;
    }

    // Validate reserved ports
    if (config->reserved_ports_count > 0) {
        if (!config->reserved_ports) {
            return -1;
        }

        // Create a sorted copy for validation
        int* sorted_ports = malloc(config->reserved_ports_count * sizeof(int));
        if (!sorted_ports) {
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
                return -1;
            }
        }

        if (has_duplicate_ports(sorted_ports, config->reserved_ports_count)) {
            free(sorted_ports);
            return -1;
        }

        free(sorted_ports);
    }

    return 0;
}

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