/*
 * Launch Thread Subsystem
 * 
 * This module handles the initialization of the thread management subsystem.
 * As a core subsystem, it provides thread tracking and metrics for all other subsystems.
 * 
 * Dependencies:
 * - Subsystem Registry must be initialized
 * - No other dependencies (this is a fundamental subsystem)
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "launch.h"
#include "launch-threads.h"
#include "../threads/threads.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../state/registry/subsystem_registry.h"

// External declarations for thread tracking
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_stopping;

// Check if the thread subsystem is ready to launch
LaunchReadiness check_threads_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Initialize as ready - this is a fundamental subsystem
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Threads");
    readiness.messages[1] = strdup("  Go:      Thread Management System Ready");
    readiness.messages[2] = strdup("  Decide:  Go For Launch of Thread Management");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Launch thread management system
int launch_threads_subsystem(void) {
    // Prevent initialization during any shutdown state
    if (server_stopping) {
        log_this("Initialization", "Cannot initialize thread management during shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this("Initialization", "Cannot initialize thread management outside startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    // Initialize all thread tracking structures
    init_service_threads(&logging_threads);
    init_service_threads(&web_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_server_threads);
    init_service_threads(&print_threads);

    log_this("Initialization", "Thread management structures initialized", LOG_LEVEL_STATE);
    
    // Register the main thread
    pthread_t main_thread = pthread_self();
    add_service_thread(&logging_threads, main_thread);
    
    log_this("Initialization", "Main thread registered", LOG_LEVEL_STATE);
    log_this("Initialization", "Thread management subsystem initialized successfully", LOG_LEVEL_STATE);
    
    return 1;
}