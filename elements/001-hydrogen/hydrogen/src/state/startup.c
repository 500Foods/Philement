/*
 * Startup Sequence Handler for Hydrogen Server
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
#include "../config/config_filesystem.h"

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
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "HYDROGEN STARTUP", LOG_LEVEL_STATE);
    log_this("Startup", "Version: %s", LOG_LEVEL_STATE, VERSION);
    log_this("Startup", "Release: %s", LOG_LEVEL_STATE, RELEASE);
    log_this("Startup", "Build Type: %s", LOG_LEVEL_STATE, BUILD_TYPE);
    log_group_end();
}

// Log configuration-dependent information
static void log_config_info(void) {
    if (!app_config) return;
    
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "Server Name: %s", LOG_LEVEL_STATE, app_config->server.server_name);
    log_this("Startup", "Executable: %s", LOG_LEVEL_STATE, app_config->server.exec_file);

    long file_size = get_file_size(app_config->server.exec_file);
    if (file_size >= 0) {
        log_this("Startup", "Size: %ld", LOG_LEVEL_STATE, file_size);
    } else {
        log_this("Startup", "Error: Unable to get file size", LOG_LEVEL_ERROR);
    }

    log_this("Startup", "Log File: %s", LOG_LEVEL_STATE, 
            app_config->server.log_file ? app_config->server.log_file : "None");
    log_this("Startup", "Startup Delay: %d milliseconds", LOG_LEVEL_STATE, app_config->server.startup_delay);
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_group_end();
}

/*
 * Main startup function implementation
 * 
 * The startup sequence follows a carefully planned order to ensure system stability:
 * 1. Check core dependencies - Verify required libraries are available
 * 2. Load configuration - Determine which features are enabled
 * 3. Initialize queue system - Required for thread-safe communication
 * 4. Initialize logging - Essential for debugging and monitoring
 * 5. Initialize services - Each with its own thread management
 *
 * Returns 1 on successful startup, 0 on critical failure
 */
int startup_hydrogen(const char* config_path) {

    // First check if we're in shutdown mode - if so, prevent restart
    extern volatile sig_atomic_t server_stopping;
    if (server_stopping) {
        log_this("Startup", "Preventing application restart during shutdown", LOG_LEVEL_ERROR);
        return 0;
    }

    // Record the server start time
    set_server_start_time();

    // Basic early logging to stderr (no config needed)
    log_early_info();
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // 1. Check core library dependencies (before config)
    int critical_dependencies = check_library_dependencies(NULL);
    if (critical_dependencies > 0) {
        log_this("Startup", "Missing core library dependencies", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // 2. Load configuration
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Startup", "Failed to load configuration", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // 3. Initialize queue system
    queue_system_init();
    update_queue_limits_from_config(app_config);
    
    // 4. Initialize logging subsystem
    // Each service gets its own thread, initialized just before the service starts
    // This ensures proper resource allocation and startup sequencing
    extern ServiceThreads logging_threads;
    init_service_threads(&logging_threads);
    if (!init_logging_subsystem()) {
        queue_system_destroy();
        return 0;
    }

    // Now that logging is available, check feature-specific dependencies
    critical_dependencies = check_library_dependencies(app_config);
    if (critical_dependencies > 0) {
        log_this("Startup", "Missing libraries required by enabled features", LOG_LEVEL_ERROR);
        return 0;
    }

    // Log successful core initialization
    log_this("Startup", "Core systems initialized successfully", LOG_LEVEL_STATE);

    // Additional services (web, websocket, mdns, print) are initialized on demand
    // Each service will initialize its own thread when it starts
    // This prevents premature thread creation and ensures proper startup order

    // // Initialize web server (with its thread)
    // init_service_threads(&web_threads);
    // if (!init_webserver_subsystem()) {
    //     log_this("Initialization", "Web server failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize websocket server (with its thread)
    // init_service_threads(&websocket_threads);
    // if (!init_websocket_subsystem()) {
    //     log_this("Initialization", "WebSocket server failed to start", LOG_LEVEL_ERROR);
    //     return 0;
    // }

    // // Initialize mDNS server (with its thread)
    // init_service_threads(&mdns_server_threads);
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

    // // Initialize print queue system (with its thread)
    // init_service_threads(&print_threads);
    // if (!init_print_subsystem()) {
    //     log_this("Initialization", "Print system failed to start", LOG_LEVEL_ERROR);
    //     queue_system_destroy();
    //     close_file_logging();
    //     return 0;
    // }

    // Give threads a moment to launch based on configured startup delay
    log_this("Startup-Debug", "Using StartupDelay value: %d milliseconds", LOG_LEVEL_DEBUG, app_config->server.startup_delay);
    if (app_config->server.startup_delay > 0 && app_config->server.startup_delay < 10000) {
        // Only sleep if within reasonable range (0-10 seconds)
        usleep(app_config->server.startup_delay * 1000);
    } else {
        log_this("Startup", "Warning: StartupDelay value (%d) out of normal range, using default 5ms", LOG_LEVEL_WARN, app_config->server.startup_delay);
        usleep(5 * 1000); // Use default 5ms
    }

    // Log full configuration information now that app_config is available
    log_config_info();

    // All services have been started successfully
    server_starting = 0; 
    server_running = 1; 
    update_server_ready_time();  // Then try to record the time

    // Final Startup Message
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "Application started", LOG_LEVEL_STATE);
    log_this("Startup", "Press Ctrl+C to exit", LOG_LEVEL_STATE);
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_group_end();
 
    return 1;
}