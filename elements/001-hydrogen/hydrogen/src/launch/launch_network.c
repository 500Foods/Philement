/*
 * Network Subsystem Launch Readiness Check
 * 
 * This file implements the launch readiness check for the network subsystem.
 * It verifies that network interfaces are available and properly configured.
 */

 // Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// Network subsystem shutdown flag
volatile sig_atomic_t network_system_shutdown = 0;

// Registry ID and cached readiness state
static int network_subsystem_id = -1;

// Register the network subsystem with the registry (for readiness)
static void register_network(void) {
    if (network_subsystem_id < 0) {
        network_subsystem_id = register_subsystem("Network", NULL, NULL, NULL, NULL, NULL);
    }
}

// Network subsystem launch function
int launch_network_subsystem(void) {
    log_this("Network", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Network", "LAUNCH: NETWORK", LOG_LEVEL_STATE);
    
    // Register with launch handlers if not already registered
    if (network_subsystem_id < 0) {
        network_subsystem_id = register_subsystem_from_launch("Network", NULL, NULL, NULL,
                                                            (int (*)(void))launch_network_subsystem,
                                                            (void (*)(void))shutdown_network_subsystem);
        if (network_subsystem_id < 0) {
            log_this("Network", "Failed to register network subsystem", LOG_LEVEL_ERROR);
            return 0;
        }
    }
    
    // Step 1: Verify system state
    if (server_stopping) {
        log_this("Network", "Cannot initialize network during shutdown", LOG_LEVEL_STATE);
        return 0;
    }
    
    if (!server_starting && !server_running) {
        log_this("Network", "Cannot initialize network outside startup/running phase", LOG_LEVEL_STATE);
        return 0;
    }
    
    // Step 2: Initialize network subsystem
    log_this("Network", "  Step 1: Initializing network subsystem", LOG_LEVEL_STATE);
    
    // Step 3: Enumerate network interfaces
    log_this("Network", "  Step 2: Enumerating network interfaces", LOG_LEVEL_STATE);
    network_info_t *info = get_network_info();
    if (!info) {
        log_this("Network", "Failed to get network info", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Step 4: Test network interfaces
    log_this("Network", "  Step 3: Testing network interfaces", LOG_LEVEL_STATE);
    bool ping_success = test_network_interfaces(info);
    
    free_network_info(info);
    
    if (!ping_success) {
        log_this("Network", "No network interfaces responded to ping", LOG_LEVEL_ERROR);
        log_this("Network", "LAUNCH: NETWORK - Failed to launch", LOG_LEVEL_STATE);
        return 0;
    }
    
    // Step 5: Update registry and verify state
    log_this("Network", "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("Network", true);
    
    SubsystemState final_state = get_subsystem_state(network_subsystem_id);
  
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("Network", "LAUNCH: NETWORK - Successfully launched and running", LOG_LEVEL_STATE);
        return 1;
    } else {
        log_this("Network", "LAUNCH: NETWORK - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }
}

// Network subsystem shutdown function
void shutdown_network_subsystem(void) {
    log_this("Network", "Shutting down network subsystem", LOG_LEVEL_STATE);
    // No specific shutdown actions needed for network subsystem
}




/*
 * Check if the network subsystem is ready to launch
 * 
 * This function performs readiness checks for the network subsystem by:
 * - Verifying system state and dependencies
 * - Checking network interface availability
 * - Validating interface configuration
 * 
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Returns:
 * LaunchReadiness structure containing:
 * - ready: true if system is ready to launch
 * - messages: NULL-terminated array of status messages (must be freed by caller on success path)
 * - subsystem: Name of the subsystem ("Network")
 */



// Check if the network subsystem is ready to launch
LaunchReadiness check_network_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup("Network"));

    // Register with registry if not already registered
    register_network();
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem registered"));

    // Register Thread dependency - we only need to verify it's ready for launch
    if (!add_dependency_from_launch(network_subsystem_id, "Threads")) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Thread dependency"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Thread dependency registered"));

    // Early return cases
    if (server_stopping || web_server_shutdown) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System shutdown in progress"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (system shutdown)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    if (!server_starting && !server_running) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System not in startup or running state"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (invalid system state)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    const AppConfig* config = get_app_config();
    if (!config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (no configuration)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    // Get network limits for validation
    const NetworkLimits* limits = get_network_limits();

    // Validate interface and IP limits
    if (config->network.max_interfaces < limits->min_interfaces ||
        config->network.max_interfaces > limits->max_interfaces) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid max_interfaces: %zu (must be between %zu and %zu)",
                    config->network.max_interfaces, limits->min_interfaces, limits->max_interfaces);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (invalid max_interfaces)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    if (config->network.max_ips_per_interface < limits->min_ips_per_interface ||
        config->network.max_ips_per_interface > limits->max_ips_per_interface) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid max_ips_per_interface: %zu (must be between %zu and %zu)",
                    config->network.max_ips_per_interface, limits->min_ips_per_interface, limits->max_ips_per_interface);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (invalid max_ips_per_interface)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    // Validate name and address length limits
    if (config->network.max_interface_name_length < limits->min_interface_name_length ||
        config->network.max_interface_name_length > limits->max_interface_name_length) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid interface name length: %zu (must be between %zu and %zu)",
                    config->network.max_interface_name_length, limits->min_interface_name_length, limits->max_interface_name_length);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (invalid interface name length)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    if (config->network.max_ip_address_length < limits->min_ip_address_length ||
        config->network.max_ip_address_length > limits->max_ip_address_length) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid IP address length: %zu (must be between %zu and %zu)",
                    config->network.max_ip_address_length, limits->min_ip_address_length, limits->max_ip_address_length);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (invalid IP address length)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    // Validate port range
    if (config->network.start_port < limits->min_port ||
        config->network.start_port > limits->max_port ||
        config->network.end_port < limits->min_port ||
        config->network.end_port > limits->max_port ||
        config->network.start_port >= config->network.end_port) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid port range: %d-%d (must be between %d and %d, start < end)",
                    config->network.start_port, config->network.end_port, limits->min_port, limits->max_port);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (invalid port range)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    // Validate reserved ports array
    if (config->network.reserved_ports_count > 0 && !config->network.reserved_ports) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Reserved ports array is NULL but count > 0"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (reserved ports array issue)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network configuration validated"));

    network_info_t* network_info = get_network_info();
    if (!network_info) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to get network information"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (no network information)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    if (network_info->count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No network interfaces available"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (no interfaces)"));
        free_network_info(network_info);
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "Network", .ready = false, .messages = messages };
    }

    char* count_msg = malloc(256);
    if (count_msg) {
        snprintf(count_msg, 256, "  Go:      %d network interfaces available", network_info->count);
        add_launch_message(&messages, &count, &capacity, count_msg);
    }

    // Check for "all" interfaces configuration
    bool all_interfaces_enabled = false;
    if (config && config->network.available_interfaces &&
        config->network.available_interfaces_count == 1 &&
        config->network.available_interfaces[0].interface_name &&
        strcmp(config->network.available_interfaces[0].interface_name, "all") == 0 &&
        config->network.available_interfaces[0].available) {
        all_interfaces_enabled = true;
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      All network interfaces enabled via config"));
    }

    // Check for specifically configured interfaces if not using "all"
    int json_interfaces_count = 0;
    if (!all_interfaces_enabled && config && config->network.available_interfaces &&
        config->network.available_interfaces_count > 0) {
        json_interfaces_count = (int)config->network.available_interfaces_count;
        char* config_count_msg = malloc(256);
        if (config_count_msg) {
            snprintf(config_count_msg, 256, "  Go:      %d network interfaces configured:", json_interfaces_count);
            add_launch_message(&messages, &count, &capacity, config_count_msg);
        }

        // List specifically configured interfaces
        for (size_t i = 0; i < config->network.available_interfaces_count; i++) {
            if (config->network.available_interfaces[i].interface_name) {
                const char* interface_name = config->network.available_interfaces[i].interface_name;
                bool is_available = config->network.available_interfaces[i].available;

                char* interface_msg = malloc(256);
                if (interface_msg) {
                    if (is_available) {
                        snprintf(interface_msg, 256, "  Go:      Available: %s is enabled", interface_name);
                    } else {
                        snprintf(interface_msg, 256, "  No-Go:   Available: %s is disabled", interface_name);
                    }
                    add_launch_message(&messages, &count, &capacity, interface_msg);
                }
            }
        }
    } else if (!all_interfaces_enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      No specific interfaces configured - using defaults"));
    }

    // Check each interface's status
    int up_interfaces = 0;
    for (int i = 0; i < network_info->count; i++) {
        interface_t* interface = &network_info->interfaces[i];
        bool is_up = interface->ip_count > 0;
        bool is_available = all_interfaces_enabled; // If all interfaces enabled, start with true

        // If not all interfaces enabled, check specific configuration
        if (!all_interfaces_enabled) {
            bool is_configured = is_interface_configured(interface->name, &is_available);
            if (!is_configured) {
                is_available = true; // Not configured means enabled by default
            }
        }

        const char* config_status = all_interfaces_enabled ? "enabled via all:true" :
            (is_interface_configured(interface->name, NULL) ?
                (is_available ? "enabled in config" : "disabled in config") :
                "not in config - enabled by default");

        char* status_msg = malloc(256);
        if (status_msg) {
            if (is_up) {
                if (is_available) {
                    up_interfaces++;
                    snprintf(status_msg, 256, "  Go:      Interface %s is up (%s)", interface->name, config_status);
                } else {
                    snprintf(status_msg, 256, "  No-Go:   Interface %s is up but %s", interface->name, config_status);
                }
            } else {
                snprintf(status_msg, 256, "  No-Go:   Interface %s is down (%s)", interface->name, config_status);
            }
            add_launch_message(&messages, &count, &capacity, status_msg);
        }
    }

    if (up_interfaces > 0) {
        char* decision_msg = malloc(256);
        if (decision_msg) {
            snprintf(decision_msg, 256, "  Decide:  Go For Launch of Network Subsystem (%d interfaces ready)", up_interfaces);
            add_launch_message(&messages, &count, &capacity, decision_msg);
        }
        ready = true;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Network Subsystem (no interfaces ready)"));
        ready = false;
    }

    free_network_info(network_info);
    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = "Network",
        .ready = ready,
        .messages = messages
    };
}
