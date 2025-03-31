/*
 * Launch WebSocket Subsystem
 * 
 * This module handles the initialization of the WebSocket server subsystem.
 * It provides functions for checking readiness and launching the websocket server.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing-websocket.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "launch.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../websocket/websocket_server.h"

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;
extern AppConfig* app_config;
extern volatile sig_atomic_t server_starting;

// Check if the websocket subsystem is ready to launch
LaunchReadiness check_websocket_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // For initial implementation, mark as ready
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("WebSocket");
    readiness.messages[1] = strdup("  Go:      WebSocket System Ready");
    readiness.messages[2] = strdup("  Decide:  Go For Launch of WebSocket");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Launch WebSocket server system
// Requires: Logging system
//
// The WebSocket server provides real-time status updates and monitoring.
// It is intentionally separate from the web server to:
// 1. Allow independent scaling
// 2. Enhance reliability through isolation
// 3. Support flexible deployment
// 4. Enable different security policies
int launch_websocket_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t websocket_server_shutdown;

    // Prevent initialization during any shutdown state
    if (server_stopping || websocket_server_shutdown) {
        log_this("Initialization", "Cannot initialize WebSocket server during shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this("Initialization", "Cannot initialize WebSocket server outside startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    // Double-check shutdown state before proceeding
    if (server_stopping || websocket_server_shutdown) {
        log_this("Initialization", "Shutdown initiated, aborting WebSocket server initialization", LOG_LEVEL_STATE);
        return 0;
    }

    // Initialize WebSocket server if enabled
    if (!app_config->websocket.enabled) {
        log_this("Initialization", "WebSocket server disabled in configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    if (init_websocket_server(app_config->websocket.port,
                            app_config->websocket.protocol,
                            app_config->websocket.key) != 0) {
        log_this("Initialization", "Failed to initialize WebSocket server", LOG_LEVEL_ERROR);
        return 0;
    }

    if (start_websocket_server() != 0) {
        log_this("Initialization", "Failed to start WebSocket server", LOG_LEVEL_ERROR);
        return 0;
    }

    log_this("Initialization", "WebSocket server initialized successfully", LOG_LEVEL_STATE);
    return 1;
}

// Check if WebSocket server is running
// This function checks if the WebSocket server is currently running
// and available to handle real-time connections.
int is_websocket_server_running(void) {
    extern volatile sig_atomic_t websocket_server_shutdown;
    
    // Server is running if:
    // 1. It's enabled in config
    // 2. Not in shutdown state
    return (app_config && app_config->websocket.enabled && !websocket_server_shutdown);
}