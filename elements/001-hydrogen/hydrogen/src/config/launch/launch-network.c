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
#include "../../state/state.h"
#include "../../state/shutdown/shutdown.h"
#include "../../config/config.h"
#include "../../network/network.h"
#include "../../logging/logging.h"
#include "launch.h"
#include "launch-network.h"

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;

// Network subsystem shutdown flag
volatile int network_system_shutdown = 0;

// Network subsystem initialization function
int init_network_subsystem(void) {
    log_this("Network", "Initializing network subsystem", LOG_LEVEL_STATE);
    // Network subsystem is already initialized by the get_network_info() function
    // No additional initialization is needed
    return 0;
}

// Network subsystem shutdown function
void shutdown_network_subsystem(void) {
    log_this("Network", "Shutting down network subsystem", LOG_LEVEL_STATE);
    // No specific shutdown actions needed for network subsystem
}

// Static arrays for readiness check messages
#define NETWORK_MAX_MESSAGES 50
#define NETWORK_MESSAGE_LENGTH 256
static char network_messages[NETWORK_MAX_MESSAGES][NETWORK_MESSAGE_LENGTH];
static const char* message_pointers[NETWORK_MAX_MESSAGES + 1]; // +1 for NULL terminator
static int message_count = 0;

// Add a header message (subsystem name only)
static void add_header_message(const char* subsystem_name) {
    if (message_count < NETWORK_MAX_MESSAGES) {
        snprintf(network_messages[message_count], NETWORK_MESSAGE_LENGTH, "%s", subsystem_name);
        message_pointers[message_count] = network_messages[message_count];
        message_count++;
    }
}

// Add a Go/No-Go message with proper indentation
static void add_go_message(const char* prefix, const char* format, ...) {
    if (message_count < NETWORK_MAX_MESSAGES) {
        // Ensure we have enough space for the formatted message
        char temp[NETWORK_MESSAGE_LENGTH - 20]; // Reserve space for prefix and formatting
        
        va_list args;
        va_start(args, format);
        vsnprintf(temp, sizeof(temp), format, args);
        va_end(args);
        
        // Now format the full message with prefix, adjusting spacing based on prefix length
        if (strcmp(prefix, "No-Go") == 0) {
            // "No-Go" is 3 characters longer than "Go", so use 3 fewer spaces
            snprintf(network_messages[message_count], NETWORK_MESSAGE_LENGTH, 
                     "  %s:   %s", prefix, temp);
        } else {
            snprintf(network_messages[message_count], NETWORK_MESSAGE_LENGTH, 
                     "  %s:      %s", prefix, temp);
        }
        
        message_pointers[message_count] = network_messages[message_count];
        message_count++;
    }
}

