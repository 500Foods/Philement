/*
 * Launch Terminal Subsystem
 * 
 * This module handles the initialization of the terminal subsystem.
 * It provides functions for checking readiness and launching the terminal.
 * 
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 * - WebSockets subsystem must be initialized and ready
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "launch.h"
#include "../config/config.h"
#include "../utils/utils_logging.h"

// External declarations
extern volatile sig_atomic_t terminal_system_shutdown;

// Check if the terminal subsystem is ready to launch
LaunchReadiness check_terminal_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    const AppConfig* config = get_app_config();
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Start with header
    readiness.messages[0] = strdup("Terminal");
    size_t msg_index = 1;
    bool is_ready = true;

    // Check dependencies first
    if (!config->webserver.enable_ipv4 && !config->webserver.enable_ipv6) {
        readiness.messages[msg_index++] = strdup("  No-Go:   WebServer Not Enabled");
        readiness.messages[msg_index++] = strdup("  Reason:  Terminal Requires WebServer (IPv4 or IPv6)");
        is_ready = false;
    }

    if (!config->websocket.enabled) {
        readiness.messages[msg_index++] = strdup("  No-Go:   WebSocket Not Enabled");
        readiness.messages[msg_index++] = strdup("  Reason:  Terminal Requires WebSocket");
        is_ready = false;
    }

    // Check if terminal is enabled
    if (!config->terminal.enabled) {
        readiness.messages[msg_index++] = strdup("  No-Go:   Terminal System Disabled");
        readiness.messages[msg_index++] = strdup("  Reason:  Disabled in Configuration");
        is_ready = false;
    } else {
        // Validate required strings
        if (!config->terminal.web_path) {
            readiness.messages[msg_index++] = strdup("  No-Go:   Missing Web Path");
            readiness.messages[msg_index++] = strdup("  Reason:  Web Path Must Be Set");
            is_ready = false;
        }
        
        if (!config->terminal.shell_command) {
            readiness.messages[msg_index++] = strdup("  No-Go:   Missing Shell Command");
            readiness.messages[msg_index++] = strdup("  Reason:  Shell Command Must Be Set");
            is_ready = false;
        }
        
        // Validate numeric ranges
        if (config->terminal.max_sessions < 1 || config->terminal.max_sessions > 100) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid Max Sessions: %d", 
                    config->terminal.max_sessions);
            readiness.messages[msg_index++] = strdup(msg);
            readiness.messages[msg_index++] = strdup("  Reason:  Must Be Between 1 and 100");
            is_ready = false;
        }
        
        if (config->terminal.idle_timeout_seconds < 60 || 
            config->terminal.idle_timeout_seconds > 3600) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid Idle Timeout: %d", 
                    config->terminal.idle_timeout_seconds);
            readiness.messages[msg_index++] = strdup(msg);
            readiness.messages[msg_index++] = strdup("  Reason:  Must Be Between 60 and 3600 Seconds");
            is_ready = false;
        }
    }

    // Add final decision message
    if (is_ready) {
        readiness.messages[msg_index++] = strdup("  Go:      Terminal System Ready");
    } else {
        readiness.messages[msg_index++] = strdup("  Decide:  No-Go For Launch of Terminal");
    }
    
    // Null terminate the message list
    readiness.messages[msg_index] = NULL;
    readiness.ready = is_ready;
    
    return readiness;
}

// Launch the terminal subsystem
int launch_terminal_subsystem(void) {
    // Reset shutdown flag
    terminal_system_shutdown = 0;
    
    // Get configuration
    const AppConfig* config = get_app_config();
    if (!config || !config->terminal.enabled) {
        return -1;
    }
    
    // Initialize terminal system
    log_this("Terminal", "Initializing terminal subsystem", LOG_LEVEL_STATE);
    
    return 0;
}