/*
 * Restart Functionality for Hydrogen Server
 * 
 * This module handles the in-process restart functionality,
 * allowing the server to reload its configuration and reinitialize
 * while maintaining the same process.
 */

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Project headers
#include "shutdown_internal.h"
#include "../../logging/logging.h"
#include "../startup/startup.h"  // For startup_hydrogen()

// External declaration for startup function
extern int startup_hydrogen(const char* config_path);

// Restart the application after graceful shutdown
// Implements an in-process restart by calling startup_hydrogen directly
int restart_hydrogen(const char* config_path) {
    // Log restart attempt with the specific message the test is looking for
    log_this("Restart", "Initiating in-process restart", LOG_LEVEL_STATE);
    
    // Increment restart counter
    restart_count++;
    log_this("Restart", "Restart count: %d", LOG_LEVEL_STATE, restart_count);
    
    // Reset server state flags before restart
    server_starting = 1;
    server_running = 0;
    server_stopping = 0;
    
    // Clear restart flag for next time
    restart_requested = 0;
    
    // Mark signal handler flags for reset on next signal
    handler_flags_reset_needed = 1;
    
    // Flush all output before restart
    fflush(stdout);
    fflush(stderr);
    
    // Call startup_hydrogen directly for in-process restart
    // Pass the provided config_path, or NULL to use normal config discovery
    log_this("Restart", "Calling startup_hydrogen() for in-process restart", LOG_LEVEL_STATE);
    int result = startup_hydrogen(config_path);
    
    if (result) {
        log_this("Restart", "In-process restart successful", LOG_LEVEL_STATE);
        log_this("Restart", "Restart completed successfully", LOG_LEVEL_STATE);
        return 1;
    } else {
        log_this("Restart", "In-process restart failed", LOG_LEVEL_ERROR);
        return 0;
    }
}