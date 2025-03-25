/*
 * Network Configuration
 *
 * Defines the configuration structure and defaults for network operations.
 * This includes settings for network interfaces, IP addresses, and port management.
 */

#ifndef HYDROGEN_CONFIG_NETWORK_H
#define HYDROGEN_CONFIG_NETWORK_H

#include <stddef.h>
#include <stdbool.h>  // Added for bool type

// Project headers
#include "../../network/network.h"  // For MAX_INTERFACES
#include "../config_forward.h"

// Default values for network configuration
#define DEFAULT_MAX_INTERFACES MAX_INTERFACES
#define DEFAULT_MAX_IPS_PER_INTERFACE 8
#define DEFAULT_MAX_INTERFACE_NAME_LENGTH 16
#define DEFAULT_MAX_IP_ADDRESS_LENGTH 40  // Accommodates IPv6
#define DEFAULT_START_PORT 1024
#define DEFAULT_END_PORT 65535

// Validation limits
#define MIN_INTERFACES 1
// MAX_INTERFACES defined in network.h
#define MIN_IPS_PER_INTERFACE 1
#define MAX_IPS_PER_INTERFACE 32
#define MIN_INTERFACE_NAME_LENGTH 1
#define MAX_INTERFACE_NAME_LENGTH 32
#define MIN_IP_ADDRESS_LENGTH 7   // "1.1.1.1"
#define MAX_IP_ADDRESS_LENGTH 45  // IPv6 with scope
#define MIN_PORT 1024
#define MAX_PORT 65535

// Network configuration structure
struct NetworkConfig {
    // Interface and IP limits
    size_t max_interfaces;
    size_t max_ips_per_interface;
    size_t max_interface_name_length;
    size_t max_ip_address_length;
    
    // Port range settings
    int start_port;
    int end_port;
    
    // Reserved ports management
    int* reserved_ports;           // Array of reserved port numbers
    size_t reserved_ports_count;   // Number of reserved ports
    
    // Interface availability configuration
    struct {
        char* interface_name;      // Name of the interface (e.g., "eth0")
        bool available;            // Whether the interface is available for use
    }* available_interfaces;       // Array of interface availability settings
    size_t available_interfaces_count; // Number of interfaces with availability settings
};

/*
 * Initialize network configuration with default values
 *
 * This function initializes a new NetworkConfig structure with conservative
 * default values suitable for most deployments.
 *
 * @param config Pointer to NetworkConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for reserved ports array
 */
int config_network_init(NetworkConfig* config);

/*
 * Free resources allocated for network configuration
 *
 * This function frees all resources allocated by config_network_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to NetworkConfig structure to cleanup
 */
void config_network_cleanup(NetworkConfig* config);

/*
 * Validate network configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies interface and IP limits are within acceptable ranges
 * - Validates port range settings
 * - Checks reserved ports for validity and conflicts
 * - Ensures length limits are reasonable
 *
 * @param config Pointer to NetworkConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any limit is outside valid range
 * - If port ranges are invalid
 * - If reserved ports are invalid or conflict
 */
int config_network_validate(const NetworkConfig* config);

/*
 * Add a reserved port to the configuration
 *
 * This function adds a port number to the reserved ports list.
 * The port must be within the configured port range and not already reserved.
 *
 * @param config Pointer to NetworkConfig structure
 * @param port Port number to reserve
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If port is outside valid range
 * - If port is already reserved
 * - If memory reallocation fails
 */
int config_network_add_reserved_port(NetworkConfig* config, int port);

/*
 * Check if a port is reserved
 *
 * This function checks if a given port number is in the reserved ports list.
 *
 * @param config Pointer to NetworkConfig structure
 * @param port Port number to check
 * @return 1 if reserved, 0 if not reserved, -1 on error
 *
 * Error conditions:
 * - If config is NULL
 * - If port is outside valid range
 */
int config_network_is_port_reserved(const NetworkConfig* config, int port);

#endif /* HYDROGEN_CONFIG_NETWORK_H */