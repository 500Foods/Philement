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
 * 
 * 4. Error Recovery
 *    Why So Careful?
 *    - Prevent partial initialization
 *    - Maintain hardware safety
 *    - Preserve calibration data
 *    - Enable manual intervention
 * 
 * 5. Resource Management
 *    Why This Matters?
 *    - Temperature sensor allocation
 *    - Motor controller initialization
 *    - End-stop signal handling
 *    - Emergency stop circuits
 * 
 * 6. Configuration Validation
 *    Why Validate Early?
 *    - Prevent unsafe settings
 *    - Verify hardware compatibility
 *    - Check temperature limits
 *    - Validate motion constraints
 * 
 * 7. Startup Monitoring
 *    Why Monitor Startup?
 *    - Detect hardware issues early
 *    - Verify sensor readings
 *    - Confirm communication links
 *    - Log initialization sequence
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
        log_this("Initialization", "Failed to create SystemLog queue", 3, true, false, true);
        return 0;
    }

    // Load configuration
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Initialization", "Failed to load configuration", 4, true, true, true);
        queue_destroy(system_log_queue);
        return 0;
    }

    // Update queue limits from loaded configuration
    update_queue_limits_from_config(app_config);

    // Initialize file logging
    init_file_logging(app_config->log_file_path);

    // Launch log queue manager
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        log_this("Initialization", "Failed to start log queue manager thread", 3, true, false, true);
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
//
// The web and websocket servers are intentionally decoupled to:
// 1. Allow independent scaling - Each server can handle its own load without affecting the other
// 2. Enhance reliability - Failure in one server doesn't compromise the other
// 3. Support flexible deployment - Systems can run with either or both servers
// 4. Enable different security policies - Each server can implement its own authentication
//
// The REST API (web server) handles stateless requests for configuration and control,
// while the WebSocket server provides low-latency status updates and real-time monitoring.
// This separation follows the principle of single responsibility and allows for
// future expansion of either interface without affecting the other.
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
//
// The mDNS system implements dynamic service advertisement based on active components.
// This design choice serves several purposes:
// 1. Zero-configuration networking - Clients can discover the server without manual setup
// 2. Accurate service representation - Only advertises services that are actually available
// 3. Runtime port adaptation - Handles cases where preferred ports are unavailable
// 4. Security through obscurity - Services are only advertised when explicitly enabled
//
// The service filtering logic prevents misleading network advertisements and
// ensures clients can rely on service discovery results. This is particularly
// important in environments with multiple Hydrogen instances or other competing services.
static int init_mdns_system(void) {
    // Initialize mDNS with validated configuration
    log_this("Initialization", "Starting mDNS initialization", LOG_LEVEL_INFO, true, true, true);

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
                        log_this("Initialization", "Setting WebSocket mDNS service port to %d", LOG_LEVEL_INFO, true, true, true, actual_port);
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
                     filtered_count,
                     app_config->mdns.enable_ipv6);

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
    mdns_arg->running = &server_running;

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
    log_this("Initialization", "%s", LOG_LEVEL_INFO, true, true, true, LOG_LINE_BREAK);
    log_this("Initialization", "Server Name: %s", LOG_LEVEL_INFO, true, true, true, app_config->server_name);
    log_this("Initialization", "Executable: %s", LOG_LEVEL_INFO, true, true, true, app_config->executable_path);
    log_this("Initialization", "Version: %s", LOG_LEVEL_INFO, true, true, true, VERSION);

    long file_size = get_file_size(app_config->executable_path);
    if (file_size >= 0) {
        log_this("Initialization", "Size: %ld", LOG_LEVEL_INFO, true, true, true, file_size);
    } else {
        log_this("Initialization", "Error: Unable to get file size", LOG_LEVEL_ERROR, true, false, true);
    }

    char* mod_time = get_file_modification_time(app_config->executable_path);
    if (mod_time) {
        log_this("Initialization", "Last Modified: %s", LOG_LEVEL_INFO, true, false, true, mod_time);
        free(mod_time);
    } else {
        log_this("Initialization", "Error: Unable to get modification time", LOG_LEVEL_ERROR, true, false, true);
    }

    log_this("Initialization", "Log File: %s", LOG_LEVEL_INFO, true, true, true, 
             app_config->log_file_path ? app_config->log_file_path : "None");
    log_this("Initialization", "%s", LOG_LEVEL_INFO, true, true, true, LOG_LINE_BREAK);
}

