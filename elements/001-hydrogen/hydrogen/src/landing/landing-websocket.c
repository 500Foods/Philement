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

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;
extern void cleanup_websocket_server(void);

// Check if the websocket subsystem is ready to land
LandingReadiness check_websocket_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "WebSocket";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("WebSocket");
    
    // Check if websocket is actually running
    if (!is_subsystem_running_by_name("WebSocket")) {
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

// Shutdown the websocket subsystem
void shutdown_websocket(void) {
    log_this("WebSocket", "Beginning WebSocket shutdown sequence", LOG_LEVEL_STATE);
    
    // Signal thread shutdown
    websocket_server_shutdown = 1;
    log_this("WebSocket", "Signaled WebSocket thread to stop", LOG_LEVEL_STATE);
    
    // Wait for thread to complete
    if (websocket_thread) {
        log_this("WebSocket", "Waiting for WebSocket thread to complete", LOG_LEVEL_STATE);
        pthread_join(websocket_thread, NULL);
        log_this("WebSocket", "WebSocket thread completed", LOG_LEVEL_STATE);
    }
    
    // Remove the websocket thread from tracking
    remove_service_thread(&websocket_threads, websocket_thread);
    
    // Reinitialize thread structure
    init_service_threads(&websocket_threads);
    
    // Call websocket server cleanup
    cleanup_websocket_server();
    
    log_this("WebSocket", "WebSocket shutdown complete", LOG_LEVEL_STATE);
}