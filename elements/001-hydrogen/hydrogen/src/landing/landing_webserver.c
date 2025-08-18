/*
 * Landing WebServer Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the webserver subsystem.
 * It provides functions for:
 * - Checking webserver landing readiness
 * - Managing graceful thread shutdown
 * - Cleaning up webserver resources
 * 
 * Dependencies:
 * - Requires all active connections to be drained or timed out
 * - Requires thread synchronization for clean shutdown
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "landing.h"

// External declarations
extern ServiceThreads webserver_threads;
extern pthread_t webserver_thread;
extern volatile sig_atomic_t web_server_shutdown;

// Check if the webserver subsystem is ready to land
LaunchReadiness check_webserver_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "WebServer";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("WebServer");
    
    // Check if webserver is actually running
    if (!is_subsystem_running_by_name("WebServer")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebServer not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of WebServer");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check thread status
    bool threads_ready = true;
    if (webserver_thread && webserver_threads.thread_count > 0) {
        readiness.messages[1] = strdup("  Go:      WebServer thread ready for shutdown");
    } else {
        threads_ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebServer thread not accessible");
    }
    
    // Final decision
    if (threads_ready) {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All resources ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  Go For Landing of WebServer");
    } else {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Resources not ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of WebServer");
    }
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Land the webserver subsystem
int land_webserver_subsystem(void) {
    log_this("WebServer", "Beginning WebServer shutdown sequence", LOG_LEVEL_STATE);
    bool success = true;
    
    // Signal thread shutdown
    web_server_shutdown = 1;
    log_this("WebServer", "Signaled WebServer thread to stop", LOG_LEVEL_STATE);
    
    // Wait for thread to complete
    if (webserver_thread) {
        log_this("WebServer", "Waiting for WebServer thread to complete", LOG_LEVEL_STATE);
        if (pthread_join(webserver_thread, NULL) != 0) {
            log_this("WebServer", "Error waiting for WebServer thread", LOG_LEVEL_ERROR);
            success = false;
        } else {
            log_this("WebServer", "WebServer thread completed", LOG_LEVEL_STATE);
        }
    }
    
    // Remove the web thread from tracking
    remove_service_thread(&webserver_threads, webserver_thread);
    
    // Reinitialize thread structure
    init_service_threads(&webserver_threads);
    
    log_this("WebServer", "WebServer shutdown complete", LOG_LEVEL_STATE);
    
    return success ? 1 : 0;  // Return 1 for success, 0 for failure
}
