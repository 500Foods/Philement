/*
 * Startup Sequence Handler for Hydrogen 3D Printer Control
 * 
 * Why Careful Startup Sequencing?
 * 1. Safety Requirements
 *    - Ensure printer is in known state
 *    - Verify safety systems before operation
 *    - Initialize emergency stop capability first
 *    - Prevent uncontrolled motion
 * 
 * 2. Component Dependencies
 *    - Queue system enables emergency commands
 *    - Logging captures hardware initialization
 *    - Print queue requires temperature monitoring
 *    - Network services need hardware status
 * 
 * 3. Initialization Order
 *    Why This Sequence?
 *    - Core safety systems first
 *    - Hardware control systems second
 *    - User interface systems last
 *    - Network services after safety checks
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Project headers
#include "../state/state.h"
#include "../logging/logging.h"
#include "../queue/queue.h"
#include "../utils/utils.h"
#include "../utils/utils_dependency.h"

// Subsystem startup headers
#include "startup_logging.h"
#include "startup_print.h"
#include "startup_webserver.h"
#include "startup_websocket.h"
#include "startup_mdns_server.h"
#include "startup_mdns_client.h"
#include "startup_smtp_server.h"
#include "startup_swagger.h"
#include "startup_terminal.h"

// Define RELEASE if not provided by compiler
#ifndef RELEASE
#define RELEASE "unknown"
#endif

// Define BUILD_TYPE if not provided by compiler
#ifndef BUILD_TYPE
#define BUILD_TYPE "unknown"
#endif

// Forward declarations
static void log_app_info(void);
extern void close_file_logging(void);

// Log application information
static void log_app_info(void) {
    log_this("Initialization", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Initialization", "Server Name: %s", LOG_LEVEL_INFO, app_config->server_name);
    log_this("Initialization", "Executable: %s", LOG_LEVEL_INFO, app_config->executable_path);
    log_this("Initialization", "Version: %s", LOG_LEVEL_INFO, VERSION);
    log_this("Initialization", "Release: %s", LOG_LEVEL_INFO, RELEASE);
    log_this("Initialization", "Build Type: %s", LOG_LEVEL_INFO, BUILD_TYPE);

    long file_size = get_file_size(app_config->executable_path);
    if (file_size >= 0) {
        log_this("Initialization", "Size: %ld", LOG_LEVEL_INFO, file_size);
    } else {
        log_this("Initialization", "Error: Unable to get file size", LOG_LEVEL_ERROR);
    }

    log_this("Initialization", "Log File: %s", LOG_LEVEL_INFO,
             app_config->log_file_path ? app_config->log_file_path : "None");
    log_this("Initialization", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
}

// Main startup function
//
// The startup sequence follows a carefully planned order to ensure system stability:
// 1. Queue system first - Required by all other components for thread-safe communication
// 2. Logging second - Essential for debugging startup issues and runtime monitoring
// 3. Optional systems last - Print queue, web servers, and network services in order of dependency
//
// Returns 1 on successful startup, 0 on critical failure
int startup_hydrogen(const char *config_path) {
    // First check if we're in shutdown mode - if so, prevent restart
    extern volatile sig_atomic_t server_stopping;
    if (server_stopping) {
        fprintf(stderr, "Preventing application restart during shutdown\n");
        return 0;
    }
    
    // Record the server start time first thing
    set_server_start_time();
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize thread tracking
    extern ServiceThreads logging_threads;
    extern ServiceThreads web_threads;
    extern ServiceThreads websocket_threads;
    extern ServiceThreads mdns_server_threads;
    extern ServiceThreads print_threads;
    
    init_service_threads(&logging_threads);
    init_service_threads(&web_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_server_threads);
    init_service_threads(&print_threads);
    
    // Initialize the queue system
    queue_system_init();
    
    // Initialize logging and configuration
    if (!init_logging_subsystem(config_path)) {
        queue_system_destroy();
        return 0;
    }

    // Check library dependencies first
    int critical_dependencies = check_library_dependencies(app_config);
    if (critical_dependencies > 0) {
        log_this("Initialization", "Found %d missing critical dependencies", LOG_LEVEL_WARN, critical_dependencies);
    } else {
        log_this("Initialization", "All critical dependencies available", LOG_LEVEL_INFO);
    }

    // Log application information
    log_app_info();

    // Initialize print queue system
    if (!init_print_subsystem()) {
        log_this("Initialization", "Print system failed to start", LOG_LEVEL_ERROR);
        queue_system_destroy();
        close_file_logging();
        return 0;
    }

    // Initialize web server
    if (!init_webserver_subsystem()) {
        log_this("Initialization", "Web server failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Initialize websocket server
    if (!init_websocket_subsystem()) {
        log_this("Initialization", "WebSocket server failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Initialize mDNS server
    if (!init_mdns_server_subsystem()) {
        log_this("Initialization", "mDNS server failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Initialize mDNS client
    if (!init_mdns_client_subsystem()) {
        log_this("Initialization", "mDNS client failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Initialize SMTP server
    if (!init_smtp_server_subsystem()) {
        log_this("Initialization", "SMTP server failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Initialize Swagger
    if (!init_swagger_subsystem()) {
        log_this("Initialization", "Swagger system failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Initialize Terminal
    if (!init_terminal_subsystem()) {
        log_this("Initialization", "Terminal system failed to start", LOG_LEVEL_ERROR);
        return 0;
    }

    // Give threads a moment to launch
    usleep(10000);

    // All services have been started successfully
    server_starting = 0;  // First set the flag
    update_server_ready_time();  // Then try to record the time

    // Log final startup message
    log_this("Initialization", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Initialization", "Application started", LOG_LEVEL_INFO);
    log_this("Initialization", "Press Ctrl+C to exit", LOG_LEVEL_INFO);
    log_this("Initialization", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    
    // Make sure we capture the ready time
    for (int i = 0; i < 5 && !is_server_ready_time_set(); i++) {
        usleep(1000);  // Wait 1ms between attempts
        update_server_ready_time();
    }

    return 1;
}