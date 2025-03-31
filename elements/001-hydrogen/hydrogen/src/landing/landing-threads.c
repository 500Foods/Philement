/*
 * Landing Thread Subsystem Implementation
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
#include "landing-threads.h"
#include "../logging/logging.h"
#include "../threads/threads.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// External declarations for thread tracking
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Check if the thread subsystem is ready to land
LandingReadiness check_threads_landing_readiness(void) {
    LandingReadiness readiness = {0};
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
    if (web_threads.thread_count > 0) {
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

// Shutdown the thread management subsystem
void shutdown_threads(void) {
    log_this("Threads", "Beginning thread management shutdown sequence", LOG_LEVEL_STATE);
    
    // Clean up web server threads
    if (web_threads.thread_count > 0) {
        log_this("Threads", "Warning: %d web threads still active during shutdown", 
                LOG_LEVEL_STATE, web_threads.thread_count);
        init_service_threads(&web_threads);
    }
    
    // Clean up websocket threads
    if (websocket_threads.thread_count > 0) {
        log_this("Threads", "Warning: %d websocket threads still active during shutdown", 
                LOG_LEVEL_STATE, websocket_threads.thread_count);
        init_service_threads(&websocket_threads);
    }
    
    // Clean up mDNS server threads
    if (mdns_server_threads.thread_count > 0) {
        log_this("Threads", "Warning: %d mDNS server threads still active during shutdown", 
                LOG_LEVEL_STATE, mdns_server_threads.thread_count);
        init_service_threads(&mdns_server_threads);
    }
    
    // Clean up print threads
    if (print_threads.thread_count > 0) {
        log_this("Threads", "Warning: %d print threads still active during shutdown", 
                LOG_LEVEL_STATE, print_threads.thread_count);
        init_service_threads(&print_threads);
    }
    
    // Note: Logging threads are handled last by the logging subsystem shutdown
    
    log_this("Threads", "Thread management shutdown complete", LOG_LEVEL_STATE);
    
    // Update registry that we're done
    update_subsystem_after_shutdown("Threads");
}