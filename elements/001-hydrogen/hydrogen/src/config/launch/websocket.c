/*
 * WebSocket Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the websocket subsystem
 * are satisfied before attempting to initialize it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "launch.h"
#include "../../logging/logging.h"
#include "../../config/config.h"
#include "../../state/registry/subsystem_registry.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t websocket_server_shutdown;

// Static message array for readiness check results
static const char* websocket_messages[15]; // Up to 15 messages plus NULL terminator
static int message_count = 0;

// Add a message to the messages array
static void add_message(const char* format, ...) {
    if (message_count >= 14) return; // Leave room for NULL terminator
    
    va_list args;
    va_start(args, format);
    char* message = NULL;
    vasprintf(&message, format, args);
    va_end(args);
    
    if (message) {
        websocket_messages[message_count++] = message;
        websocket_messages[message_count] = NULL; // Ensure NULL termination
    }
}

// Check if the websocket subsystem is ready to launch
LaunchReadiness check_websocket_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 15; i++) {
        websocket_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("WebSocketServer");
    
    // Check 1: Configuration loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        add_message("  Go:      Configuration (loaded)");
    } else {
        add_message("  No-Go:   Configuration (not loaded)");
        overall_readiness = false;
    }
    
    // Only proceed with other checks if configuration is loaded
    if (config_loaded) {
        // Check 2: Enabled in configuration
        if (app_config->websocket.enabled) {
            add_message("  Go:      Enabled (in configuration)");
        } else {
            add_message("  No-Go:   Enabled (disabled in configuration)");
            overall_readiness = false;
        }
        
        // Check 3: Port configuration
        if (app_config->websocket.port > 0 && app_config->websocket.port < 65536) {
            add_message("  Go:      Port Configuration (valid: %d)", app_config->websocket.port);
        } else {
            add_message("  No-Go:   Port Configuration (invalid: %d)", app_config->websocket.port);
            overall_readiness = false;
        }
        
        // Check 4: Protocol configuration
        if (app_config->websocket.protocol && strlen(app_config->websocket.protocol) > 0) {
            add_message("  Go:      Protocol Configuration (valid)");
        } else {
            add_message("  No-Go:   Protocol Configuration (invalid or missing)");
            overall_readiness = false;
        }
        
        // Check 5: Not in shutdown state
        if (!websocket_server_shutdown) {
            add_message("  Go:      Shutdown State (not in shutdown)");
        } else {
            add_message("  No-Go:   Shutdown State (in shutdown)");
            overall_readiness = false;
        }
        
        // Check 6: Logging subsystem dependency
        if (is_subsystem_running_by_name("Logging")) {
            add_message("  Go:      Dependency (Logging subsystem running)");
        } else {
            add_message("  No-Go:   Dependency (Logging subsystem not running)");
            overall_readiness = false;
        }
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of WebSocketServer Subsystem");
    } else {
        add_message("  Decide:  No-Go For Launch of WebSocketServer Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "WebSocketServer",
        .ready = overall_readiness,
        .messages = websocket_messages
    };
    
    return readiness;
}