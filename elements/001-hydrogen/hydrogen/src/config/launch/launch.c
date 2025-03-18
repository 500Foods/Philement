/*
 * Launch Readiness System
 * 
 * This module coordinates pre-launch checks for all subsystems.
 * It ensures that dependencies are met and reports readiness status.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "launch.h"
#include "../../logging/logging.h"

// Forward declarations for subsystem readiness checks
extern LaunchReadiness check_logging_launch_readiness(void);
// Add additional checks here as they are implemented:
// extern LaunchReadiness check_webserver_launch_readiness(void);
// extern LaunchReadiness check_websocket_launch_readiness(void);
// extern LaunchReadiness check_print_launch_readiness(void);
// etc.

// Log all messages from a readiness check
static void log_readiness_messages(const LaunchReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    // Log subsystem header
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "Subsystem: %s", LOG_LEVEL_STATE, readiness->subsystem);
    
    // Log each message
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        int level = LOG_LEVEL_STATE;
        
        // Use appropriate log level based on the message content
        if (strstr(readiness->messages[i], "No-Go") != NULL) {
            level = LOG_LEVEL_ALERT;
        }
        
        log_this("Launch", "  %s", level, readiness->messages[i]);
    }
    
    // Log overall status
    log_this("Launch", "Status: %s", readiness->ready ? LOG_LEVEL_STATE : LOG_LEVEL_ALERT, 
             readiness->ready ? "Ready" : "Not Ready");
}

// Check if all subsystems are ready to launch
bool check_all_launch_readiness(void) {
    bool all_ready = true;
    
    // Begin LAUNCH READINESS logging section
    log_group_begin();
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH READINESS", LOG_LEVEL_STATE);
    
    // Check logging subsystem
    LaunchReadiness logging_readiness = check_logging_launch_readiness();
    log_readiness_messages(&logging_readiness);
    all_ready &= logging_readiness.ready;
    
    // Add additional subsystem checks here as they are implemented
    // LaunchReadiness webserver_readiness = check_webserver_launch_readiness();
    // log_readiness_messages(&webserver_readiness);
    // all_ready &= webserver_readiness.ready;
    // 
    // LaunchReadiness websocket_readiness = check_websocket_launch_readiness();
    // log_readiness_messages(&websocket_readiness);
    // all_ready &= websocket_readiness.ready;
    // etc.
    
    // Log overall launch status
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "Overall Launch Status: %s", 
             all_ready ? LOG_LEVEL_STATE : LOG_LEVEL_ALERT,
             all_ready ? "Go for Launch" : "No-Go for Launch");
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_group_end();
    
    return all_ready;
}