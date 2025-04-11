/*
 * Launch Print Subsystem
 * 
 * This module handles the initialization of the print subsystem.
 * It provides functions for checking readiness and launching the print queue.
 * 
 * The print subsystem manages:
 * - Print job queuing
 * - Print thread management
 * - Print resource allocation
 * 
 * Note: Shutdown functionality has been moved to landing/landing_print.c
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "launch.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"

// External declarations
extern ServiceThreads print_threads;
extern pthread_t print_queue_thread;
extern volatile sig_atomic_t print_system_shutdown;
extern AppConfig* app_config;

// Check if the print subsystem is ready to launch
LaunchReadiness check_print_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("Print");
    
    // Register dependency on Network subsystem for remote printing
    int print_id = get_subsystem_id_by_name("Print");
    if (print_id >= 0) {
        if (!add_dependency_from_launch(print_id, "Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Failed to register Network dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network dependency registered");
        
        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Network subsystem not running");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network subsystem running");
    }
    
    // Check configuration
    if (!app_config || !app_config->print.enabled) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Print queue disabled in configuration");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Print queue enabled in configuration");
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of Print Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Launch the print subsystem
int launch_print_subsystem(void) {
    // Reset shutdown flag
    print_system_shutdown = 0;
    
    // Initialize print queue thread structure
    init_service_threads(&print_threads);
    
    // Additional initialization as needed
    return 1;
}