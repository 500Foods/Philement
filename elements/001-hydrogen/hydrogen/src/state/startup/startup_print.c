/*
 * Print Subsystem Startup Handler
 * 
 * This module handles the initialization of the print queue subsystem.
 * It manages 3D printer job queues and print processing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "../state.h"
#include "../../logging/logging.h"
#include "../../queue/queue.h"
#include "../../print/print_queue_manager.h"

// Initialize print queue system
// Requires: Logging system, Queue system
//
// The print queue system manages 3D printer jobs:
// 1. Job queuing and prioritization
// 2. Print status tracking
// 3. Job history management
// 4. Resource allocation
int init_print_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t print_system_shutdown;

    // Prevent initialization during any shutdown state
    if (server_stopping || print_system_shutdown) {
        log_this("Initialization", "Cannot initialize Print system during shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this("Initialization", "Cannot initialize Print system outside startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    // Check if print queue is enabled in configuration
    if (!app_config->print_queue.enabled) {
        log_this("Initialization", "Print Queue system disabled in configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    // Initialize the print queue
    if (!init_print_queue()) {
        log_this("Initialization", "Failed to initialize print queue", LOG_LEVEL_ERROR);
        return 0;
    }

    // Start print queue manager thread
    if (pthread_create(&print_queue_thread, NULL, print_queue_manager, NULL) != 0) {
        log_this("Initialization", "Failed to start print queue manager thread", LOG_LEVEL_ERROR);
        // TODO: Implement cleanup for print queue
        return 0;
    }

    log_this("Initialization", "Print Queue system initialized successfully", LOG_LEVEL_STATE);
    return 1;
}