/*
 * Landing WebSocket Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the websocket subsystem.
 * It provides functions for:
 * - Checking websocket landing readiness
 * - Managing graceful thread shutdown
 * - Cleaning up websocket resources
 * 
 * Dependencies:
 * - Requires all active connections to be drained or timed out
 * - Requires thread synchronization for clean shutdown
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;
extern void stop_websocket_server(void);
extern void cleanup_websocket_server(void);

// Check if the websocket subsystem is ready to land
LaunchReadiness check_websocket_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_WEBSOCKET;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup(SR_WEBSOCKET);
    
    // Check if websocket is actually running
    if (!is_subsystem_running_by_name(SR_WEBSOCKET)) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebSocket not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of WebSocket");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check thread status
    bool threads_ready = true;
    if (websocket_thread && websocket_threads.thread_count > 0) {
        readiness.messages[1] = strdup("  Go:      WebSocket thread ready for shutdown");
    } else {
        threads_ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebSocket thread not accessible");
    }
    
    // Final decision
    if (threads_ready) {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All resources ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  Go For Landing of WebSocket");
    } else {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Resources not ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of WebSocket");
    }
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Land the websocket subsystem
int land_websocket_subsystem(void) {
    log_this(SR_WEBSOCKET, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_WEBSOCKET, "LANDING: LOGGING", LOG_LEVEL_STATE, 0);

    // Step 1: Stop the WebSocket server (handles thread shutdown and connection termination)
    log_this(SR_WEBSOCKET, "Stopping WebSocket server", LOG_LEVEL_STATE, 0);
    stop_websocket_server();
    
    // Step 2: Clean up server resources (handles context destruction)
    log_this(SR_WEBSOCKET, "Cleaning up WebSocket server resources", LOG_LEVEL_STATE, 0);
    cleanup_websocket_server();
    
    // Step 3: Remove the websocket thread from tracking and reinitialize
    log_this(SR_WEBSOCKET, "Cleaning up thread tracking", LOG_LEVEL_STATE, 0);
    remove_service_thread(&websocket_threads, websocket_thread);
    init_service_threads(&websocket_threads, SR_WEBSOCKET);
    
    log_this(SR_WEBSOCKET, "WebSocket shutdown complete", LOG_LEVEL_STATE, 0);
    
    return 1;  // Always successful
}
