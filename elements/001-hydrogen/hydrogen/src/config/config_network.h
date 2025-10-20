/*
 * Network Configuration
 *
 * Defines the configuration structure and handlers for network operations.
 * This includes settings for network interfaces, IP addresses, and port management.
 */

#ifndef HYDROGEN_CONFIG_NETWORK_H
#define HYDROGEN_CONFIG_NETWORK_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// Network validation limits structure
struct NetworkLimits {
    size_t min_interfaces;
    size_t max_interfaces;
    size_t min_ips_per_interface;
    size_t max_ips_per_interface;
    size_t min_interface_name_length;
    size_t max_interface_name_length;
    size_t min_ip_address_length;
    size_t max_ip_address_length;
    int min_port;
    int max_port;
    size_t initial_reserved_ports_capacity;
};
typedef struct NetworkLimits NetworkLimits;

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
    }* available_interfaces;       // Array of interface availability settings (sorted by name)
    size_t available_interfaces_count; // Number of interfaces with availability settings
};
typedef struct NetworkConfig NetworkConfig;

// Note: Interface names are maintained in sorted order within available_interfaces

// Helper functions for network configuration

/*
 * Check if a port is within the specified range
 *
 * @param port Port number to check
 * @param start_port Starting port of range
 * @param end_port Ending port of range
 * @return 1 if in range, 0 if not
 */
int is_port_in_range(int port, int start_port, int end_port);

/*
 * Sort available interfaces by name
 *
 * @param config Pointer to NetworkConfig structure
 */
void sort_available_interfaces(NetworkConfig* config);

/*
 * Compare function for interface sorting
 *
 * @param a First interface element
 * @param b Second interface element
 * @return Comparison result for qsort
 */
int compare_interface_names(const void* a, const void* b);

// Get the network validation limits
const NetworkLimits* get_network_limits(void);

/*
 * Load network configuration from JSON with validation limits
 *
 * This function loads the network configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified. All validation is performed during launch
 * readiness checks.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_network_config(json_t* root, AppConfig* config);

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
void cleanup_network_config(NetworkConfig* config);

/*
 * Dump network configuration to logs
 *
 * This function dumps the current state of the network configuration
 * to the logs, following the standard format:
 * - Key: Value for normal values
 * - Array elements with proper indentation
 * - Interface availability status
 *
 * @param config Pointer to NetworkConfig structure to dump
 */
void dump_network_config(const NetworkConfig* config);

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
