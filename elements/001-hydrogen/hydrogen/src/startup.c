/*
 * Startup sequence handler for the Hydrogen 3D printer server.
 * 
 * This file manages the initialization of all server components including logging,
 * print queue, web server, WebSocket server, and mDNS service discovery. It ensures
 * proper startup order and handles initialization failures gracefully.
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

// Initialize web and websocket servers
static int init_web_systems(void) {
    // Initialize web server
    if (!init_web_server(&app_config->web)) {
        log_this("Initialization", "Failed to initialize web server", 4, true, true, true);
        return 0;
    }

    // Initialize web server thread
    if (pthread_create(&web_thread, NULL, run_web_server, NULL) != 0) {
        log_this("Initialization", "Failed to start web server thread", 4, true, true, true);
        shutdown_web_server();
        return 0;
    }

    // Initialize WebSocket server
    if (init_websocket_server(app_config->websocket.port, 
                            app_config->websocket.protocol, 
                            app_config->websocket.key) != 0) {
        log_this("Initialization", "Failed to initialize WebSocket server", 4, true, true, true);
        return 0;
    }

    // Start WebSocket server
    if (start_websocket_server() != 0) {
        log_this("Initialization", "Failed to start WebSocket server", 4, true, true, true);
        return 0;
    }

    return 1;
}

// Initialize mDNS system
static int init_mdns_system(void) {
    // Get the actual bound port and validate it
    int actual_port = get_websocket_port();
    if (actual_port <= 0 || actual_port > 65535) {
        log_this("Initialization", "Invalid WebSocket port: %d", 3, true, true, true, actual_port);
        return 0;
    }

    // Initialize mDNS with validated configuration
    log_this("Initialization", "Starting mDNS initialization", 0, true, true, true);
    char config_url[128];
    snprintf(config_url, sizeof(config_url), "http://localhost:%d", app_config->web.port);
    
    // Update service ports before mdns_init
    for (size_t i = 0; i < app_config->mdns.num_services; i++) {
        if (strcmp(app_config->mdns.services[i].type, "_websocket._tcp") == 0) {
            app_config->mdns.services[i].port = (uint16_t)actual_port;
            log_this("Initialization", "Setting WebSocket mDNS service port to %d", 0, true, true, true, actual_port);
        }
    }
    
    mdns = mdns_init(app_config->server_name, 
                     app_config->mdns.device_id, 
                     app_config->mdns.friendly_name,
                     app_config->mdns.model, 
                     app_config->mdns.manufacturer, 
                     app_config->mdns.version,
                     "1.0", // Hardware version
                     config_url,
                     app_config->mdns.services, 
                     app_config->mdns.num_services);

    if (!mdns) {
        log_this("Initialization", "Failed to initialize mDNS", 3, true, true, true);
        return 0;
    }

    // Start mDNS thread with heap-allocated arguments
    net_info = get_network_info();
    mdns_thread_arg_t *mdns_arg = malloc(sizeof(mdns_thread_arg_t));
    if (!mdns_arg) {
        log_this("Initialization", "Failed to allocate mDNS thread arguments", 3, true, true, true);
        mdns_shutdown(mdns);
        free_network_info(net_info);
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
        return 0;
    }

    return 1;
}

// Log application information
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

    // Initialize print queue system
    if (!init_print_system()) {
        queue_system_destroy();
        close_file_logging();
        return 0;
    }

    // Initialize web and websocket servers
    if (!init_web_systems()) {
        queue_system_destroy();
        close_file_logging();
        return 0;
    }

    // Initialize mDNS system
    if (!init_mdns_system()) {
        queue_system_destroy();
        close_file_logging();
        return 0;
    }

    // Give threads a moment to launch
    usleep(10000);
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);
    log_this("Initialization", "Application started", 0, true, true, true);
    log_this("Initialization", "Press Ctrl+C to exit", 0, true, false, true);
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);

    return 1;
}