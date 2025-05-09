/*
 * Launch WebServer Subsystem
 * 
 * This module handles the initialization of the webserver subsystem.
 * It provides functions for checking readiness and launching the webserver.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_webserver.c
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
#include "launch_network.h"
#include "launch_threads.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../webserver/web_server.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// Network constants
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x02
#endif

// External declarations
extern ServiceThreads webserver_threads;
extern pthread_t webserver_thread;
extern volatile sig_atomic_t web_server_shutdown;
extern AppConfig* app_config;
extern volatile sig_atomic_t server_starting;

// Registry ID and cached readiness state
int webserver_subsystem_id = -1;
static LaunchReadiness cached_readiness = {0};
static bool readiness_cached = false;

// Forward declarations
static void clear_cached_readiness(void);
static void register_webserver(void);

// Helper to clear cached readiness
static void clear_cached_readiness(void) {
    if (readiness_cached && cached_readiness.messages) {
        free_readiness_messages(&cached_readiness);
        readiness_cached = false;
    }
}

// Get cached readiness result
LaunchReadiness get_webserver_readiness(void) {
    if (readiness_cached) {
        return cached_readiness;
    }
    
    // Perform fresh check and cache result
    cached_readiness = check_webserver_launch_readiness();
    readiness_cached = true;
    return cached_readiness;
}

// Register the webserver subsystem with the registry
static void register_webserver(void) {
    // Always register during readiness check if not already registered
    if (webserver_subsystem_id < 0) {
        webserver_subsystem_id = register_subsystem("WebServer", NULL, NULL, NULL,
                                                  (int (*)(void))launch_webserver_subsystem,
                                                  (void (*)(void))shutdown_web_server);
    }
}

/*
 * Check if the webserver subsystem is ready to launch
 * 
 * This function performs readiness checks for the webserver subsystem by:
 * - Verifying system state and dependencies (Threads, Network)
 * - Checking protocol configuration (IPv4/IPv6)
 * - Validating interface availability
 * - Checking port and web root configuration
 * 
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Note: Prefer using get_webserver_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_webserver_launch_readiness(void) {
    const char** messages = malloc(15 * sizeof(char*));  // Space for messages + NULL
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "WebServer", .ready = false, .messages = NULL };
    }
    
    int msg_index = 0;
    bool ready = true;
    
    // First message is subsystem name
    messages[msg_index++] = strdup("WebServer");
    
    // Register with registry if not already registered
    register_webserver();
    
    LaunchReadiness readiness = { .subsystem = "WebServer", .ready = false, .messages = messages };
    
    // Register dependencies
    if (webserver_subsystem_id >= 0) {
        if (!add_dependency_from_launch(webserver_subsystem_id, "Threads")) {
            messages[msg_index++] = strdup("  No-Go:   Failed to register Threads dependency");
            messages[msg_index] = NULL;
            free_readiness_messages(&readiness);
            return readiness;
        }
        messages[msg_index++] = strdup("  Go:      Threads dependency registered");

        if (!add_dependency_from_launch(webserver_subsystem_id, "Network")) {
            messages[msg_index++] = strdup("  No-Go:   Failed to register Network dependency");
            messages[msg_index] = NULL;
            free_readiness_messages(&readiness);
            return readiness;
        }
        messages[msg_index++] = strdup("  Go:      Network dependency registered");
    }
    
    // 1. Check Threads subsystem launch readiness (using cached version)
    LaunchReadiness threads_readiness = get_threads_readiness();
    if (!threads_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   Threads subsystem not Go for Launch");
        messages[msg_index] = NULL;
        readiness.ready = false;
        free_readiness_messages(&readiness);
        return readiness;
    } else {
        messages[msg_index++] = strdup("  Go:      Threads subsystem Go for Launch");
    }
    
    // 2. Check Network subsystem launch readiness (using cached version)
    LaunchReadiness network_readiness = get_network_readiness();
    if (!network_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   Network subsystem not Go for Launch");
        ready = false;
    } else {
        messages[msg_index++] = strdup("  Go:      Network subsystem Go for Launch");
    }
    
    // 3. Check protocol configuration
    if (!app_config) {
        messages[msg_index++] = strdup("  No-Go:   Configuration not loaded");
        ready = false;
    } else {
        if (!app_config->webserver.enable_ipv4 && !app_config->webserver.enable_ipv6) {
            messages[msg_index++] = strdup("  No-Go:   No network protocols enabled");
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Protocols enabled: %s%s%s", 
                    app_config->webserver.enable_ipv4 ? "IPv4" : "",
                    (app_config->webserver.enable_ipv4 && app_config->webserver.enable_ipv6) ? " and " : "",
                    app_config->webserver.enable_ipv6 ? "IPv6" : "");
                messages[msg_index++] = msg;
            }
        }
        
        // 4 & 5. Check interface availability
        struct ifaddrs *ifaddr, *ifa;
        bool ipv4_available = false;
        bool ipv6_available = false;
        
        if (getifaddrs(&ifaddr) != -1) {
            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL) continue;
                
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    ipv4_available = true;
                } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                    ipv6_available = true;
                }
            }
            freeifaddrs(ifaddr);
        }
        
        if (app_config->webserver.enable_ipv4 && !ipv4_available) {
            messages[msg_index++] = strdup("  No-Go:   No IPv4 interfaces available");
            ready = false;
        } else if (app_config->webserver.enable_ipv4) {
            messages[msg_index++] = strdup("  Go:      IPv4 interfaces available");
        }
        
        if (app_config->webserver.enable_ipv6 && !ipv6_available) {
            messages[msg_index++] = strdup("  No-Go:   No IPv6 interfaces available");
            ready = false;
        } else if (app_config->webserver.enable_ipv6) {
            messages[msg_index++] = strdup("  Go:      IPv6 interfaces available");
        }
        
        // 6. Check port number
        int port = app_config->webserver.port;
        if (port != 80 && port != 443 && port <= 1023) {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  No-Go:   Invalid port number: %d", port);
                messages[msg_index++] = msg;
            }
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Valid port number: %d", port);
                messages[msg_index++] = msg;
            }
        }
        
        // 7. Check webserver root
        if (!app_config->webserver.web_root || !strchr(app_config->webserver.web_root, '/')) {
            messages[msg_index++] = strdup("  No-Go:   Invalid web root path");
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Valid web root: %s", app_config->webserver.web_root);
                messages[msg_index++] = msg;
            }
        }
    }
    
    // Final decision
    messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of WebServer Subsystem" :
        "  Decide:  No-Go For Launch of WebServer Subsystem");
    messages[msg_index] = NULL;
    
    return (LaunchReadiness){
        .subsystem = "WebServer",
        .ready = ready,
        .messages = messages
    };
}
// Launch web server system
// Requires: Logging system
//
// The web server handles HTTP/REST API requests for configuration and control.
// It is intentionally separate from the WebSocket server to:
// 1. Allow independent scaling
// 2. Enhance reliability through isolation
// 3. Support flexible deployment
// 4. Enable different security policies
int launch_webserver_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t web_server_shutdown;
    // Clear any cached readiness before checking final state
    clear_cached_readiness();
    log_this("WebServer", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("WebServer", "LAUNCH: WEBSERVER", LOG_LEVEL_STATE);

    // Step 1: Verify system state
    log_this("WebServer", "  Step 1: Verifying system state", LOG_LEVEL_STATE);
    
    if (server_stopping || web_server_shutdown) {
        log_this("WebServer", "Cannot initialize web server during shutdown", LOG_LEVEL_STATE);
        log_this("WebServer", "LAUNCH: WEBSERVER - Failed: System in shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    if (!server_starting) {
        log_this("WebServer", "Cannot initialize web server outside startup phase", LOG_LEVEL_STATE);
        log_this("WebServer", "LAUNCH: WEBSERVER - Failed: Not in startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config) {
        log_this("WebServer", "Configuration not loaded", LOG_LEVEL_STATE);
        log_this("WebServer", "LAUNCH: WEBSERVER - Failed: No configuration", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->webserver.enable_ipv4 && !app_config->webserver.enable_ipv6) {
        log_this("WebServer", "Web server disabled in configuration (no protocols enabled)", LOG_LEVEL_STATE);
        log_this("WebServer", "LAUNCH: WEBSERVER - Disabled by configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    log_this("WebServer", "    System state verified", LOG_LEVEL_STATE);

    // Step 2: Initialize web server
    log_this("WebServer", "  Step 2: Initializing web server", LOG_LEVEL_STATE);
    
    if (!init_web_server(&app_config->webserver)) {
        log_this("WebServer", "Failed to initialize web server", LOG_LEVEL_ERROR);
        log_this("WebServer", "LAUNCH: WEBSERVER - Failed to initialize", LOG_LEVEL_STATE);
        return 0;
    }

    // Step 3: Log configuration
    log_this("WebServer", "  Step 3: Verifying configuration", LOG_LEVEL_STATE);
    log_this("WebServer", "    IPv6 support: %s", LOG_LEVEL_STATE, 
             app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
    log_this("WebServer", "    Port: %d", LOG_LEVEL_STATE, 
             app_config->webserver.port);
    log_this("WebServer", "    WebRoot: %s", LOG_LEVEL_STATE, 
             app_config->webserver.web_root);
    log_this("WebServer", "    Upload Path: %s", LOG_LEVEL_STATE, 
             app_config->webserver.upload_path);
    log_this("WebServer", "    Upload Dir: %s", LOG_LEVEL_STATE, 
             app_config->webserver.upload_dir);
    log_this("WebServer", "    Thread Pool Size: %d", LOG_LEVEL_STATE, 
             app_config->webserver.thread_pool_size);
    log_this("WebServer", "    Max Connections: %d", LOG_LEVEL_STATE, 
             app_config->webserver.max_connections);
    log_this("WebServer", "    Max Connections Per IP: %d", LOG_LEVEL_STATE, 
             app_config->webserver.max_connections_per_ip);
    log_this("WebServer", "    Connection Timeout: %d seconds", LOG_LEVEL_STATE, 
             app_config->webserver.connection_timeout);

    // Step 4: Create and register web server thread
    log_this("WebServer", "  Step 4: Creating web server thread", LOG_LEVEL_STATE);
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    // Register thread before creation
    extern ServiceThreads webserver_threads;
    
    if (pthread_create(&webserver_thread, &thread_attr, run_web_server, NULL) != 0) {
        log_this("WebServer", "Failed to start web server thread", LOG_LEVEL_ERROR);
        pthread_attr_destroy(&thread_attr);
        shutdown_web_server();
        return 0;
    }
    pthread_attr_destroy(&thread_attr);

    // Register thread and wait for initialization
    add_service_thread(&webserver_threads, webserver_thread);
    
    // Wait for server to fully initialize (up to 10 seconds)
    struct timespec wait_time = {0, 100000000}; // 100ms intervals
    int max_tries = 100; // 10 seconds total
    int tries = 0;
    bool server_ready = false;

    log_this("WebServer", "  Step 5: Waiting for initialization", LOG_LEVEL_STATE);
    
    while (tries < max_tries) {
        nanosleep(&wait_time, NULL);
        
        // Check if web daemon is running and bound to port
        extern struct MHD_Daemon *webserver_daemon;
        if (webserver_daemon != NULL) {
            const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
            if (info != NULL && info->port > 0) {
                // Get connection info
                const union MHD_DaemonInfo *conn_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
                unsigned int num_connections = conn_info ? conn_info->num_connections : 0;
                
                // Get thread info
                const union MHD_DaemonInfo *thread_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_FLAGS);
                bool using_threads = thread_info && (thread_info->flags & MHD_USE_THREAD_PER_CONNECTION);
                
                log_this("WebServer", "    Server status:", LOG_LEVEL_STATE);
                log_this("WebServer", "    -> Bound to port: %u", LOG_LEVEL_STATE, info->port);
                log_this("WebServer", "    -> Active connections: %u", LOG_LEVEL_STATE, num_connections);
                log_this("WebServer", "    -> Thread mode: %s", LOG_LEVEL_STATE, 
                        using_threads ? "Thread per connection" : "Single thread");
                log_this("WebServer", "    -> IPv6: %s", LOG_LEVEL_STATE, 
                        app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
                log_this("WebServer", "    -> Max connections: %d", LOG_LEVEL_STATE, 
                        app_config->webserver.max_connections);
                
                // Log network interfaces
                log_this("WebServer", "    Network interfaces:", LOG_LEVEL_STATE);
                struct ifaddrs *ifaddr, *ifa;
                if (getifaddrs(&ifaddr) != -1) {
                    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                        if (ifa->ifa_addr == NULL)
                            continue;

                        int family = ifa->ifa_addr->sa_family;
                        if (family == AF_INET || (family == AF_INET6 && app_config->webserver.enable_ipv6)) {
                            char host[NI_MAXHOST];
                            int s = getnameinfo(ifa->ifa_addr,
                                             (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                                 sizeof(struct sockaddr_in6),
                                             host, NI_MAXHOST,
                                             NULL, 0, NI_NUMERICHOST);
                            if (s == 0) {
                                log_this("WebServer", "    -> %s: %s (%s)", LOG_LEVEL_STATE,
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
            log_this("WebServer", "Still waiting for web server... (%d seconds)", LOG_LEVEL_STATE, tries / 10);
        }
    }

    if (!server_ready) {
        log_this("WebServer", "Web server failed to start within timeout", LOG_LEVEL_ERROR);
        shutdown_web_server();
        return 0;
    }

    // Step 6: Update registry and verify state
    log_this("WebServer", "  Step 6: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("WebServer", true);
    
    SubsystemState final_state = get_subsystem_state(webserver_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("WebServer", "LAUNCH: WEBSERVER - Successfully launched and running", LOG_LEVEL_STATE);
    } else {
        log_this("WebServer", "LAUNCH: WEBSERVER - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }
    return 1;
}

// Check if web server is running
// This function checks if the web server is currently running
// and available to handle requests.
int is_web_server_running(void) {
    extern volatile sig_atomic_t web_server_shutdown;
    
    // Server is running if:
    // 1. At least one protocol is enabled in config
    // 2. Not in shutdown state
    return (app_config && (app_config->webserver.enable_ipv4 || app_config->webserver.enable_ipv6) && !web_server_shutdown);
}