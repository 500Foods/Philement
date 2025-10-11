/*
 * Network Configuration Implementation
 *
 * Implements the configuration handlers for network operations,
 * including JSON parsing, environment variable handling, and validation.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_network.h"

// Static network limits with default values
static const NetworkLimits network_limits = {
    .min_interfaces = 1,
    .max_interfaces = 16,
    .min_ips_per_interface = 1,
    .max_ips_per_interface = 32,
    .min_interface_name_length = 1,
    .max_interface_name_length = 32,
    .min_ip_address_length = 7,   // "1.1.1.1"
    .max_ip_address_length = 50,  // IPv6 with scope
    .min_port = 1024,
    .max_port = 65535,
    .initial_reserved_ports_capacity = 16
};

// Get the network validation limits
const NetworkLimits* get_network_limits(void) {
    return &network_limits;
}

// Forward declarations for internal functions
static int is_port_in_range(int port, int start_port, int end_port);
static void sort_available_interfaces(NetworkConfig* config);

// Compare function for interface sorting
static int compare_interface_names(const void* a, const void* b) {
    const struct {
        char* interface_name;
        bool available;
    }* if_a = a;
    const struct {
        char* interface_name;
        bool available;
    }* if_b = b;
    
    return strcmp(if_a->interface_name, if_b->interface_name);
}

// Sort available interfaces by name
static void sort_available_interfaces(NetworkConfig* config) {
    if (!config || !config->available_interfaces || config->available_interfaces_count <= 1) {
        return;
    }
    
    qsort(config->available_interfaces, 
          config->available_interfaces_count,
          sizeof(*config->available_interfaces),
          compare_interface_names);
}

// Load network configuration from JSON
bool load_network_config(json_t* root, AppConfig* config) {

    bool success = true;

    // Initialize with defaults
    config->network = (NetworkConfig){
        .max_interfaces = network_limits.max_interfaces,
        .max_ips_per_interface = network_limits.max_ips_per_interface,
        .max_interface_name_length = network_limits.max_interface_name_length,
        .max_ip_address_length = network_limits.max_ip_address_length,
        .start_port = network_limits.min_port,
        .end_port = network_limits.max_port,
        .reserved_ports = NULL,
        .reserved_ports_count = 0,
        .available_interfaces = NULL,
        .available_interfaces_count = 0
    };

    // Allocate initial reserved ports array
    config->network.reserved_ports = malloc(network_limits.initial_reserved_ports_capacity * sizeof(int));
    if (!config->network.reserved_ports) {
        log_this(SR_CONFIG, "Failed to allocate reserved ports array", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Process network configuration
    success = PROCESS_SECTION(root, "Network");
    if (!success) {
        cleanup_network_config(&config->network);
        return false;
    }

    // Process interface settings
    success = success && PROCESS_SECTION(root, "Network.Interfaces");
    success = success && PROCESS_SIZE(root, &config->network, max_interfaces, "Network.Interfaces.MaxInterfaces", "Network");
    success = success && PROCESS_SIZE(root, &config->network, max_ips_per_interface, "Network.Interfaces.MaxIPsPerInterface", "Network");
    success = success && PROCESS_SIZE(root, &config->network, max_interface_name_length, "Network.Interfaces.MaxInterfaceNameLength", "Network");
    success = success && PROCESS_SIZE(root, &config->network, max_ip_address_length, "Network.Interfaces.MaxIPAddressLength", "Network");

    // Process port settings
    success = success && PROCESS_SECTION(root, "Network.PortAllocation");
    success = success && PROCESS_INT(root, &config->network, start_port, "Network.PortAllocation.StartPort", "Network");
    success = success && PROCESS_INT(root, &config->network, end_port, "Network.PortAllocation.EndPort", "Network");

    // Process reserved ports array with proper indentation
    success = success && PROCESS_SECTION(root, "Network.PortAllocation.ReservedPorts");
    {
        // Create temporary struct for array processing
        struct {
            int* array;
            size_t count;
        } ports = {
            .array = config->network.reserved_ports,
            .count = config->network.reserved_ports_count
        };
        success = success && process_int_array_config(root, 
                                                    (ConfigIntArray){
                                                        .array = ports.array,
                                                        .count = &ports.count,
                                                        .capacity = network_limits.initial_reserved_ports_capacity
                                                    },
                                                    "Network.PortAllocation.ReservedPorts", "Network");
        config->network.reserved_ports_count = ports.count;
    }

        // Process interface availability
        json_t* network = json_object_get(root, "Network");
        json_t* available = network ? json_object_get(network, "Available") : NULL;


        if (available && json_is_object(available)) {

            // Process Available section header
        success = success && PROCESS_SECTION(root, "Network.Available");

        // Log the Available section header (like other sections)
        log_this(SR_CONFIG, "―― Available", LOG_LEVEL_DEBUG, 0);

        // Get all interface names for sorting
        const char** interface_names = malloc(json_object_size(available) * sizeof(char*));
        size_t interface_count = 0;

        const char* interface_name;
        json_t* value;
        json_object_foreach(available, interface_name, value) {
            if (json_is_boolean(value)) {
                interface_names[interface_count++] = interface_name;
            }
        }

        // Allocate space for interfaces (we'll sort them after loading)
        config->network.available_interfaces = malloc(interface_count * sizeof(*config->network.available_interfaces));
        if (!config->network.available_interfaces) {
            free(interface_names);
            log_this(SR_CONFIG, "Failed to allocate interface array", LOG_LEVEL_ERROR, 0);
            cleanup_network_config(&config->network);
            return false;
        }

        // Process each interface in sorted order
        for (size_t i = 0; i < interface_count; i++) {
            interface_name = interface_names[i];
            value = json_object_get(available, interface_name);
            bool is_enabled = json_boolean_value(value);

            // Initialize interface entry
            config->network.available_interfaces[i].interface_name = strdup(interface_name);
            if (!config->network.available_interfaces[i].interface_name) {
                free(interface_names);
                log_this(SR_CONFIG, "Failed to allocate interface name", LOG_LEVEL_ERROR, 0);
                cleanup_network_config(&config->network);
                return false;
            }
            config->network.available_interfaces[i].available = is_enabled;

            // Log the interface availability with proper format (but only if not "all")
            if (strcmp(interface_name, "all") != 0) {
                log_this(SR_CONFIG, "――― %s: %s", LOG_LEVEL_DEBUG, 2, interface_name, is_enabled ? "enabled" : "disabled");
            }
        }

        config->network.available_interfaces_count = interface_count;
        free(interface_names);

        // Sort interfaces by name for consistent ordering
        sort_available_interfaces(&config->network);
        } else {
            // Default to just "all" enabled if no Available section (IE, an invalid actual interface)
            config->network.available_interfaces = malloc(sizeof(*config->network.available_interfaces));
            if (!config->network.available_interfaces) {
                log_this(SR_CONFIG, "Failed to allocate interface array", LOG_LEVEL_ERROR, 0);
                cleanup_network_config(&config->network);
                return false;
            }

            config->network.available_interfaces[0].interface_name = strdup("all");
            if (!config->network.available_interfaces[0].interface_name) {
                log_this(SR_CONFIG, "Failed to allocate interface name", LOG_LEVEL_ERROR, 0);
                cleanup_network_config(&config->network);
                return false;
            }
            config->network.available_interfaces[0].available = true;
            config->network.available_interfaces_count = 1;

            // Log the default interface
            log_this(SR_CONFIG, "― Available *", LOG_LEVEL_DEBUG, 0);
            log_this(SR_CONFIG, "――― all: enabled *", LOG_LEVEL_DEBUG, 0);
        }

    if (!success) {
        cleanup_network_config(&config->network);
        return false;
    }

    return success;
}

// Initialize network configuration with default values from limits
int config_network_init(NetworkConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Network config pointer is NULL", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    // Initialize with defaults from network_limits
    config->max_interfaces = network_limits.max_interfaces;
    config->max_ips_per_interface = network_limits.max_ips_per_interface;
    config->max_interface_name_length = network_limits.max_interface_name_length;
    config->max_ip_address_length = network_limits.max_ip_address_length;
    config->start_port = network_limits.min_port;
    config->end_port = network_limits.max_port;

    // Initialize arrays
    config->reserved_ports = malloc(network_limits.initial_reserved_ports_capacity * sizeof(int));
    if (!config->reserved_ports) {
        log_this(SR_CONFIG, "Failed to allocate reserved ports array", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    config->reserved_ports_count = 0;
    config->available_interfaces = NULL;
    config->available_interfaces_count = 0;

    return 0;
}

void dump_network_config(const NetworkConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Cannot dump NULL network config", LOG_LEVEL_TRACE, 0);
        return;
    }

    // Interface and IP limits
    DUMP_TEXT("――", "Interfaces");
    DUMP_SIZE("―――― max_interfaces", config->max_interfaces);
    DUMP_SIZE("―――― max_ips_per_interface", config->max_ips_per_interface);
    DUMP_SIZE("―――― max_interface_name_length", config->max_interface_name_length);
    DUMP_SIZE("―――― max_ip_address_length", config->max_ip_address_length);

    // Port range settings
    DUMP_INT("―――― start_port", config->start_port);
    DUMP_INT("―――― end_port", config->end_port);

    // Reserved ports
    DUMP_TEXT("――", "Reserved Ports");
    if (config->reserved_ports && config->reserved_ports_count > 0) {
        for (size_t i = 0; i < config->reserved_ports_count; i++) {
            DUMP_INT("―――― Port", config->reserved_ports[i]);
        }
    } else {
        DUMP_TEXT("――――", "None");
    }

    // Available interfaces
    DUMP_INT("―― Available Interfaces", config->available_interfaces_count);
    if (config->available_interfaces && config->available_interfaces_count > 0) {
        for (size_t i = 0; i < config->available_interfaces_count; i++) {
            const char* status = config->available_interfaces[i].available ? "enabled" : "disabled";
            DUMP_STRING2( "――――", config->available_interfaces[i].interface_name, status);
        }
    } else {
        log_this(SR_CONFIG, "―――― None", LOG_LEVEL_DEBUG, 0);
    }
}

// Free resources allocated for network configuration
void cleanup_network_config(NetworkConfig* config) {
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
    if (config->reserved_ports_count % network_limits.initial_reserved_ports_capacity == 0) {
        size_t new_size = (config->reserved_ports_count + network_limits.initial_reserved_ports_capacity)
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
