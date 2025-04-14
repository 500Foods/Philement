/**
 * @file landing_threads.c
 * @brief Thread subsystem landing (shutdown) implementation
 *
 * This module handles the landing (shutdown) sequence for the thread management subsystem.
 * As a core subsystem, it must ensure all thread tracking structures are properly cleaned up
 * and that no threads are left running.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "landing.h"
#include "landing_readiness.h"
#include "landing_threads.h"
#include "../logging/logging.h"
#include "../threads/threads.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations for thread tracking
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Get the thread subsystem ID from registry
static int get_thread_subsystem_id(void) {
    return get_subsystem_id_by_name("Threads");
}

// Check if the thread subsystem is ready to land
LaunchReadiness check_threads_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Threads";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Threads");
    int msg_idx = 1;
    
    // Check if thread management is actually running
    if (!is_subsystem_running_by_name("Threads")) {
        readiness.ready = false;
        readiness.messages[msg_idx++] = strdup("  No-Go:   Thread management not running");
        readiness.messages[msg_idx++] = strdup("  Decide:  No-Go For Landing of Threads");
        readiness.messages[msg_idx] = NULL;
        return readiness;
    }
    
    // Check all service thread structures
    bool all_clear = true;
    
    // Only check non-logging threads since logging needs to stay active until the end
    if (webserver_threads.thread_count > 0) {
        readiness.messages[msg_idx++] = strdup("  No-Go:   Web threads still active");
        all_clear = false;
    }
    
    if (websocket_threads.thread_count > 0) {
        readiness.messages[msg_idx++] = strdup("  No-Go:   WebSocket threads still active");
        all_clear = false;
    }
    
    if (mdns_server_threads.thread_count > 0) {
        readiness.messages[msg_idx++] = strdup("  No-Go:   mDNS server threads still active");
        all_clear = false;
    }
    
    if (print_threads.thread_count > 0) {
        readiness.messages[msg_idx++] = strdup("  No-Go:   Print threads still active");
        all_clear = false;
    }
    
    // Final decision
    if (all_clear) {
        readiness.ready = true;
        readiness.messages[msg_idx++] = strdup("  Go:      All service threads ready for cleanup");
        readiness.messages[msg_idx++] = strdup("  Decide:  Go For Landing of Threads");
    } else {
        readiness.ready = false;
        readiness.messages[msg_idx++] = strdup("  Decide:  No-Go For Landing of Threads");
    }
    readiness.messages[msg_idx] = NULL;
    
    return readiness;
}

/**
 * Free thread-related resources
 * 
 * This function handles the cleanup of thread management resources during shutdown.
 * It ensures proper cleanup of all thread tracking structures and updates the registry.
 */
static void free_thread_resources(void) {
    // Begin resource cleanup section
    log_this("Threads", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Threads", "LANDING: THREADS - Resource Cleanup", LOG_LEVEL_STATE);
    
    // Step 1: Remove individual threads first
    if (webserver_threads.thread_count > 0) {
        log_this("Threads", "  Step 1a: Removing web threads (%d remaining)", 
                LOG_LEVEL_STATE, webserver_threads.thread_count);
        for (int i = 0; i < webserver_threads.thread_count; i++) {
            remove_service_thread(&webserver_threads, webserver_threads.thread_ids[i]);
        }
    }
    
    if (websocket_threads.thread_count > 0) {
        log_this("Threads", "  Step 1b: Removing websocket threads (%d remaining)", 
                LOG_LEVEL_STATE, websocket_threads.thread_count);
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            remove_service_thread(&websocket_threads, websocket_threads.thread_ids[i]);
        }
    }
    
    if (mdns_server_threads.thread_count > 0) {
        log_this("Threads", "  Step 1c: Removing mDNS server threads (%d remaining)", 
                LOG_LEVEL_STATE, mdns_server_threads.thread_count);
        for (int i = 0; i < mdns_server_threads.thread_count; i++) {
            remove_service_thread(&mdns_server_threads, mdns_server_threads.thread_ids[i]);
        }
    }
    
    if (print_threads.thread_count > 0) {
        log_this("Threads", "  Step 1d: Removing print threads (%d remaining)", 
                LOG_LEVEL_STATE, print_threads.thread_count);
        for (int i = 0; i < print_threads.thread_count; i++) {
            remove_service_thread(&print_threads, print_threads.thread_ids[i]);
        }
    }
    
    // Step 2: Free all thread resources
    log_this("Threads", "  Step 2: Freeing thread resources", LOG_LEVEL_STATE);
    free_threads_resources();
    
    // Note: Logging threads are handled last by the logging subsystem shutdown
    log_this("Threads", "  Step 3: Resource cleanup complete", LOG_LEVEL_STATE);
}

/**
 * Land the thread management subsystem
 * 
 * This function coordinates the complete shutdown sequence for the thread
 * management subsystem, ensuring proper cleanup and state transitions.
 * 
 * @return int 1 on success, 0 on failure
 */
int land_threads_subsystem(void) {
    // Begin LANDING: THREADS section
    log_this("Threads", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Threads", "LANDING: THREADS", LOG_LEVEL_STATE);
    
    // Step 1: Get current subsystem state
    int subsys_id = get_thread_subsystem_id();
    if (subsys_id < 0 || !is_subsystem_running(subsys_id)) {
        log_this("Threads", "Thread management not running, skipping shutdown", LOG_LEVEL_STATE);
        return 1;  // Success - nothing to do
    }
    
    // Step 2: Mark as stopping
    update_subsystem_state(subsys_id, SUBSYSTEM_STOPPING);
    log_this("Threads", "Beginning thread management shutdown sequence", LOG_LEVEL_STATE);
    
    // Step 3: Clean up resources
    free_thread_resources();
    
    // Step 4: Update registry and verify state
    update_subsystem_after_shutdown("Threads");
    
    // Step 5: Verify final state for restart capability
    SubsystemState final_state = get_subsystem_state(subsys_id);
    if (final_state == SUBSYSTEM_INACTIVE) {
        log_this("Threads", "LANDING: THREADS - Successfully landed and ready for future restart", LOG_LEVEL_STATE);
    } else {
        log_this("Threads", "LANDING: THREADS - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
    }
    
    return 1;  // Success
}