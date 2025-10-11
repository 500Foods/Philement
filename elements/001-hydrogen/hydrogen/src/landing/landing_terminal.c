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

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations
extern volatile sig_atomic_t terminal_system_shutdown;

// Check if the terminal subsystem is ready to land
LaunchReadiness check_terminal_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_TERMINAL;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup(SR_TERMINAL);
    
    // Check if terminal is actually running
    if (!is_subsystem_running_by_name(SR_TERMINAL)) {
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
    log_this(SR_TERMINAL, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_TERMINAL, "LANDING: TERMINAL", LOG_LEVEL_STATE, 0);
    
    // Signal shutdown
    terminal_system_shutdown = 1;
    log_this(SR_TERMINAL, "Signaled Terminal to stop", LOG_LEVEL_STATE, 0);
    
    // Cleanup resources
    // Additional cleanup will be added as needed
    
    log_this(SR_TERMINAL, "Terminal shutdown complete", LOG_LEVEL_STATE, 0);
    
    return 1;  // Always successful
}
