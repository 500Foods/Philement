/*
 * Launch WebSocket Subsystem
 * 
 * This module handles the initialization of the WebSocket server subsystem.
 * It provides functions for checking readiness and launching the websocket server.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_websocket.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#include "launch.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../websocket/websocket_server.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_EXIT_WAIT_SECONDS 1
#define MAX_EXIT_WAIT_SECONDS 60
#define WEBSOCKET_MIN_MESSAGE_SIZE 1024        // 1KB
#define WEBSOCKET_MAX_MESSAGE_SIZE 0x40000000  // 1GB

// Helper function prototypes
static bool validate_protocol(const char* protocol);
static bool validate_key(const char* key);

// Validate protocol string
static bool validate_protocol(const char* protocol) {
    if (!protocol || !protocol[0]) {
        return false;
    }

    // Protocol must be a valid identifier:
    // - Start with letter
    // - Contain only letters, numbers, and hyphens
    if (!((protocol[0] >= 'a' && protocol[0] <= 'z') ||
          (protocol[0] >= 'A' && protocol[0] <= 'Z'))) {
        return false;
    }

    for (size_t i = 0; protocol[i]; i++) {
        char c = protocol[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '-')) {
            return false;
        }
    }

    return true;
}

// Validate key string
static bool validate_key(const char* key) {
    if (!key || !key[0]) {
        return false;
    }

    // Key must be:
    // - At least 8 characters long
    // - Contain only printable ASCII characters
    // - No spaces or control characters
    size_t len = strlen(key);
    if (len < 8) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        char c = key[i];
        if (c < 33 || c > 126) {  // Non-printable ASCII or space
            return false;
        }
    }

    return true;
}

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;
extern AppConfig* app_config;
extern volatile sig_atomic_t server_starting;

// Check if the websocket subsystem is ready to launch
LaunchReadiness check_websocket_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("WebSocket");
    
    // Register dependency on Network subsystem
    int websocket_id = get_subsystem_id_by_name("WebSockets");
    if (websocket_id >= 0) {
        if (!add_dependency_from_launch(websocket_id, "Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Failed to register Network dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network dependency registered");
        
        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Network subsystem not running");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network subsystem running");
    }
    
    // Check configuration
    if (!app_config || !app_config->websocket.enabled) {
        readiness.messages[msg_count++] = strdup("  No-Go:   WebSocket server disabled in configuration");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      WebSocket server enabled in configuration");

    // Validate port number
    if (app_config->websocket.port < MIN_PORT || app_config->websocket.port > MAX_PORT) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid port number (must be between 1024-65535)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Port number valid");

    // Validate protocol
    if (!validate_protocol(app_config->websocket.protocol)) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid protocol format");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Protocol format valid");

    // Validate key
    if (!validate_key(app_config->websocket.key)) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid key format (must be at least 8 printable characters)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Key format valid");

    // Validate message size limits
    if (app_config->websocket.max_message_size < WEBSOCKET_MIN_MESSAGE_SIZE ||
        app_config->websocket.max_message_size > WEBSOCKET_MAX_MESSAGE_SIZE) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid message size limits");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Message size limits valid");

    // Validate exit wait timeout
    if (app_config->websocket.exit_wait_seconds < MIN_EXIT_WAIT_SECONDS ||
        app_config->websocket.exit_wait_seconds > MAX_EXIT_WAIT_SECONDS) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid exit wait timeout (must be between 1-60 seconds)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Exit wait timeout valid");
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of WebSocket Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
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