// Main startup function
//
// The startup sequence follows a carefully planned order to ensure system stability:
// 1. Queue system first - Required by all other components for thread-safe communication
// 2. Logging second - Essential for debugging startup issues and runtime monitoring
// 3. Optional systems last - Print queue, web servers, and mDNS in order of dependency
//
// This ordering is designed to:
// - Ensure core infrastructure (queues, logging) is available for all components
// - Allow partial system functionality even if optional components fail
// - Provide meaningful error reporting throughout the startup process
// - Enable clean shutdown if critical components fail
//
// Returns 1 on successful startup, 0 on critical failure
// Critical failures trigger cleanup of initialized components to prevent resource leaks
int startup_hydrogen(const char *config_path) {
    // Record the server start time first thing
    set_server_start_time();
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize thread tracking
    extern ServiceThreads logging_threads;
    extern ServiceThreads web_threads;
    extern ServiceThreads websocket_threads;
    extern ServiceThreads mdns_threads;
    extern ServiceThreads print_threads;
    
    init_service_threads(&logging_threads);
    init_service_threads(&web_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_threads);
    init_service_threads(&print_threads);
    
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
        log_this("Initialization", "Print Queue system initialized", LOG_LEVEL_INFO, true, true, true);
    } else {
        log_this("Initialization", "Print Queue system disabled", LOG_LEVEL_INFO, true, true, true);
    }

    // Initialize web and websocket servers if enabled
    if (app_config->web.enabled || app_config->websocket.enabled) {
        if (!init_web_systems()) {
            queue_system_destroy();
            close_file_logging();
            return 0;
        }
        if (app_config->web.enabled) {
            log_this("Initialization", "Web Server initialized", LOG_LEVEL_INFO, true, true, true);
        }
        if (app_config->websocket.enabled) {
            log_this("Initialization", "WebSocket Server initialized", LOG_LEVEL_INFO, true, true, true);
        }
    } else {
        log_this("Initialization", "Web systems disabled", LOG_LEVEL_INFO, true, true, true);
    }

    // Initialize mDNS system if enabled
    if (app_config->mdns.enabled) {
        if (!init_mdns_system()) {
            queue_system_destroy();
            close_file_logging();
            return 0;
        }
        log_this("Initialization", "mDNS system initialized", LOG_LEVEL_INFO, true, true, true);
    } else {
        log_this("Initialization", "mDNS system disabled", LOG_LEVEL_INFO, true, true, true);
    }

    // Give threads a moment to launch
    usleep(10000);
    log_this("Initialization", "%s", LOG_LEVEL_INFO, true, true, true, LOG_LINE_BREAK);
    log_this("Initialization", "Application started", LOG_LEVEL_INFO, true, true, true);
    log_this("Initialization", "Press Ctrl+C to exit", LOG_LEVEL_INFO, true, false, true);
    log_this("Initialization", "%s", LOG_LEVEL_INFO, true, true, true, LOG_LINE_BREAK);

    // All services have been started successfully, mark startup as complete
    server_starting = 0;  // First set the flag
    update_server_ready_time();  // Then try to record the time
    
    // Make sure we capture the ready time
    for (int i = 0; i < 5 && !is_server_ready_time_set(); i++) {
        usleep(1000);  // Wait 1ms between attempts
        update_server_ready_time();
    }

    return 1;
}