/*
 * Startup sequence handler for the Hydrogen Project server.
 * 
 * This file manages the initialization of all server components including core services
 * and optional features. The startup sequence handles both general-purpose components
 * (logging, web server, WebSocket server, mDNS) and specialized features (print queue). The startup
 * sequence follows a specific order to handle dependencies between components and
 * ensure proper initialization:
 * 
 * Initialization Order:
 * 1. Queue System - Core infrastructure for inter-thread communication
 * 2. Logging System - Enables system-wide logging and error reporting
 * 3. Configuration - Loads and validates server configuration
 * 4. Print Queue (optional) - Manages 3D print job scheduling
 * 5. Web Server (optional) - Handles HTTP requests for the REST API
 * 6. WebSocket Server (optional) - Provides real-time status updates
 * 7. mDNS System (optional) - Enables network service discovery
 * 
 * Error Handling:
 * - Each component's initialization is isolated to prevent cascading failures
 * - Failed initialization of optional components may not stop the server
 * - Critical component failures (logging, queue system) trigger immediate shutdown
 * - All failures are logged when possible for diagnostics
 * 
 * Configuration:
 * - Components can be selectively enabled/disabled via config
 * - Each component has its own configuration section
 * - Runtime validation ensures all required settings are present
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Project headers
#include "state.h"
#include "logging.h"
#include "queue.h"
#include "log_queue_manager.h"    // For init_file_logging()
#include "print_queue_manager.h"
#include "web_server.h"
#include "websocket_server.h"
#include "utils.h"

// Forward declarations
static void log_app_info(void);

// Initialize logging system and create log queue
// This is a critical system component - failure here will prevent startup
// The log queue provides thread-safe logging for all other components
static int init_logging(const char *config_path) {
    // Create the SystemLog queue
    QueueAttributes system_log_attrs = {0};
    Queue* system_log_queue = queue_create("SystemLog", &system_log_attrs);
    if (!system_log_queue) {
        fprintf(stderr, "Failed to create SystemLog queue\n");
        return 0;
    }

    // Load configuration
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Initialization", "Failed to load configuration", 4, true, true, true);
        queue_destroy(system_log_queue);
        return 0;
    }

    // Initialize file logging
    init_file_logging(app_config->log_file_path);

    // Launch log queue manager
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        fprintf(stderr, "Failed to start log queue manager thread\n");
        queue_destroy(system_log_queue);
        return 0;
    }

    return 1;
}

// Initialize print queue system
// Requires: Logging system, Queue system
// Optional component that manages the 3D printer job queue
// Launches a dedicated thread for processing print jobs
static int init_print_system(void) {
    if (!init_print_queue()) {
        return 0;
    }

    if (pthread_create(&print_queue_thread, NULL, print_queue_manager, NULL) != 0) {
        log_this("Initialization", "Failed to start print queue manager thread", 4, true, true, true);
        return 0;
    }

    return 1;
}

// Initialize web and websocket servers independently
// Requires: Logging system
// Optional components that can be enabled/disabled separately
// Web server provides REST API access
// WebSocket server enables real-time status updates
// Each server runs in its own thread when enabled
static int init_web_systems(void) {
    int web_success = 1;
    int websocket_success = 1;

    // Initialize web server if enabled
    if (app_config->web.enabled) {
        if (!init_web_server(&app_config->web)) {
            log_this("Initialization", "Failed to initialize web server", 4, true, true, true);
            web_success = 0;
        } else if (pthread_create(&web_thread, NULL, run_web_server, NULL) != 0) {
            log_this("Initialization", "Failed to start web server thread", 4, true, true, true);
            shutdown_web_server();
            web_success = 0;
        }
    }

    // Initialize WebSocket server if enabled (completely independent of web server)
    if (app_config->websocket.enabled) {
        if (init_websocket_server(app_config->websocket.port, 
                                app_config->websocket.protocol, 
                                app_config->websocket.key) != 0) {
            log_this("Initialization", "Failed to initialize WebSocket server", 4, true, true, true);
            websocket_success = 0;
        } else if (start_websocket_server() != 0) {
            log_this("Initialization", "Failed to start WebSocket server", 4, true, true, true);
            websocket_success = 0;
        }
    }

    // Return success if at least one enabled service started successfully
    if (app_config->web.enabled && !web_success) {
        log_this("Initialization", "Web server failed to start", 3, true, true, true);
    }
    if (app_config->websocket.enabled && !websocket_success) {
        log_this("Initialization", "WebSocket server failed to start", 3, true, true, true);
    }

    // Only return failure if all enabled services failed
    return (!app_config->web.enabled || web_success) || 
           (!app_config->websocket.enabled || websocket_success);
}

// Initialize mDNS system
// Requires: Network info, Logging system
// Optional component for network service discovery
// Advertises enabled services (web/websocket) via mDNS
// Filters advertised services based on which servers are running
static int init_mdns_system(void) {
    // Initialize mDNS with validated configuration
    log_this("Initialization", "Starting mDNS initialization", 0, true, true, true);

    // Create a filtered list of services based on what's enabled
    mdns_service_t *filtered_services = NULL;
    size_t filtered_count = 0;

    if (app_config->mdns.services && app_config->mdns.num_services > 0) {
        filtered_services = calloc(app_config->mdns.num_services, sizeof(mdns_service_t));
        if (!filtered_services) {
            log_this("Initialization", "Failed to allocate memory for filtered services", 3, true, true, true);
            return 0;
        }

        for (size_t i = 0; i < app_config->mdns.num_services; i++) {
            // Only include web-related services if web server is enabled
            if (strstr(app_config->mdns.services[i].type, "_http._tcp") != NULL) {
                if (app_config->web.enabled) {
                    memcpy(&filtered_services[filtered_count], &app_config->mdns.services[i], sizeof(mdns_service_t));
                    filtered_count++;
                }
            }
            // Only include websocket services if websocket server is enabled
            else if (strstr(app_config->mdns.services[i].type, "_websocket._tcp") != NULL) {
                if (app_config->websocket.enabled) {
                    int actual_port = get_websocket_port();
                    if (actual_port > 0 && actual_port <= 65535) {
                        memcpy(&filtered_services[filtered_count], &app_config->mdns.services[i], sizeof(mdns_service_t));
                        filtered_services[filtered_count].port = (uint16_t)actual_port;
                        log_this("Initialization", "Setting WebSocket mDNS service port to %d", 0, true, true, true, actual_port);
                        filtered_count++;
                    } else {
                        log_this("Initialization", "Invalid WebSocket port: %d, skipping mDNS service", 3, true, true, true, actual_port);
                    }
                }
            }
            // Include any other services by default
            else {
                memcpy(&filtered_services[filtered_count], &app_config->mdns.services[i], sizeof(mdns_service_t));
                filtered_count++;
            }
        }
    }

    // Only create config URL if web server is enabled
    char config_url[128] = {0};
    if (app_config->web.enabled) {
        snprintf(config_url, sizeof(config_url), "http://localhost:%d", app_config->web.port);
    }
    
    mdns = mdns_init(app_config->server_name, 
                     app_config->mdns.device_id, 
                     app_config->mdns.friendly_name,
                     app_config->mdns.model, 
                     app_config->mdns.manufacturer, 
                     app_config->mdns.version,
                     "1.0", // Hardware version
                     config_url,
                     filtered_services,
                     filtered_count);

    if (!mdns) {
        log_this("Initialization", "Failed to initialize mDNS", 3, true, true, true);
        free(filtered_services);
        return 0;
    }

    // Start mDNS thread with heap-allocated arguments
    net_info = get_network_info();
    mdns_thread_arg_t *mdns_arg = malloc(sizeof(mdns_thread_arg_t));
    if (!mdns_arg) {
        log_this("Initialization", "Failed to allocate mDNS thread arguments", 3, true, true, true);
        mdns_shutdown(mdns);
        free_network_info(net_info);
        free(filtered_services);
        return 0;
    }

    mdns_arg->mdns = mdns;
    mdns_arg->port = 0;  // Not used anymore, each service has its own port
    mdns_arg->net_info = net_info;
    mdns_arg->running = &keep_running;

    if (pthread_create(&mdns_thread, NULL, mdns_announce_loop, mdns_arg) != 0) {
        log_this("Initialization", "Failed to start mDNS thread", 3, true, true, true);
        free(mdns_arg);
        mdns_shutdown(mdns);
        free_network_info(net_info);
        free(filtered_services);
        return 0;
    }

    free(filtered_services);  // Safe to free after mdns_init has copied the data

    return 1;
}

// Log application information
// Records key details about the server instance including:
// - Server name and version
// - Executable details (path, size, modification time)
// - Active configuration settings
static void log_app_info(void) {
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);
    log_this("Initialization", "Server Name: %s", 0, true, true, true, app_config->server_name);
    log_this("Initialization", "Executable: %s", 0, true, true, true, app_config->executable_path);
    log_this("Initialization", "Version: %s", 0, true, true, true, VERSION);

    long file_size = get_file_size(app_config->executable_path);
    if (file_size >= 0) {
        log_this("Initialization", "Size: %ld", 0, true, true, true, file_size);
    } else {
        log_this("Initialization", "Error: Unable to get file size", 0, true, false, true);
    }

    char* mod_time = get_file_modification_time(app_config->executable_path);
    if (mod_time) {
        log_this("Initialization", "Last Modified: %s", 0, true, false, true, mod_time);
        free(mod_time);
    } else {
        log_this("Initialization", "Error: Unable to get modification time", 0, true, false, true);
    }

    log_this("Initialization", "Log File: %s", 0, true, true, true, 
             app_config->log_file_path ? app_config->log_file_path : "None");
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);
}

// Main startup function
// Orchestrates the entire startup sequence
// Returns 1 on successful startup, 0 on critical failure
// Critical failures trigger cleanup of initialized components
int startup_hydrogen(const char *config_path) {
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize the queue system
    queue_system_init();
    
    // Initialize logging and configuration
    if (!init_logging(config_path)) {
        queue_system_destroy();
        return 0;
    }

    // Log application information after logging is initialized
    log_app_info();

    // Initialize print queue system if enabled
    if (app_config->print_queue.enabled) {
        if (!init_print_system()) {
            queue_system_destroy();
            close_file_logging();
            return 0;
        }
        log_this("Initialization", "Print Queue system initialized", 0, true, true, true);
    } else {
        log_this("Initialization", "Print Queue system disabled", 0, true, true, true);
    }

    // Initialize web and websocket servers if enabled
    if (app_config->web.enabled || app_config->websocket.enabled) {
        if (!init_web_systems()) {
            queue_system_destroy();
            close_file_logging();
            return 0;
        }
        if (app_config->web.enabled) {
            log_this("Initialization", "Web Server initialized", 0, true, true, true);
        }
        if (app_config->websocket.enabled) {
            log_this("Initialization", "WebSocket Server initialized", 0, true, true, true);
        }
    } else {
        log_this("Initialization", "Web systems disabled", 0, true, true, true);
    }

    // Initialize mDNS system if enabled
    if (app_config->mdns.enabled) {
        if (!init_mdns_system()) {
            queue_system_destroy();
            close_file_logging();
            return 0;
        }
        log_this("Initialization", "mDNS system initialized", 0, true, true, true);
    } else {
        log_this("Initialization", "mDNS system disabled", 0, true, true, true);
    }

    // Give threads a moment to launch
    usleep(10000);
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);
    log_this("Initialization", "Application started", 0, true, true, true);
    log_this("Initialization", "Press Ctrl+C to exit", 0, true, false, true);
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);

    return 1;
}