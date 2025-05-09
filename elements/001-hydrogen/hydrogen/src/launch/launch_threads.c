/**
 * @file launch_threads.c
 * @brief Thread subsystem launch implementation
 *
 * This module handles the initialization of the thread management subsystem.
 * As a core subsystem, it provides thread tracking and metrics for all other subsystems.
 * 
 * Dependencies:
 * - Registry must be initialized
 * - No other dependencies (this is a fundamental subsystem)
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#include "launch.h"
#include "launch_threads.h"
#include "../threads/threads.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations for thread tracking
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// External system state flags
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_running;

// Registry ID and cached readiness state
static int thread_subsystem_id = -1;
static LaunchReadiness cached_readiness = {0};
static bool readiness_cached = false;

// Forward declarations
static void clear_cached_readiness(void);
static void register_threads(void);

// Helper to clear cached readiness
static void clear_cached_readiness(void) {
    if (readiness_cached && cached_readiness.messages) {
        free_readiness_messages(&cached_readiness);
        readiness_cached = false;
    }
}

// Get cached readiness result
LaunchReadiness get_threads_readiness(void) {
    if (readiness_cached) {
        return cached_readiness;
    }
    
    // Perform fresh check and cache result
    cached_readiness = check_threads_launch_readiness();
    readiness_cached = true;
    return cached_readiness;
}

// Register the thread subsystem with the registry
static void register_threads(void) {
    // Always register during readiness check if not already registered
    if (thread_subsystem_id < 0) {
        thread_subsystem_id = register_subsystem("Threads", NULL, NULL, NULL, NULL, NULL);
    }
}

/*
 * Check if the thread subsystem is ready to launch
 * 
 * This function performs high-level readiness checks for the thread management
 * subsystem, ensuring all prerequisites are met before launch.
 * 
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Note: Prefer using get_threads_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_threads_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Threads";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    int msg_index = 0;
    bool ready = true;
    
    // First message is subsystem name
    readiness.messages[msg_index++] = strdup("Threads");
    
    // Register with registry if not already registered
    register_threads();
    
    // Check system state
    if (server_stopping || (!server_starting && !server_running)) {
        readiness.messages[msg_index++] = strdup("  No-Go:   System State (not ready for threads)");
        ready = false;
    } else {
        readiness.messages[msg_index++] = strdup("  Go:      System State Ready");
    }
    
    // Check if already running
    if (is_subsystem_running_by_name("Threads")) {
        readiness.messages[msg_index++] = strdup("  No-Go:   Thread subsystem already running");
        ready = false;
    } else {
        readiness.messages[msg_index++] = strdup("  Go:      Thread Management System Ready");
    }
    
    // Final decision
    readiness.messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of Thread Management" :
        "  Decide:  No-Go For Launch of Thread Management");
    readiness.messages[msg_index] = NULL;
    
    if (!ready) {
        readiness.messages[msg_index] = NULL;
        free_readiness_messages(&readiness);
    }
    
    readiness.ready = ready;
    return readiness;
}

/**
 * Launch the thread management subsystem
 * 
 * This function coordinates the launch of the thread management subsystem by:
 * 1. Using the standard launch formatting
 * 2. Initializing thread tracking structures
 * 3. Registering the main thread
 * 4. Updating the subsystem registry with the result
 * 
 * @return 1 if thread management was successfully launched, 0 on failure
 */
int launch_threads_subsystem(void) {
    // Begin LAUNCH: THREADS section
    log_this("Threads", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Threads", "LAUNCH: THREADS", LOG_LEVEL_STATE);
    
    // Clear any cached readiness before checking final state
    clear_cached_readiness();
    // Step 1: Verify system state
    if (server_stopping) {
        log_this("Threads", "Cannot initialize thread management during shutdown", LOG_LEVEL_STATE);
        return 0;
    }
    
    if (!server_starting && !server_running) {
        log_this("Threads", "Cannot initialize thread management outside startup/running phase", LOG_LEVEL_STATE);
        return 0;
    }
    
    // Step 2: Initialize thread tracking structures
    log_this("Threads", "  Step 1: Initializing thread tracking structures", LOG_LEVEL_STATE);
    init_service_threads(&logging_threads);
    init_service_threads(&webserver_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_server_threads);
    init_service_threads(&print_threads);
    
    // Step 3: Register the main thread
    log_this("Threads", "  Step 2: Registering main thread", LOG_LEVEL_STATE);
    pthread_t main_thread = pthread_self();
    add_service_thread(&logging_threads, main_thread);
    
    // Step 4: Update registry and verify state
    log_this("Threads", "  Step 3: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("Threads", true);
    
    SubsystemState final_state = get_subsystem_state(thread_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("Threads", "LAUNCH: THREADS - Successfully launched and running", LOG_LEVEL_STATE);
        return 1;
    } else {
        log_this("Threads", "LAUNCH: THREADS - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }
}