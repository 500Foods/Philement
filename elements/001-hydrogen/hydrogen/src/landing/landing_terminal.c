/*
 * Landing Terminal Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the terminal subsystem.
 * It provides functions for:
 * - Checking terminal landing readiness
 * - Managing terminal shutdown
 * - Cleaning up terminal resources
 * 
 * Dependencies:
 * - Must coordinate with WebServer for shutdown
 * - Must coordinate with WebSocket for shutdown
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations
extern volatile sig_atomic_t terminal_system_shutdown;

// Check if the terminal subsystem is ready to land
LaunchReadiness check_terminal_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Terminal";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Terminal");
    
    // Check if terminal is actually running
    if (!is_subsystem_running_by_name("Terminal")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Terminal not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Terminal");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check WebServer status
    bool webserver_ready = is_subsystem_running_by_name("WebServer");
    if (!webserver_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebServer subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Terminal");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check WebSocket status
    bool websocket_ready = is_subsystem_running_by_name("WebSocket");
    if (!websocket_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebSocket subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Terminal");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // All dependencies are ready
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      WebServer ready for shutdown");
    readiness.messages[2] = strdup("  Go:      WebSocket ready for shutdown");
    readiness.messages[3] = strdup("  Go:      Terminal ready for cleanup");
    readiness.messages[4] = strdup("  Decide:  Go For Landing of Terminal");
    readiness.messages[5] = NULL;
    
    return readiness;
}

// Land the terminal subsystem
int land_terminal_subsystem(void) {
    log_this("Terminal", "Beginning Terminal shutdown sequence", LOG_LEVEL_STATE);
    bool success = true;
    
    // Signal shutdown
    terminal_system_shutdown = 1;
    log_this("Terminal", "Signaled Terminal to stop", LOG_LEVEL_STATE);
    
    // Cleanup resources
    // Additional cleanup will be added as needed
    
    log_this("Terminal", "Terminal shutdown complete", LOG_LEVEL_STATE);
    
    return success ? 1 : 0;  // Return 1 for success, 0 for failure
}