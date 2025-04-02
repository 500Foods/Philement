/*
 * Launch Terminal Subsystem
 * 
 * This module handles the initialization of the terminal subsystem.
 * It provides functions for checking readiness and launching the terminal.
 * 
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 * - WebSockets subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_terminal.c
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "launch.h"
#include "../utils/utils_logging.h"

// External declarations
extern volatile sig_atomic_t terminal_system_shutdown;

// Check if the terminal subsystem is ready to launch
LaunchReadiness check_terminal_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // For now, mark as not ready as it's under development
    readiness.ready = false;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Terminal");
    readiness.messages[1] = strdup("  No-Go:   Terminal System Not Ready");
    readiness.messages[2] = strdup("  Reason:  Under Development");
    readiness.messages[3] = strdup("  Decide:  No-Go For Launch of Terminal");
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Launch the terminal subsystem
int launch_terminal_subsystem(void) {
    // Reset shutdown flag
    terminal_system_shutdown = 0;
    
    // Launch terminal system
    // Currently a placeholder as system is under development
    return 0;  // Return 0 for now as system is under development
}