// Add a decision message with proper indentation
static void add_decision_message(const char* format, ...) {
    if (message_count < NETWORK_MAX_MESSAGES) {
        // Calculate the size needed for the prefix part
        const char* prefix = "Decide";
        size_t prefix_len = strlen(prefix) + 10; // "  Decide:  " is about 10 chars plus prefix length
        
        // Ensure we have enough space for the formatted message
        char temp[NETWORK_MESSAGE_LENGTH - prefix_len];
        
        va_list args;
        va_start(args, format);
        vsnprintf(temp, sizeof(temp), format, args);
        va_end(args);
        
        // Now format the full message with prefix
        snprintf(network_messages[message_count], NETWORK_MESSAGE_LENGTH, 
                 "  %s:  %s", prefix, temp);
        
        message_pointers[message_count] = network_messages[message_count];
        message_count++;
    }
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
    bool overall_readiness = false;
    message_count = 0;
    
    // Add the subsystem name as the first message (header)
    add_header_message("Network");
    
    // Check system state
    if (server_stopping || web_server_shutdown) {
        add_go_message("No-Go", "System shutdown in progress");
        
        // Ensure the message_pointers array is NULL-terminated
        message_pointers[message_count] = NULL;
        
        // Build the readiness structure
        LaunchReadiness readiness = {
            .subsystem = "Network",
            .ready = false,
            .messages = message_pointers
        };
        return readiness;
    }
    
    // Check if system is in startup or running state
    if (!server_starting && !server_running) {
        add_go_message("No-Go", "System not in startup or running state");
        
        // Ensure the message_pointers array is NULL-terminated
        message_pointers[message_count] = NULL;
        
        // Build the readiness structure
        LaunchReadiness readiness = {
            .subsystem = "Network",
            .ready = false,
            .messages = message_pointers
        };
        return readiness;
    }
    
    // Check if configuration is loaded
    const AppConfig* app_config = get_app_config();
    if (!app_config) {
        add_go_message("No-Go", "Configuration not loaded");
        
        // Ensure the message_pointers array is NULL-terminated
        message_pointers[message_count] = NULL;
        
        // Build the readiness structure
        LaunchReadiness readiness = {
            .subsystem = "Network",
            .ready = false,
            .messages = message_pointers
        };
        return readiness;
    }
    
    // Get network information
    network_info_t* network_info = get_network_info();
    if (!network_info) {
        add_go_message("No-Go", "Failed to get network information");
        
        // Ensure the message_pointers array is NULL-terminated
        message_pointers[message_count] = NULL;
        
        // Build the readiness structure
        LaunchReadiness readiness = {
            .subsystem = "Network",
            .ready = false,
            .messages = message_pointers
        };
        return readiness;
    }
    
    // Check if interfaces are available
    if (network_info->count == 0) {
        add_go_message("No-Go", "No network interfaces available");
        free_network_info(network_info);
        
        // Ensure the message_pointers array is NULL-terminated
        message_pointers[message_count] = NULL;
        
        // Build the readiness structure
        LaunchReadiness readiness = {
            .subsystem = "Network",
            .ready = false,
            .messages = message_pointers
        };
        return readiness;
    }
    
    add_go_message("Go", "%d network interfaces available", network_info->count);
    
    // Count and list interfaces from the JSON configuration
    int json_interfaces_count = 0;
    if (app_config) {
        if (app_config->network.available_interfaces && app_config->network.available_interfaces_count > 0) {
            json_interfaces_count = app_config->network.available_interfaces_count;
            
            // Report the count of interfaces found in JSON
            if (json_interfaces_count > 0) {
                add_go_message("Go", "%d network interfaces configured:", json_interfaces_count);
            } else {
                add_go_message("No-Go", "No network interfaces found in JSON configuration");
            }
            for (size_t i = 0; i < app_config->network.available_interfaces_count; i++) {
                if (app_config->network.available_interfaces[i].interface_name) {
                    const char* interface_name = app_config->network.available_interfaces[i].interface_name;
                    bool is_available = app_config->network.available_interfaces[i].available;
                    
                    if (is_available) {
                        add_go_message("Go", "Available: %s is enabled", interface_name);
                    } else {
                        add_go_message("No-Go", "Available: %s is disabled", interface_name);
                    }
                }
            }
        } else {
            // No interfaces found in JSON configuration
            add_go_message("No-Go", "No network interfaces found in JSON configuration");
        }
    } else {
        // No app_config available
        add_go_message("No-Go", "No configuration loaded, cannot check JSON configuration");
    }
    
    // Check each interface
    int up_interfaces = 0;
    for (int i = 0; i < network_info->count; i++) {
        interface_t* interface = &network_info->interfaces[i];
        bool is_up = interface->ip_count > 0;
        
        // Check if the interface is configured in the Available section
        bool is_available = true;
        bool is_configured = is_interface_configured(app_config, interface->name, &is_available);
        
        // Get the configuration status message based on the result
        const char* config_status;
        if (is_configured) {
            if (is_available) {
                config_status = "enabled in config";
            } else {
                config_status = "disabled in config";
            }
        } else {
            config_status = "not in config - enabled by default";
        }
        
        if (is_up) {
            if (is_available) {
                // Interface is up and available in config (or not in config but default available)
                up_interfaces++;
                add_go_message("Go", "Interface %s is up (%s)", interface->name, config_status);
            } else {
                // Interface is up but disabled in config - this is a No-Go condition
                add_go_message("No-Go", "Interface %s is up but %s", interface->name, config_status);
            }
        } else {
            // Interface is down
            add_go_message("No-Go", "Interface %s is down (%s)", interface->name, config_status);
        }
    }
    
    // Make final decision
    // An interface is considered "Go" if it's up AND not disabled in the config
    if (up_interfaces > 0) {
        add_decision_message("Go For Launch of Network Subsystem (%d interfaces ready)", up_interfaces);
        overall_readiness = true;
    } else {
        add_decision_message("No-Go For Launch of Network Subsystem (no interfaces ready)");
        overall_readiness = false;
    }
    
    // Free network information
    free_network_info(network_info);
    
    // Ensure the message_pointers array is NULL-terminated
    message_pointers[message_count] = NULL;
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Network",
        .ready = overall_readiness,
        .messages = message_pointers
    };
    
    return readiness;
}