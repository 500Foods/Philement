/*
 * Main application entry point for the Hydrogen 3D printer server.
 * 
 * This file orchestrates multiple components including a web server, WebSocket server,
 * mDNS service discovery, logging system, and print queue management. It handles
 * graceful startup/shutdown of all services and provides a robust multi-threaded
 * architecture for 3D printer control and monitoring.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Project headers
#include "configuration.h"
#include "logging.h"
#include "queue.h"
#include "log_queue_manager.h"
#include "print_queue_manager.h"
#include "network.h"
#include "web_server.h"
#include "websocket_server.h"
#include "mdns_server.h"

// Handle application state in threadsafe manner
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t shutting_down = 0;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Track shutdown of each component seprately
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;
volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t mdns_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;

// Queue Threads
pthread_t log_thread;
pthread_t print_queue_thread;
pthread_t mdns_thread;
pthread_t web_thread;
pthread_t websocket_thread;

// Local data
AppConfig *app_config = NULL;

// mDNS and network info
mdns_t *mdns = NULL;
network_info_t *net_info = NULL;

// Ctrl+C handler
void inthandler(int signum) {
    (void)signum; 

    printf("\n");
    log_this("Shutdown", "Cleaning up and shutting down", 0, true, true, true);

    pthread_mutex_lock(&terminate_mutex);
    keep_running = 0;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
}

void graceful_shutdown() {
    log_this("Shutdown", "Initiating graceful shutdown", 0, true, true, true);

    // Signal all threads that shutdown is imminent
    keep_running = 0;
    pthread_cond_broadcast(&terminate_cond);

    // Shutdown mDNS
    log_this("Shutdown", "Initiating mDNS shutdown", 0, true, true, true);
    mdns_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(mdns_thread, NULL);
    if (mdns) {
        mdns_shutdown(mdns);
        mdns = NULL;
    }
    log_this("Shutdown", "mDNS shutdown complete", 0, true, true, true);

    // Shutdown Web Server
    log_this("Shutdown", "Initiating Web Server shutdown", 0, true, true, true);
    web_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(web_thread, NULL);
    shutdown_web_server();
    log_this("Shutdown", "Web Server shutdown complete", 0, true, true, true);

    // Shutdown WebSocket Server
    log_this("Shutdown", "Initiating WebSocket Server shutdown", 0, true, true, true);
    websocket_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    stop_websocket_server();
    cleanup_websocket_server();
    log_this("Shutdown", "WebSocket Server shutdown complete", 0, true, true, true);

    // Shutdown Print Queue
    log_this("Shutdown", "Initiating Print Queue shutdown", 0, true, true, true);
    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(print_queue_thread, NULL);
    shutdown_print_queue();
    log_this("Shutdown", "Print Queue shutdown complete", 0, true, true, true);

    // Shutdown Network Info
    log_this("Shutdown", "Freeing network info", 0, true, true, true);
    if (net_info) {
        free_network_info(net_info);
        net_info = NULL;
    }

    // Free other resources
    log_this("Shutdown", "Freeing other resources", 0, true, true, true);

    // Shutdown Logging
    log_this("Shutdown", "Initiating Logging shutdown", 0, true, true, true);
    log_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(log_thread, NULL);
    log_this("Shutdown", "Closing File Logging", 0, true, true, true);
    close_file_logging();
    log_this("Shutdown", "Logging shutdown complete", 0, true, true, true);

    // Shutdown queue system
    log_this("Shutdown", "Initiating queue system shutdown", 0, true, true, true);
    queue_system_destroy();
    log_this("Shutdown", "Initiating queue system shutdown", 0, true, true, true);

    // Synchronization variables
    log_this("Shutdown", "Releasing condition variable and mutex", 0, true, true, true);
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_destroy(&terminate_mutex);

    // Free app_config
    log_this("Shutdown", "Releasing configuration information", 0, true, true, true);
    if (app_config) {
        free(app_config->server_name);
        free(app_config->executable_path);
        free(app_config->log_file_path);

        // Free Web config
        free(app_config->web.web_root);
        free(app_config->web.upload_path);
        free(app_config->web.upload_dir);

        // Free WebSocket config
        free(app_config->websocket.key);
        free(app_config->websocket.protocol);

        // Free mDNS config
        free(app_config->mdns.device_id);
        free(app_config->mdns.friendly_name);
        free(app_config->mdns.model);
        free(app_config->mdns.manufacturer);
        free(app_config->mdns.version);
        for (size_t i = 0; i < app_config->mdns.num_services; i++) {
            free(app_config->mdns.services[i].name);
            free(app_config->mdns.services[i].type);
            for (size_t j = 0; j < app_config->mdns.services[i].num_txt_records; j++) {
                free(app_config->mdns.services[i].txt_records[j]);
            }
            free(app_config->mdns.services[i].txt_records);
        }
        free(app_config->mdns.services);

        free(app_config);
        app_config = NULL;
    }

    log_this("Shutdown", "Shutdown complete", 0, true, true, true);
    shutting_down = 1;
}
                    
int main(int argc, char *argv[]) {
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Set up interrupt handler
    signal(SIGINT, inthandler);

    // Initialize the queue system
    queue_system_init();
    
    // Create the SystemLog queue
    QueueAttributes system_log_attrs = {0}; // Initialize with default values for now
    Queue* system_log_queue = queue_create("SystemLog", &system_log_attrs);
    if (!system_log_queue) {
        fprintf(stderr, "Failed to create SystemLog queue\n");
        queue_system_destroy();
        return 1;
    }

    // Load configuration
    char* config_path = (argc > 1) ? argv[1] : "hydrogen.json";
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Initialization", "Failed to load configuration", 4, true, true, true);
        queue_system_destroy();
        return 1;
    }

    // Initialize file logging
    init_file_logging(app_config->log_file_path);

    log_this("Initialization", "Configuration loaded", 0, true, true, true);

    // Launch log queue manager
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        fprintf(stderr, "Failed to start log queue manager thread\n");
        queue_destroy(system_log_queue);
        queue_system_destroy();
        return 1;
    }

    // Output app information
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

    log_this("Initialization", "Log File: %s", 0, true, true, true, app_config->log_file_path ? app_config->log_file_path : "None"); 
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);

    // Initialize print queue
    init_print_queue();

    // Launch print queue manager
    if (pthread_create(&print_queue_thread, NULL, print_queue_manager, NULL) != 0) {
        log_this("Initialization", "Failed to start print queue manager thread", 4, true, true, true);
        // Cleanup code
        queue_destroy(system_log_queue);
        queue_system_destroy();
        return 1;
    }

    // Initialize web server
    if (!init_web_server(&app_config->web)) {
        log_this("Initialization", "Failed to initialize web server", 4, true, true, true);
        // Cleanup code
        queue_system_destroy();
        close_file_logging();
        free(app_config);
        return 1;
    }

    // Initialize web server thread
    if (pthread_create(&web_thread, NULL, run_web_server, NULL) != 0) {
        log_this("Initialization", "Failed to start web server thread", 4, true, true, true);
        // Cleanup code
        shutdown_web_server(); // This should clean up resources allocated in init_web_server
        queue_system_destroy();
        close_file_logging();
        free(app_config);
        return 1;
    }

    // Initialize WebSocket server
    if (init_websocket_server(app_config->websocket.port, app_config->websocket.protocol, app_config->websocket.key) != 0) {
        log_this("Initialization", "Failed to initialize WebSocket server", 4, true, true, true);
        graceful_shutdown();
        return 1;
    }

    // Start WebSocket server
    if (start_websocket_server() != 0) {
        log_this("Initialization", "Failed to start WebSocket server", 4, true, true, true);
        graceful_shutdown();
        return 1;
    }

    // Get the actual bound port and update mDNS configuration
    int actual_port = get_websocket_port();
    for (size_t i = 0; i < app_config->mdns.num_services; i++) {
        if (strcmp(app_config->mdns.services[i].type, "_websocket._tcp") == 0) {
            app_config->mdns.services[i].port = actual_port;
            log_this("Initialization", "Updated WebSocket mDNS service port to %d", 0, true, true, true, actual_port);
            break;
        }
    }

    // Initialize mDNS
    log_this("Initialization", "Starting mDNS initialization", 0, true, true, true);
    mdns = mdns_init(app_config->server_name, app_config->mdns.device_id, app_config->mdns.friendly_name,
                     app_config->mdns.model, app_config->mdns.manufacturer, app_config->mdns.version,
                     "1.0", // Hardware version
                     "http://localhost:5000", // Config URL
                     app_config->mdns.services, app_config->mdns.num_services);

    if (!mdns) {
        log_this("Initialization", "Failed to initialize mDNS", 3, true, true, true);
        graceful_shutdown();
        return 1;
    }

    // Start mDNS thread
    net_info = get_network_info();
    mdns_thread_arg_t mdns_arg = {
        .mdns = mdns,
        .port = app_config->web.port,
        .net_info = net_info,
        .running = &keep_running
    };

    if (pthread_create(&mdns_thread, NULL, mdns_announce_loop, &mdns_arg) != 0) {
        log_this("Initialization", "Failed to start mDNS thread", 3, true, true, true);
        mdns_shutdown(mdns);
        free_network_info(net_info);
        graceful_shutdown();
        return 1;
    }

    // Ready to go
    // Give threads a moment to launch first
    usleep(10000);
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);
    log_this("Initialization", "Application started", 0, true, true, true);
    log_this("Initialization", "Press Ctrl+C to exit", 0, true, false, true);
    log_this("Initialization", "%s", 0, true, true, true, LOG_LINE_BREAK);

    ////////////////////////////////////////////////////////////////////////////////

    while (keep_running) {
        pthread_mutex_lock(&terminate_mutex);
        pthread_cond_wait(&terminate_cond, &terminate_mutex);
        pthread_mutex_unlock(&terminate_mutex);
    }

    ////////////////////////////////////////////////////////////////////////////////

    graceful_shutdown();

    return 0;
}
