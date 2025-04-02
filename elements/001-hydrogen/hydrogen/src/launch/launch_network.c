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

// Network subsystem launch function
int launch_network_subsystem(void) {
    log_this("Network", "Initializing network subsystem", LOG_LEVEL_STATE);
    // Network subsystem is already initialized by the get_network_info() function
    // No additional initialization is needed
    return 1;  // Return 1 for success as per standard
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

// Check if an interface is configured in the Available section
static bool is_interface_configured(const AppConfig* app_config, const char* interface_name, bool* is_available) {
    if (!app_config) {
        *is_available = true;  // Default to available if no config
        return false;          // Not explicitly configured
    }
    
    // Check if the available_interfaces array is NULL
    if (!app_config->network.available_interfaces || app_config->network.available_interfaces_count == 0) {
        *is_available = true;  // Default to available if no available_interfaces
        return false;          // Not explicitly configured
    }
    
    // Check if the interface is in the available_interfaces array
    for (size_t i = 0; i < app_config->network.available_interfaces_count; i++) {
        if (app_config->network.available_interfaces[i].interface_name) {
            if (strcmp(interface_name, app_config->network.available_interfaces[i].interface_name) == 0) {
                // Interface is explicitly configured
                *is_available = app_config->network.available_interfaces[i].available;
                return true;  // Explicitly configured
            }
        }
    }
    
    // If not in the configuration, it's not explicitly configured
    *is_available = true;  // Default to available
    return false;          // Not explicitly configured
}


// Check network subsystem launch readiness
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
    
    // Register dependency on Threads subsystem
    int network_id = get_subsystem_id_by_name("Network");
    if (network_id >= 0) {
        if (!add_dependency_from_launch(network_id, "Threads")) {
            add_go_message(readiness.messages, &msg_count, "No-Go", "Failed to register Thread dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            readiness.subsystem = "Network";
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
        return readiness;
    }
    
    if (!server_starting && !server_running) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "System not in startup or running state");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        return readiness;
    }
    
    const AppConfig* app_config = get_app_config();
    if (!app_config) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "Configuration not loaded");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        return readiness;
    }
    
    network_info_t* network_info = get_network_info();
    if (!network_info) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "Failed to get network information");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
        return readiness;
    }
    
    if (network_info->count == 0) {
        add_go_message(readiness.messages, &msg_count, "No-Go", "No network interfaces available");
        free_network_info(network_info);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        readiness.subsystem = "Network";
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