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
#include "startup_smtp_relay.h"
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

// External declarations
extern void close_file_logging(void);

// Public interface
int startup_hydrogen(const char* config_path);

// Private declarations
static void log_early_info(void);
static void log_config_info(void);

// Log early startup information (before any initialization)
static void log_early_info(void) {
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Startup", "Starting Hydrogen Server...", LOG_LEVEL_INFO);
    log_this("Startup", "Version: %s", LOG_LEVEL_INFO, VERSION);
    log_this("Startup", "Release: %s", LOG_LEVEL_INFO, RELEASE);
    log_this("Startup", "Build Type: %s", LOG_LEVEL_INFO, BUILD_TYPE);
    log_this("Startup", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_group_end();
}

// Log configuration-dependent information
static void log_config_info(void) {
    if (!app_config) return;
    
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Startup", "Server Name: %s", LOG_LEVEL_INFO, app_config->server_name);
    log_this("Startup", "Executable: %s", LOG_LEVEL_INFO, app_config->executable_path);

    long file_size = get_file_size(app_config->executable_path);
    if (file_size >= 0) {
        log_this("Startup", "Size: %ld", LOG_LEVEL_INFO, file_size);
    } else {
        log_this("Startup", "Error: Unable to get file size", LOG_LEVEL_ERROR);
    }

    log_this("Startup", "Log File: %s", LOG_LEVEL_INFO, 
            app_config->log_file_path ? app_config->log_file_path : "None");
    log_this("Startup", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_group_end();
}

/*
 * Main startup function implementation
 * 
 * The startup sequence follows a carefully planned order to ensure system stability:
 * 1. Early logging - Capture startup process from the very beginning
 * 2. Queue system - Required by all other components for thread-safe communication
 * 3. Logging system - Essential for debugging startup issues and runtime monitoring
 * 4. Optional systems - Print queue, web servers, and network services in order of dependency
 *
 * Returns 1 on successful startup, 0 on critical failure
 */
int startup_hydrogen(const char* config_path) {

    // First check if we're in shutdown mode - if so, prevent restart
    extern volatile sig_atomic_t server_stopping;
    if (server_stopping) {
        fprintf(stderr, "Preventing application restart during shutdown\n");
        return 0;
    }

    // Record the server start time
    set_server_start_time();

    // Start logging as early as possible
    log_early_info();
    
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

    // Check library dependencies
    int critical_dependencies = check_library_dependencies(app_config);
    if (critical_dependencies > 0) {
        log_this("Startup", "Found %d missing critical dependencies", LOG_LEVEL_WARN, critical_dependencies);
    } else {
        log_this("Startup", "All critical dependencies available", LOG_LEVEL_INFO);
    }

    // // Initialize web server
    // if (!init_webserver_subsystem()) {
    //     log_this("Initialization", "Web server failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize websocket server
    // if (!init_websocket_subsystem()) {
    //     log_this("Initialization", "WebSocket server failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize mDNS server
    // if (!init_mdns_server_subsystem()) {
    //     log_this("Initialization", "mDNS server failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize mDNS client
    // if (!init_mdns_client_subsystem()) {
    //     log_this("Initialization", "mDNS client failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize SMTP relay
    // if (!init_smtp_relay_subsystem()) {
    //     log_this("Initialization", "SMTP relay failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize Swagger
    // if (!init_swagger_subsystem()) {
    //     log_this("Initialization", "Swagger system failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize Terminal
    // if (!init_terminal_subsystem()) {
    //     log_this("Initialization", "Terminal system failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize print queue system
    // if (!init_print_subsystem()) {
    //     log_this("Initialization", "Print system failed to start", LOG_LEVEL_ERROR);
    //     queue_system_destroy();
    //     close_file_logging();
    //     return 0;
    // }


    // Give threads a moment to launch
    usleep(10000);

    // Log full configuration information now that app_config is available
    log_config_info();

     // All services have been started successfully
    server_starting = 0; 
    server_running = 1; 
    update_server_ready_time();  // Then try to record the time

    // Final Startup Message
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Startup", "Application started", LOG_LEVEL_INFO);
    log_this("Startup", "Press Ctrl+C to exit", LOG_LEVEL_INFO);
    log_this("Startup", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_group_end();
 
    return 1;
}