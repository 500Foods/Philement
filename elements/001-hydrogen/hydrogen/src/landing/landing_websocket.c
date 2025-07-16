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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../websocket/websocket_server.h"
#include "../utils/utils_dependency.h"
#include "../state/state_types.h"

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;
extern void stop_websocket_server(void);
extern void cleanup_websocket_server(void);

// Check if the websocket subsystem is ready to land
LaunchReadiness check_websocket_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "WebSockets";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("WebSockets");
    
    // Check if websocket is actually running
    if (!is_subsystem_running_by_name("WebSockets")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebSockets not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of WebSockets");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check thread status
    bool threads_ready = true;
    if (websocket_thread && websocket_threads.thread_count > 0) {
        readiness.messages[1] = strdup("  Go:      WebSockets thread ready for shutdown");
    } else {
        threads_ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebSockets thread not accessible");
    }
    
    // Final decision
    if (threads_ready) {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All resources ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  Go For Landing of WebSockets");
    } else {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Resources not ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of WebSockets");
    }
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Land the websocket subsystem
int land_websocket_subsystem(void) {
    log_this("WebSockets", "Beginning WebSockets shutdown sequence", LOG_LEVEL_STATE);
    bool success = true;
    
    // Step 1: Stop the WebSocket server (handles thread shutdown and connection termination)
    log_this("WebSockets", "Stopping WebSockets server", LOG_LEVEL_STATE);
    stop_websocket_server();
    
    // Step 2: Clean up server resources (handles context destruction)
    log_this("WebSockets", "Cleaning up WebSockets server resources", LOG_LEVEL_STATE);
    cleanup_websocket_server();
    
    // Step 3: Remove the websocket thread from tracking and reinitialize
    log_this("WebSockets", "Cleaning up thread tracking", LOG_LEVEL_STATE);
    remove_service_thread(&websocket_threads, websocket_thread);
    init_service_threads(&websocket_threads);
    
    log_this("WebSockets", "WebSockets shutdown complete", LOG_LEVEL_STATE);
    
    return success ? 1 : 0;  // Return 1 for success, 0 for failure
}