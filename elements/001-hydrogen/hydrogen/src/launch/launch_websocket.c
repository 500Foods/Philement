/*
 * Launch WebSocket Subsystem
 * 
 * This module handles the initialization of the websocket subsystem.
 * It provides functions for checking readiness and launching the websocket.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_websocket.c
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <src/websocket/websocket_server.h>

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;

// Registry ID and cached readiness state
int websocket_subsystem_id = -1;

// Register the websocket subsystem with the registry
void register_websocket(void) {
    // Always register during readiness check if not already registered
}

// Validate protocol string
bool validate_protocol(const char* protocol) {
    if (!protocol || !protocol[0]) {
        return false;
    }

    // Protocol must be a valid identifier:
    // - Start with letter
    // - Contain only letters, numbers, and hyphens
    if (!((protocol[0] >= 'a' && protocol[0] <= 'z') ||
          (protocol[0] >= 'A' && protocol[0] <= 'Z'))) {
        return false;
    }

    for (size_t i = 0; protocol[i]; i++) {
        char c = protocol[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '-')) {
            return false;
        }
    }

    return true;
}

// Validate key string
bool validate_key(const char* key) {
    if (!key || !key[0]) {
        return false;
    }

    // Key must be:
    // - At least 8 characters long
    // - Contain only printable ASCII characters
    // - No spaces or control characters
    size_t len = strlen(key);
    if (len < 8) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        char c = key[i];
        if (c < 33 || c > 126) {  // Non-printable ASCII or space
            return false;
        }
    }

    return true;
}

/*
 * Check if the websocket subsystem is ready to launch
 * 
 * This function performs readiness checks for the websocket subsystem by:
 * - Verifying system state and dependencies (Threads, Network)
 * - Checking protocol configuration (IPv4/IPv6)
 * - Validating interface availability
 * - Checking port and other configuration
 * 
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Note: Prefer using get_websocket_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_websocket_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_WEBSOCKET));

    // Register with registry if not already registered
    if (websocket_subsystem_id < 0) {
        websocket_subsystem_id = register_subsystem(SR_WEBSOCKET, NULL, NULL, NULL,
                                                  (int (*)(void))launch_websocket_subsystem,
                                                  (void (*)(void))stop_websocket_server);
    }

    // Register dependencies
    if (websocket_subsystem_id >= 0) {
        if (!add_dependency_from_launch(websocket_subsystem_id, SR_THREADS)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Threads dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_WEBSOCKET, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Threads dependency registered"));

        if (!add_dependency_from_launch(websocket_subsystem_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_WEBSOCKET, .ready = false, .messages = messages };
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
    
    // // 2. Check Network subsystem launch readiness (using cached version)
    // LaunchReadiness network_readiness = get_network_readiness();
    // if (!network_readiness.ready) {
    //     messages[msg_index++] = strdup("  No-Go:   Network subsystem not Go for Launch");
    //     ready = false;
    // } else {
    //     messages[msg_index++] = strdup("  Go:      Network subsystem Go for Launch");
    // }

    // 3. Check configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        ready = false;
    } else {
        if (!app_config->websocket.enable_ipv4) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   IPv4 disabled"));
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      IPv4 enabled"));
        }

                if (app_config->websocket.enable_ipv6) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:      IPv6 enabled"));
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:         IPv6 disabled"));
        }

        if (app_config->websocket.enable_ipv4 || app_config->websocket.enable_ipv6) {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:         At least one interface enabled"));    
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:      No interfaces enabled"));    
            ready = false;
        }

        // 4. Check interface availability
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

        if (!ipv4_available) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No IPv4 interfaces available"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      IPv4 interfaces available"));
        }

        if (app_config->websocket.enable_ipv6 && !ipv6_available) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No IPv6 interfaces available"));
            ready = false;
        } else if (app_config->websocket.enable_ipv6) {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      IPv6 interfaces available"));
        }

        // 5. Check port number
        int port = app_config->websocket.port;
        if (port != 80 && port != 443 && (port < MIN_PORT || port > MAX_PORT)) {
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

        // 6. Validate protocol (use default if not set)
        const char* protocol = app_config->websocket.protocol ? app_config->websocket.protocol : "hydrogen";
        if (!validate_protocol(protocol)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid protocol format"));
            ready = false;
        } else {
            char* protocol_msg = malloc(256);
            if (protocol_msg) {
                snprintf(protocol_msg, 256, "  Go:      Protocol format valid: %s", protocol);
                add_launch_message(&messages, &count, &capacity, protocol_msg);
            }
        }

        // 7. Validate key (use default if not set)
        const char* key = app_config->websocket.key ? app_config->websocket.key : "default_websocket_key";
        if (!validate_key(key)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid key format (must be at least 8 printable characters)"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      Key format valid"));
        }

        // 8. Validate message size limits
        if (app_config->websocket.max_message_size < WEBSOCKET_MIN_MESSAGE_SIZE ||
            app_config->websocket.max_message_size > WEBSOCKET_MAX_MESSAGE_SIZE) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid message size limits"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      Message size limits valid"));
        }

        // 9. Validate exit wait timeout
        if (app_config->websocket.connection_timeouts.exit_wait_seconds < MIN_EXIT_WAIT_SECONDS ||
            app_config->websocket.connection_timeouts.exit_wait_seconds > MAX_EXIT_WAIT_SECONDS) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid exit wait timeout (must be between 1-60 seconds)"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      Exit wait timeout valid"));
        }
    }

    // Final decision
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of WebSocket Subsystem" :
        "  Decide:  No-Go For Launch of WebSocket Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_WEBSOCKET,
        .ready = ready,
        .messages = messages
    };
}

// Launch websocket system
// Requires: Logging system
//
// The websocket server provides real-time status updates and monitoring.
// It is intentionally separate from the web server to:
// 1. Allow independent scaling
// 2. Enhance reliability through isolation
// 3. Support flexible deployment
// 4. Enable different security policies
int launch_websocket_subsystem(void) {
    log_this(SR_WEBSOCKET, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_WEBSOCKET, "LAUNCH: " SR_WEBSOCKET, LOG_LEVEL_STATE, 0);

    // Step 1: Verify system state
    log_this(SR_WEBSOCKET, "  Step 1: Verifying system state", LOG_LEVEL_STATE, 0);
    
    if (server_stopping || websocket_server_shutdown) {
        log_this(SR_WEBSOCKET, "Cannot initialize websocket during shutdown", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Failed: System in shutdown", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!server_starting) {
        log_this(SR_WEBSOCKET, "Cannot initialize websocket outside startup phase", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Failed: Not in startup phase", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!app_config) {
        log_this(SR_WEBSOCKET, "Configuration not loaded", LOG_LEVEL_STATE, 0);
        log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Failed: No configuration", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // if (!app_config->websocket.enabled) {
    //     log_this(SR_WEBSOCKET, "Websocket disabled in configuration", LOG_LEVEL_STATE, 0);
    //     log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Disabled by configuration", LOG_LEVEL_STATE, 0);
    //     return 1; // Not an error if disabled
    // }

    log_this(SR_WEBSOCKET, "    System state verified", LOG_LEVEL_STATE, 0);

    // Step 2: Initialize websocket server
    log_this(SR_WEBSOCKET, "  Step 2: Initializing websocket server", LOG_LEVEL_STATE, 0);
    
    // Use safe defaults for protocol and key if not configured
    const char* protocol = app_config->websocket.protocol ? app_config->websocket.protocol : "hydrogen";
    const char* key = app_config->websocket.key ? app_config->websocket.key : "default_websocket_key";
    
    if (init_websocket_server(app_config->websocket.port, protocol, key) != 0) {
        log_this(SR_WEBSOCKET, "Failed to initialize websocket server", LOG_LEVEL_ERROR, 0);
        log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Failed to initialize", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Step 3: Log configuration
    log_this(SR_WEBSOCKET, "  Step 3: Verifying configuration", LOG_LEVEL_STATE, 0);
    log_this(SR_WEBSOCKET, "    IPv6 support: %s", LOG_LEVEL_STATE, 1,  app_config->websocket.enable_ipv6 ? "enabled" : "disabled");
    log_this(SR_WEBSOCKET, "    Port: %d", LOG_LEVEL_STATE, 1, app_config->websocket.port);
    log_this(SR_WEBSOCKET, "    Protocol: %s", LOG_LEVEL_STATE, 1, protocol);
    log_this(SR_WEBSOCKET, "    Max Message Size: %zu", LOG_LEVEL_STATE, 1, app_config->websocket.max_message_size);
    log_this(SR_WEBSOCKET, "    Exit Wait Seconds: %d", LOG_LEVEL_STATE, 1, app_config->websocket.connection_timeouts.exit_wait_seconds);

    // Step 4: Start websocket server (which creates its thread)
    log_this(SR_WEBSOCKET, "  Step 4: Starting websocket server", LOG_LEVEL_STATE, 0);
    if (start_websocket_server() != 0) {
        log_this(SR_WEBSOCKET, "Failed to start websocket server", LOG_LEVEL_ERROR, 0);
        cleanup_websocket_server();
        return 0;
    }

    // Step 5: Wait for server to fully initialize (up to 10 seconds)
    log_this(SR_WEBSOCKET, "  Step 5: Waiting for initialization", LOG_LEVEL_STATE, 0);
    struct timespec wait_time = {0, 100000000}; // 100ms intervals
    int max_tries = 100; // 10 seconds total
    int tries = 0;
    bool server_ready = false;

    while (tries < max_tries) {
        nanosleep(&wait_time, NULL);
        
        // Check if server is running and bound to port
        int bound_port = get_websocket_port();
        if (bound_port > 0) {
            log_this(SR_WEBSOCKET, "    Server status:", LOG_LEVEL_STATE, 0);
            log_this(SR_WEBSOCKET, "    -> Bound to port: %d", LOG_LEVEL_STATE, 1, bound_port);
            log_this(SR_WEBSOCKET, "    -> IPv6: %s", LOG_LEVEL_STATE, 1, app_config->websocket.enable_ipv6 ? "enabled" : "disabled");
            
            // Log network interfaces
            log_this(SR_WEBSOCKET, "    Network interfaces:", LOG_LEVEL_STATE, 0);
            struct ifaddrs *ifaddr, *ifa;
            if (getifaddrs(&ifaddr) != -1) {
                for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                    if (ifa->ifa_addr == NULL)
                        continue;

                    int family = ifa->ifa_addr->sa_family;
                    if (family == AF_INET || (family == AF_INET6 && app_config->websocket.enable_ipv6)) {
                        char host[NI_MAXHOST];
                        int s = getnameinfo(ifa->ifa_addr,
                                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                                  sizeof(struct sockaddr_in6),
                                            host, NI_MAXHOST,
                                            NULL, 0, NI_NUMERICHOST);
                        if (s == 0) {
                            log_this(SR_WEBSOCKET, "    -> %s: %s (%s)", LOG_LEVEL_STATE, 3,
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
        tries++;
        
        if (tries % 10 == 0) { // Log every second
            log_this(SR_WEBSOCKET, "Still waiting for websocket server... (%d seconds)", LOG_LEVEL_STATE, 1, tries / 10);
        }
    }

    if (!server_ready) {
        log_this(SR_WEBSOCKET, "Websocket server failed to start within timeout", LOG_LEVEL_ERROR, 0);
        stop_websocket_server();
        cleanup_websocket_server();
        return 0;
    }

    // Step 6: Update registry and verify state
    log_this(SR_WEBSOCKET, "  Step 6: Updating subsystem registry", LOG_LEVEL_STATE, 0);
    update_subsystem_on_startup(SR_WEBSOCKET, true);
    
    SubsystemState final_state = get_subsystem_state(websocket_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Successfully launched and running", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_WEBSOCKET, "LAUNCH: WEBSOCKETS - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
    return 1;
}
