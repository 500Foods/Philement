/*
 * Web Server Subsystem Startup Handler
 * 
 * This module handles the initialization of the web server subsystem.
 * It provides HTTP/REST API capabilities and is completely independent
 * from the WebSocket subsystem.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "../state.h"
#include "../../logging/logging.h"
#include "../../webserver/web_server.h"

// Initialize web server system
// Requires: Logging system
//
// The web server handles HTTP/REST API requests for configuration and control.
// It is intentionally separate from the WebSocket server to:
// 1. Allow independent scaling
// 2. Enhance reliability through isolation
// 3. Support flexible deployment
// 4. Enable different security policies
int init_webserver_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t web_server_shutdown;

    // Prevent initialization during any shutdown state
    if (server_stopping || web_server_shutdown) {
        log_this("Initialization", "Cannot initialize web server during shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this("Initialization", "Cannot initialize web server outside startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    // Double-check shutdown state before proceeding
    if (server_stopping || web_server_shutdown) {
        log_this("Initialization", "Shutdown initiated, aborting web server initialization", LOG_LEVEL_STATE);
        return 0;
    }

    // Initialize web server if enabled
    if (!app_config->web.enabled) {
        log_this("Initialization", "Web server disabled in configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    if (!init_web_server(&app_config->web)) {
        log_this("Initialization", "Failed to initialize web server", LOG_LEVEL_ERROR);
        return 0;
    }

    if (pthread_create(&web_thread, NULL, run_web_server, NULL) != 0) {
        log_this("Initialization", "Failed to start web server thread", LOG_LEVEL_ERROR);
        shutdown_web_server();
        return 0;
    }

    log_this("Initialization", "Web server initialized successfully", LOG_LEVEL_STATE);
    return 1;
}

// Check if web server is running
// This function checks if the web server is currently running
// and available to handle requests.
int is_web_server_running(void) {
    extern volatile sig_atomic_t web_server_shutdown;
    
    // Server is running if:
    // 1. It's enabled in config
    // 2. Not in shutdown state
    return (app_config && app_config->web.enabled && !web_server_shutdown);
}