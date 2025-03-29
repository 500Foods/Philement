/*
 * Launch WebServer Subsystem
 * 
 * This module handles the initialization of the webserver subsystem.
 * It provides functions for checking readiness and launching the webserver.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing-webserver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "launch.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../utils/utils_threads.h"
#include "../webserver/web_server.h"

// Network constants
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x02
#endif

// External declarations
extern ServiceThreads web_threads;
extern pthread_t web_thread;
extern volatile sig_atomic_t web_server_shutdown;
extern AppConfig* app_config;
extern volatile sig_atomic_t server_starting;

// Check if the webserver subsystem is ready to launch
LaunchReadiness check_webserver_launch_readiness(void) {
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
    readiness.messages[0] = strdup("WebServer");
    readiness.messages[1] = strdup("  Go:      WebServer System Ready");
    readiness.messages[2] = strdup("  Decide:  Go For Launch of WebServer");
    readiness.messages[3] = NULL;
    
    return readiness;
}

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

    // Create and register the web server thread
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    // Register thread before creation
    extern ServiceThreads web_threads;
    
    if (pthread_create(&web_thread, &thread_attr, run_web_server, NULL) != 0) {
        log_this("Initialization", "Failed to start web server thread", LOG_LEVEL_ERROR);
        pthread_attr_destroy(&thread_attr);
        shutdown_web_server();
        return 0;
    }
    pthread_attr_destroy(&thread_attr);

    // Register thread and wait for initialization
    add_service_thread(&web_threads, web_thread);
    
    // Wait for server to fully initialize (up to 10 seconds)
    struct timespec wait_time = {0, 100000000}; // 100ms intervals
    int max_tries = 100; // 10 seconds total
    int tries = 0;
    bool server_ready = false;

    log_this("Initialization", "Waiting for web server to initialize...", LOG_LEVEL_STATE);
    
    while (tries < max_tries) {
        nanosleep(&wait_time, NULL);
        
        // Check if web daemon is running and bound to port
        extern struct MHD_Daemon *web_daemon;
        if (web_daemon != NULL) {
            const union MHD_DaemonInfo *info = MHD_get_daemon_info(web_daemon, MHD_DAEMON_INFO_BIND_PORT);
            if (info != NULL && info->port > 0) {
                // Get connection info
                const union MHD_DaemonInfo *conn_info = MHD_get_daemon_info(web_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
                unsigned int num_connections = conn_info ? conn_info->num_connections : 0;
                
                // Get thread info
                const union MHD_DaemonInfo *thread_info = MHD_get_daemon_info(web_daemon, MHD_DAEMON_INFO_FLAGS);
                bool using_threads = thread_info && (thread_info->flags & MHD_USE_THREAD_PER_CONNECTION);
                
                log_this("Initialization", "Web server status:", LOG_LEVEL_STATE);
                log_this("Initialization", "-> Bound to port: %u", LOG_LEVEL_STATE, info->port);
                log_this("Initialization", "-> Active connections: %u", LOG_LEVEL_STATE, num_connections);
                log_this("Initialization", "-> Thread mode: %s", LOG_LEVEL_STATE, 
                        using_threads ? "Thread per connection" : "Single thread");
                log_this("Initialization", "-> IPv6: %s", LOG_LEVEL_STATE, 
                        app_config->web.enable_ipv6 ? "enabled" : "disabled");
                log_this("Initialization", "-> Max connections: %d", LOG_LEVEL_STATE, 
                        app_config->web.max_connections);
                
                // Log network interfaces
                log_this("Initialization", "Network interfaces:", LOG_LEVEL_STATE);
                struct ifaddrs *ifaddr, *ifa;
                if (getifaddrs(&ifaddr) != -1) {
                    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                        if (ifa->ifa_addr == NULL)
                            continue;

                        int family = ifa->ifa_addr->sa_family;
                        if (family == AF_INET || (family == AF_INET6 && app_config->web.enable_ipv6)) {
                            char host[NI_MAXHOST];
                            int s = getnameinfo(ifa->ifa_addr,
                                             (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                                 sizeof(struct sockaddr_in6),
                                             host, NI_MAXHOST,
                                             NULL, 0, NI_NUMERICHOST);
                            if (s == 0) {
                                log_this("Initialization", "-> %s: %s (%s)", LOG_LEVEL_STATE,
                                        ifa->ifa_name, host,
                                        (family == AF_INET) ? "IPv4" : "IPv6");
                            }
                        }
                    }
                    freeifaddrs(ifaddr);
                }
                
                server_ready = true;
                break;
            }
        }
        tries++;
        
        if (tries % 10 == 0) { // Log every second
            log_this("Initialization", "Still waiting for web server... (%d seconds)", LOG_LEVEL_STATE, tries / 10);
        }
    }

    if (!server_ready) {
        log_this("Initialization", "Web server failed to start within timeout", LOG_LEVEL_ERROR);
        shutdown_web_server();
        return 0;
    }

    log_this("Initialization", "Web server thread created and registered", LOG_LEVEL_STATE);
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