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
 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sched.h>  // For sched_yield()
#include <src/webserver/web_server.h>

// External declarations
extern ServiceThreads webserver_threads;
extern pthread_t webserver_thread;
extern volatile sig_atomic_t web_server_shutdown;

// Registry ID and cached readiness state
int webserver_subsystem_id = -1;

// Forward declarations
static void register_webserver(void);

// Register the webserver subsystem with the registry
static void register_webserver(void) {
    // Always register during readiness check if not already registered
    if (webserver_subsystem_id < 0) {
        webserver_subsystem_id = register_subsystem(SR_WEBSERVER, NULL, NULL, NULL,
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
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_WEBSERVER));

    // Register with registry if not already registered
    register_webserver();

    // Register dependencies
    if (webserver_subsystem_id >= 0) {
        if (!add_dependency_from_launch(webserver_subsystem_id, SR_THREADS)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Threads dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_WEBSERVER, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Threads dependency registered"));

        if (!add_dependency_from_launch(webserver_subsystem_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_WEBSERVER, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));
    }
    
    // 1. Check Threads subsystem launch readiness (using cached version)
    // LaunchReadiness threads_readiness = get_threads_readiness();
    // if (!threads_readiness.ready) {
    //     messages[msg_index++] = strdup("  No-Go:   Threads subsystem not Go for Launch");
    //     messages[msg_index] = NULL;
    //     readiness.ready = false;
    //     free_readiness_messages(&readiness);
    //     return readiness;
    // } else {
    //     messages[msg_index++] = strdup("  Go:      Threads subsystem Go for Launch");
    // }
    
    // 2. Check Network subsystem launch readiness (using cached version)
    // LaunchReadiness network_readiness = get_network_readiness();
    // if (!network_readiness.ready) {
    //     messages[msg_index++] = strdup("  No-Go:   Network subsystem not Go for Launch");
    //     ready = false;
    // } else {
    //     messages[msg_index++] = strdup("  Go:      Network subsystem Go for Launch");
    // }

    // 3. Check protocol configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        ready = false;
    } else {
        if (!app_config->webserver.enable_ipv4 && !app_config->webserver.enable_ipv6) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No network protocols enabled"));
            ready = false;
        } else {
            char* protocol_msg = malloc(256);
            if (protocol_msg) {
                snprintf(protocol_msg, 256, "  Go:      Protocols enabled: %s%s%s",
                    app_config->webserver.enable_ipv4 ? "IPv4" : "",
                    (app_config->webserver.enable_ipv4 && app_config->webserver.enable_ipv6) ? " and " : "",
                    app_config->webserver.enable_ipv6 ? "IPv6" : "");
                add_launch_message(&messages, &count, &capacity, protocol_msg);
            }
        }

        // 4 & 5. Check interface availability
        struct ifaddrs *ifaddr;
        const struct ifaddrs *ifa;
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
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No IPv4 interfaces available"));
            ready = false;
        } else if (app_config->webserver.enable_ipv4) {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      IPv4 interfaces available"));
        }

        if (app_config->webserver.enable_ipv6 && !ipv6_available) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No IPv6 interfaces available"));
            ready = false;
        } else if (app_config->webserver.enable_ipv6) {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      IPv6 interfaces available"));
        }

        // 6. Check port number
        int port = app_config->webserver.port;
        if (port != 80 && port != 443 && port <= 1023) {
            char* port_msg = malloc(256);
            if (port_msg) {
                snprintf(port_msg, 256, "  No-Go:   Invalid port number: %d", port);
                add_launch_message(&messages, &count, &capacity, port_msg);
            }
            ready = false;
        } else {
            char* port_valid_msg = malloc(256);
            if (port_valid_msg) {
                snprintf(port_valid_msg, 256, "  Go:      Valid port number: %d", port);
                add_launch_message(&messages, &count, &capacity, port_valid_msg);
            }
        }

        // 7. Check webserver root
        if (!app_config->webserver.web_root || !strchr(app_config->webserver.web_root, '/')) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid web root path"));
            ready = false;
        } else {
            char* root_msg = malloc(256);
            if (root_msg) {
                snprintf(root_msg, 256, "  Go:      Valid web root: %s", app_config->webserver.web_root);
                add_launch_message(&messages, &count, &capacity, root_msg);
            }
        }
    }

    // Final decision
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of WebServer Subsystem" :
        "  Decide:  No-Go For Launch of WebServer Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_WEBSERVER,
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
    log_this(SR_WEBSERVER, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_WEBSERVER, "LAUNCH: " SR_WEBSERVER, LOG_LEVEL_STATE, 0);

    // Step 1: Verify system state
    log_this(SR_WEBSERVER, "  Step 1: Verifying system state", LOG_LEVEL_STATE, 0);
    
    if (server_stopping || web_server_shutdown) {
        log_this(SR_WEBSERVER, "Cannot initialize web server during shutdown", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Failed: System in shutdown", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!server_starting) {
        log_this(SR_WEBSERVER, "Cannot initialize web server outside startup phase", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Failed: Not in startup phase", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!app_config) {
        log_this(SR_WEBSERVER, "Configuration not loaded", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Failed: No configuration", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!app_config->webserver.enable_ipv4 && !app_config->webserver.enable_ipv6) {
        log_this(SR_WEBSERVER, "Web server disabled in configuration (no protocols enabled)", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Disabled by configuration", LOG_LEVEL_STATE, 0);
        return 1; // Not an error if disabled
    }

    log_this(SR_WEBSERVER, "    System state verified", LOG_LEVEL_STATE, 0);

    // Step 2: Initialize web server
    log_this(SR_WEBSERVER, "  Step 2: Initializing web server", LOG_LEVEL_STATE, 0);
    
    if (!init_web_server(&app_config->webserver)) {
        log_this(SR_WEBSERVER, "Failed to initialize web server", LOG_LEVEL_ERROR, 0);
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Failed to initialize", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Step 3: Log configuration
    log_this(SR_WEBSERVER, "  Step 3: Verifying configuration", LOG_LEVEL_STATE, 0);
    log_this(SR_WEBSERVER, "    IPv6 support: %s", LOG_LEVEL_STATE, 1, app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
    log_this(SR_WEBSERVER, "    Port: %d", LOG_LEVEL_STATE, 1, app_config->webserver.port);
    log_this(SR_WEBSERVER, "    WebRoot: %s", LOG_LEVEL_STATE, 1, app_config->webserver.web_root);
    log_this(SR_WEBSERVER, "    Upload Path: %s", LOG_LEVEL_STATE, 1, app_config->webserver.upload_path);
    log_this(SR_WEBSERVER, "    Upload Dir: %s", LOG_LEVEL_STATE, 1, app_config->webserver.upload_dir);
    log_this(SR_WEBSERVER, "    Thread Pool Size: %d", LOG_LEVEL_STATE, 1, app_config->webserver.thread_pool_size);
    log_this(SR_WEBSERVER, "    Max Connections: %d", LOG_LEVEL_STATE, 1, app_config->webserver.max_connections);
    log_this(SR_WEBSERVER, "    Max Connections Per IP: %d", LOG_LEVEL_STATE, 1, app_config->webserver.max_connections_per_ip);
    log_this(SR_WEBSERVER, "    Connection Timeout: %d seconds", LOG_LEVEL_STATE, 1, app_config->webserver.connection_timeout);

    // Step 4: Create and register web server thread
    log_this(SR_WEBSERVER, "  Step 4: Creating web server thread", LOG_LEVEL_STATE, 0);
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&webserver_thread, &thread_attr, run_web_server, NULL) != 0) {
        log_this(SR_WEBSERVER, "Failed to start web server thread", LOG_LEVEL_ERROR, 0);
        pthread_attr_destroy(&thread_attr);
        shutdown_web_server();
        return 0;
    }
    pthread_attr_destroy(&thread_attr);

    // Register thread and wait for initialization
    add_service_thread(&webserver_threads, webserver_thread);

    // Wait for server to fully initialize (up to 10 seconds)
    int max_tries = 10000; // 10 seconds total
    int tries = 0;
    bool server_ready = false;

    log_this(SR_WEBSERVER, "  Step 5: Waiting for initialization", LOG_LEVEL_STATE, 0);

    // Phase 1: Immediate check (no sleep)
    if (webserver_daemon != NULL) {
        const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
        if (info != NULL && info->port > 0) {
            server_ready = true;
        }
    }

    // Phase 2: CPU-friendly busy wait with yield (first 100 tries = ~10ms)
    while (!server_ready && tries < 100) {
        sched_yield(); // Yield CPU to other threads

        // Check if web daemon is running and bound to port
        if (webserver_daemon != NULL) {
            const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
            if (info != NULL && info->port > 0) {
                // Get connection info
                const union MHD_DaemonInfo *conn_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
                unsigned int num_connections = conn_info ? conn_info->num_connections : 0;
                
                // Get thread info
                const union MHD_DaemonInfo *thread_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_FLAGS);
                bool using_threads = thread_info && (thread_info->flags & MHD_USE_THREAD_PER_CONNECTION);
                
                log_this(SR_WEBSERVER, "    Server status:", LOG_LEVEL_STATE, 0);
                log_this(SR_WEBSERVER, "    -> Bound to port: %u", LOG_LEVEL_STATE, 1, info->port);
                log_this(SR_WEBSERVER, "    -> Active connections: %u", LOG_LEVEL_STATE, 1, num_connections);
                log_this(SR_WEBSERVER, "    -> Thread mode: %s", LOG_LEVEL_STATE, 1, using_threads ? "Thread per connection" : "Single thread");
                log_this(SR_WEBSERVER, "    -> IPv6: %s", LOG_LEVEL_STATE, 1, app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
                log_this(SR_WEBSERVER, "    -> Max connections: %d", LOG_LEVEL_STATE, 1, app_config->webserver.max_connections);
                
                // Log network interfaces
                log_this(SR_WEBSERVER, "    Network interfaces:", LOG_LEVEL_STATE, 0);
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
                                log_this(SR_WEBSERVER, "    -> %s: %s (%s)", LOG_LEVEL_STATE, 3,
                                        ifa->ifa_name, 
                                        host,
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
    }

    // Phase 3: Short microsecond sleeps (next 900 tries = ~9ms with 10us sleeps)
    struct timespec micro_wait = {0, 10000}; // 10 microseconds
    while (!server_ready && tries < 1000) {
        nanosleep(&micro_wait, NULL);

        // Check if web daemon is running and bound to port
        if (webserver_daemon != NULL) {
            const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
            if (info != NULL && info->port > 0) {
                // Get connection info
                const union MHD_DaemonInfo *conn_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
                unsigned int num_connections = conn_info ? conn_info->num_connections : 0;

                // Get thread info
                const union MHD_DaemonInfo *thread_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_FLAGS);
                bool using_threads = thread_info && (thread_info->flags & MHD_USE_THREAD_PER_CONNECTION);

                log_this(SR_WEBSERVER, "    Server status:", LOG_LEVEL_STATE, 0);
                log_this(SR_WEBSERVER, "    -> Bound to port: %u", LOG_LEVEL_STATE, 1, info->port);
                log_this(SR_WEBSERVER, "    -> Active connections: %u", LOG_LEVEL_STATE, 1, num_connections);
                log_this(SR_WEBSERVER, "    -> Thread mode: %s", LOG_LEVEL_STATE, 1, using_threads ? "Thread per connection" : "Single thread");
                log_this(SR_WEBSERVER, "    -> IPv6: %s", LOG_LEVEL_STATE, 1, app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
                log_this(SR_WEBSERVER, "    -> Max connections: %d", LOG_LEVEL_STATE, 1, app_config->webserver.max_connections);

                // Log network interfaces
                log_this(SR_WEBSERVER, "    Network interfaces:", LOG_LEVEL_STATE, 0);
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
                                log_this(SR_WEBSERVER, "    -> %s: %s (%s)", LOG_LEVEL_STATE, 3,
                                        ifa->ifa_name,
                                        host,
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
    }

    // Phase 4: Longer millisecond sleeps for remaining time
    struct timespec milli_wait = {0, 1000000}; // 1ms
    while (!server_ready && tries < max_tries) {
        nanosleep(&milli_wait, NULL);

        // Check if web daemon is running and bound to port
        if (webserver_daemon != NULL) {
            const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
            if (info != NULL && info->port > 0) {
                // Get connection info
                const union MHD_DaemonInfo *conn_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
                unsigned int num_connections = conn_info ? conn_info->num_connections : 0;

                // Get thread info
                const union MHD_DaemonInfo *thread_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_FLAGS);
                bool using_threads = thread_info && (thread_info->flags & MHD_USE_THREAD_PER_CONNECTION);

                log_this(SR_WEBSERVER, "    Server status:", LOG_LEVEL_STATE, 0);
                log_this(SR_WEBSERVER, "    -> Bound to port: %u", LOG_LEVEL_STATE, 1, info->port);
                log_this(SR_WEBSERVER, "    -> Active connections: %u", LOG_LEVEL_STATE, 1, num_connections);
                log_this(SR_WEBSERVER, "    -> Thread mode: %s", LOG_LEVEL_STATE, 1, using_threads ? "Thread per connection" : "Single thread");
                log_this(SR_WEBSERVER, "    -> IPv6: %s", LOG_LEVEL_STATE, 1, app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
                log_this(SR_WEBSERVER, "    -> Max connections: %d", LOG_LEVEL_STATE, 1, app_config->webserver.max_connections);

                // Log network interfaces
                log_this(SR_WEBSERVER, "    Network interfaces:", LOG_LEVEL_STATE, 0);
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
                                log_this(SR_WEBSERVER, "    -> %s: %s (%s)", LOG_LEVEL_STATE, 3,
                                        ifa->ifa_name,
                                        host,
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

        if (tries % 1000 == 0) { // Log every second
            log_this(SR_WEBSERVER, "Still waiting for web server... (%d seconds)", LOG_LEVEL_STATE, 1, tries / 1000);
        }
    }

    if (!server_ready) {
        log_this(SR_WEBSERVER, "Web server failed to start within timeout", LOG_LEVEL_ERROR, 0);
        shutdown_web_server();
        return 0;
    }

    // Step 6: Update registry and verify state
    log_this(SR_WEBSERVER, "  Step 6: Updating subsystem registry", LOG_LEVEL_STATE, 0);
    update_subsystem_on_startup(SR_WEBSERVER, true);
    
    SubsystemState final_state = get_subsystem_state(webserver_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Successfully launched and running", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_WEBSERVER, "LAUNCH: WEBSERVER - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
    return 1;
}
