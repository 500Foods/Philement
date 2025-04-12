/*
 * Network Subsystem Launch Readiness Check
 * 
 * This file implements the launch readiness check for the network subsystem.
 * It verifies that network interfaces are available and properly configured.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include "../state/state.h"
#include "../landing/landing.h"
#include "../config/config.h"
#include "../config/config_network.h"
#include "../network/network.h"
#include "../logging/logging.h"
#include "../registry/registry_integration.h"
#include "launch.h"
#include "launch_network.h"

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;

// Network subsystem shutdown flag
volatile int network_system_shutdown = 0;

// Registry ID and cached readiness state
static int network_subsystem_id = -1;
static LaunchReadiness cached_readiness = {0};
static bool readiness_cached = false;

// Forward declarations
static void clear_cached_readiness(void);
static void register_network(void);

// Helper to clear cached readiness
static void clear_cached_readiness(void) {
    if (readiness_cached && cached_readiness.messages) {
        free_readiness_messages(&cached_readiness);
        readiness_cached = false;
    }
}

// Get cached readiness result
LaunchReadiness get_network_readiness(void) {
    if (readiness_cached) {
        return cached_readiness;
    }
    
    // Perform fresh check and cache result
    cached_readiness = check_network_launch_readiness();
    readiness_cached = true;
    return cached_readiness;
}

// Register the network subsystem with the registry
static void register_network(void) {
    // Only register if not already registered
    if (network_subsystem_id < 0) {
        network_subsystem_id = register_subsystem_from_launch("Network", NULL, NULL, NULL,
                                                            (int (*)(void))launch_network_subsystem,
                                                            (void (*)(void))shutdown_network_subsystem);
    }
}
// Network subsystem launch function
int launch_network_subsystem(void) {
    log_this("Network", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Network", "LAUNCH: NETWORK", LOG_LEVEL_STATE);
    
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
    
    // Clear any cached readiness before checking final state
    clear_cached_readiness();
    
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

// Add a message to the messages array
static void add_message(const char** messages, int* count, const char* message) {
    if (message) {
        messages[*count] = message;
        (*count)++;
    }
}

// Format and add a Go/No-Go message
static void add_go_message(const char** messages, int* count, const char* prefix, const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    char buffer[512];
    if (strcmp(prefix, "No-Go") == 0) {
        snprintf(buffer, sizeof(buffer), "  %s:   %s", prefix, temp);
    } else {
        snprintf(buffer, sizeof(buffer), "  %s:      %s", prefix, temp);
    }
    
    add_message(messages, count, strdup(buffer));
}

// Format and add a decision message
static void add_decision_message(const char** messages, int* count, const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "  Decide:  %s", temp);
    add_message(messages, count, strdup(buffer));
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
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (estimate max needed + NULL terminator)
    readiness.messages = malloc(50 * sizeof(const char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    add_message(readiness.messages, &msg_count, strdup("Network"));
    
    // Register with registry if not already registered
    register_network();
    
    // Register dependency on Threads subsystem
    if (network_subsystem_id >= 0) {
        if (!add_dependency_from_launch(network_subsystem_id, "Threads")) {
            add_go_message(readiness.messages, &msg_count, "No-Go", "Failed to register Thread dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            readiness.subsystem = "Network";
            free_readiness_messages(&readiness);
            return readiness;
        }
        add_go_message(readiness.messages, &msg_count, "Go", "Thread dependency registered");
    }
    
    // Early return cases with cleanup
    if (server_stopping || web_server_shutdown) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "System shutdown in progress");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }
    
    if (!server_starting && !server_running) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "System not in startup or running state");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }
    
    const AppConfig* app_config = get_app_config();
    if (!app_config) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "Configuration not loaded");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    // Get network limits for validation
    const NetworkLimits* limits = get_network_limits();

    // Validate interface and IP limits
    if (app_config->network.max_interfaces < limits->min_interfaces ||
        app_config->network.max_interfaces > limits->max_interfaces) {
        add_go_message(readiness.messages, &msg_count, "No-Go", 
            "Invalid max_interfaces: %zu (must be between %zu and %zu)",
            app_config->network.max_interfaces, limits->min_interfaces, limits->max_interfaces);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    if (app_config->network.max_ips_per_interface < limits->min_ips_per_interface ||
        app_config->network.max_ips_per_interface > limits->max_ips_per_interface) {
        add_go_message(readiness.messages, &msg_count, "No-Go", 
            "Invalid max_ips_per_interface: %zu (must be between %zu and %zu)",
            app_config->network.max_ips_per_interface, limits->min_ips_per_interface, limits->max_ips_per_interface);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    // Validate name and address length limits
    if (app_config->network.max_interface_name_length < limits->min_interface_name_length ||
        app_config->network.max_interface_name_length > limits->max_interface_name_length) {
        add_go_message(readiness.messages, &msg_count, "No-Go", 
            "Invalid interface name length: %zu (must be between %zu and %zu)",
            app_config->network.max_interface_name_length, limits->min_interface_name_length, limits->max_interface_name_length);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    if (app_config->network.max_ip_address_length < limits->min_ip_address_length ||
        app_config->network.max_ip_address_length > limits->max_ip_address_length) {
        add_go_message(readiness.messages, &msg_count, "No-Go", 
            "Invalid IP address length: %zu (must be between %zu and %zu)",
            app_config->network.max_ip_address_length, limits->min_ip_address_length, limits->max_ip_address_length);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    // Validate port range
    if (app_config->network.start_port < limits->min_port ||
        app_config->network.start_port > limits->max_port ||
        app_config->network.end_port < limits->min_port ||
        app_config->network.end_port > limits->max_port ||
        app_config->network.start_port >= app_config->network.end_port) {
        add_go_message(readiness.messages, &msg_count, "No-Go", 
            "Invalid port range: %d-%d (must be between %d and %d, start < end)",
            app_config->network.start_port, app_config->network.end_port, limits->min_port, limits->max_port);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    // Validate reserved ports array
    if (app_config->network.reserved_ports_count > 0 && !app_config->network.reserved_ports) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "Reserved ports array is NULL but count > 0");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }

    add_go_message(readiness.messages, &msg_count, "Go", "Network configuration validated");
    
    network_info_t* network_info = get_network_info();
    if (!network_info) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "Failed to get network information");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }
    
    if (network_info->count == 0) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "No network interfaces available");
        free_network_info(network_info);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        free_readiness_messages(&readiness);
        return readiness;
    }
    
    add_go_message(readiness.messages, &msg_count, "Go", "%d network interfaces available", network_info->count);
    
    int json_interfaces_count = 0;
    if (app_config && app_config->network.available_interfaces && app_config->network.available_interfaces_count > 0) {
        json_interfaces_count = app_config->network.available_interfaces_count;
        
        if (json_interfaces_count > 0) {
            add_go_message(readiness.messages, &msg_count, "Go", "%d network interfaces configured:", json_interfaces_count);
        } else {
            add_go_message(readiness.messages, &msg_count, "No-Go", "No network interfaces found in JSON configuration");
        }
        
        for (size_t i = 0; i < app_config->network.available_interfaces_count; i++) {
            if (app_config->network.available_interfaces[i].interface_name) {
                const char* interface_name = app_config->network.available_interfaces[i].interface_name;
                bool is_available = app_config->network.available_interfaces[i].available;
                
                if (is_available) {
                    add_go_message(readiness.messages, &msg_count, "Go", "Available: %s is enabled", interface_name);
                } else {
                    add_go_message(readiness.messages, &msg_count, "No-Go", "Available: %s is disabled", interface_name);
                }
            }
        }
    } else {
        add_go_message(readiness.messages, &msg_count, "No-Go", "No network interfaces found in JSON configuration");
    }
    
    int up_interfaces = 0;
    for (int i = 0; i < network_info->count; i++) {
        interface_t* interface = &network_info->interfaces[i];
        bool is_up = interface->ip_count > 0;
        bool is_available = true;
        bool is_configured = is_interface_configured(app_config, interface->name, &is_available);
        
        const char* config_status = is_configured ? 
            (is_available ? "enabled in config" : "disabled in config") :
            "not in config - enabled by default";
        
        if (is_up) {
            if (is_available) {
                up_interfaces++;
                add_go_message(readiness.messages, &msg_count, "Go", "Interface %s is up (%s)", interface->name, config_status);
            } else {
                add_go_message(readiness.messages, &msg_count, "No-Go", "Interface %s is up but %s", interface->name, config_status);
            }
        } else {
            add_go_message(readiness.messages, &msg_count, "No-Go", "Interface %s is down (%s)", interface->name, config_status);
        }
    }
    
    if (up_interfaces > 0) {
        add_decision_message(readiness.messages, &msg_count, "Go For Launch of Network Subsystem (%d interfaces ready)", up_interfaces);
        readiness.ready = true;
    } else {
        add_decision_message(readiness.messages, &msg_count, "No-Go For Launch of Network Subsystem (no interfaces ready)");
        readiness.ready = false;
    }
    
    free_network_info(network_info);
    readiness.messages[msg_count] = NULL;
    readiness.subsystem = "Network";
    
    return readiness;
}