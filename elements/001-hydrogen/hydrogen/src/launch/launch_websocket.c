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
#include "../websocket/websocket_server.h"
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

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_EXIT_WAIT_SECONDS 1
#define MAX_EXIT_WAIT_SECONDS 60
#define WEBSOCKET_MIN_MESSAGE_SIZE 1024        // 1KB
#define WEBSOCKET_MAX_MESSAGE_SIZE 0x40000000  // 1GB

// External declarations
extern ServiceThreads websocket_threads;
extern pthread_t websocket_thread;
extern volatile sig_atomic_t websocket_server_shutdown;
extern AppConfig* app_config;
extern volatile sig_atomic_t server_starting;

// Registry ID and cached readiness state
int websocket_subsystem_id = -1;
static LaunchReadiness cached_readiness = {0};
static bool readiness_cached = false;

// Forward declarations
static void clear_cached_readiness(void);
static void register_websocket(void);
static bool validate_protocol(const char* protocol);
static bool validate_key(const char* key);

// Helper to clear cached readiness
static void clear_cached_readiness(void) {
    if (readiness_cached && cached_readiness.messages) {
        free_readiness_messages(&cached_readiness);
        readiness_cached = false;
    }
}

// Get cached readiness result
LaunchReadiness get_websocket_readiness(void) {
    if (readiness_cached) {
        return cached_readiness;
    }
    
    // Perform fresh check and cache result
    cached_readiness = check_websocket_launch_readiness();
    readiness_cached = true;
    return cached_readiness;
}

// Register the websocket subsystem with the registry
static void register_websocket(void) {
    // Always register during readiness check if not already registered
    if (websocket_subsystem_id < 0) {
        websocket_subsystem_id = register_subsystem("WebSocket", NULL, NULL, NULL,
                                                  (int (*)(void))launch_websocket_subsystem,
                                                  (void (*)(void))stop_websocket_server);
    }
}

// Validate protocol string
static bool validate_protocol(const char* protocol) {
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
static bool validate_key(const char* key) {
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
    const char** messages = malloc(20 * sizeof(char*));  // Space for messages + NULL
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "WebSocket", .ready = false, .messages = NULL };
    }
    
    int msg_index = 0;
    bool ready = true;
    
    // First message is subsystem name
    messages[msg_index++] = strdup("WebSocket");
    
    // Register with registry if not already registered
    register_websocket();
    
    LaunchReadiness readiness = { .subsystem = "WebSocket", .ready = false, .messages = messages };
    
    // Register dependencies
    if (websocket_subsystem_id >= 0) {
        if (!add_dependency_from_launch(websocket_subsystem_id, "Threads")) {
            messages[msg_index++] = strdup("  No-Go:   Failed to register Threads dependency");
            messages[msg_index] = NULL;
            free_readiness_messages(&readiness);
            return readiness;
        }
        messages[msg_index++] = strdup("  Go:      Threads dependency registered");

        if (!add_dependency_from_launch(websocket_subsystem_id, "Network")) {
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
    
    // 3. Check configuration
    if (!app_config) {
        messages[msg_index++] = strdup("  No-Go:   Configuration not loaded");
        ready = false;
    } else {
        if (!app_config->websocket.enabled) {
            messages[msg_index++] = strdup("  No-Go:   WebSocket server disabled in configuration");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      WebSocket server enabled");
        }
        
        // Check protocol configuration
        if (app_config->websocket.enable_ipv6) {
            messages[msg_index++] = strdup("  Go:      IPv6 enabled");
        } else {
            messages[msg_index++] = strdup("  Go:      IPv4 only (IPv6 disabled)");
        }
        
        // 4. Check interface availability
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
        
        if (!ipv4_available) {
            messages[msg_index++] = strdup("  No-Go:   No IPv4 interfaces available");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      IPv4 interfaces available");
        }
        
        if (app_config->websocket.enable_ipv6 && !ipv6_available) {
            messages[msg_index++] = strdup("  No-Go:   No IPv6 interfaces available");
            ready = false;
        } else if (app_config->websocket.enable_ipv6) {
            messages[msg_index++] = strdup("  Go:      IPv6 interfaces available");
        }
        
        // 5. Check port number
        int port = app_config->websocket.port;
        if (port != 80 && port != 443 && (port < MIN_PORT || port > MAX_PORT)) {
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
        
        // 6. Validate protocol (use default if not set)
        const char* protocol = app_config->websocket.protocol ? app_config->websocket.protocol : "hydrogen";
        if (!validate_protocol(protocol)) {
            messages[msg_index++] = strdup("  No-Go:   Invalid protocol format");
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Protocol format valid: %s", protocol);
                messages[msg_index++] = msg;
            }
        }

        // 7. Validate key (use default if not set)
        const char* key = app_config->websocket.key ? app_config->websocket.key : "default_websocket_key";
        if (!validate_key(key)) {
            messages[msg_index++] = strdup("  No-Go:   Invalid key format (must be at least 8 printable characters)");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      Key format valid");
        }

        // 8. Validate message size limits
        if (app_config->websocket.max_message_size < WEBSOCKET_MIN_MESSAGE_SIZE ||
            app_config->websocket.max_message_size > WEBSOCKET_MAX_MESSAGE_SIZE) {
            messages[msg_index++] = strdup("  No-Go:   Invalid message size limits");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      Message size limits valid");
        }

        // 9. Validate exit wait timeout
        if (app_config->websocket.connection_timeouts.exit_wait_seconds < MIN_EXIT_WAIT_SECONDS ||
            app_config->websocket.connection_timeouts.exit_wait_seconds > MAX_EXIT_WAIT_SECONDS) {
            messages[msg_index++] = strdup("  No-Go:   Invalid exit wait timeout (must be between 1-60 seconds)");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      Exit wait timeout valid");
        }
    }
    
    // Final decision
    messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of WebSocket Subsystem" :
        "  Decide:  No-Go For Launch of WebSocket Subsystem");
    messages[msg_index] = NULL;
    
    return (LaunchReadiness){
        .subsystem = "WebSocket",
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
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t websocket_server_shutdown;
    // Clear any cached readiness before checking final state
    clear_cached_readiness();
    log_this("WebSocket", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("WebSocket", "LAUNCH: WEBSOCKETS", LOG_LEVEL_STATE);

    // Step 1: Verify system state
    log_this("WebSocket", "  Step 1: Verifying system state", LOG_LEVEL_STATE);
    
    if (server_stopping || websocket_server_shutdown) {
        log_this("WebSocket", "Cannot initialize websocket during shutdown", LOG_LEVEL_STATE);
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Failed: System in shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    if (!server_starting) {
        log_this("WebSocket", "Cannot initialize websocket outside startup phase", LOG_LEVEL_STATE);
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Failed: Not in startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config) {
        log_this("WebSocket", "Configuration not loaded", LOG_LEVEL_STATE);
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Failed: No configuration", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->websocket.enabled) {
        log_this("WebSocket", "Websocket disabled in configuration", LOG_LEVEL_STATE);
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Disabled by configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    log_this("WebSocket", "    System state verified", LOG_LEVEL_STATE);

    // Step 2: Initialize websocket server
    log_this("WebSocket", "  Step 2: Initializing websocket server", LOG_LEVEL_STATE);
    
    // Use safe defaults for protocol and key if not configured
    const char* protocol = app_config->websocket.protocol ? app_config->websocket.protocol : "hydrogen";
    const char* key = app_config->websocket.key ? app_config->websocket.key : "default_websocket_key";
    
    if (init_websocket_server(app_config->websocket.port, protocol, key) != 0) {
        log_this("WebSocket", "Failed to initialize websocket server", LOG_LEVEL_ERROR);
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Failed to initialize", LOG_LEVEL_STATE);
        return 0;
    }

    // Step 3: Log configuration
    log_this("WebSocket", "  Step 3: Verifying configuration", LOG_LEVEL_STATE);
    log_this("WebSocket", "    IPv6 support: %s", LOG_LEVEL_STATE, 
             app_config->websocket.enable_ipv6 ? "enabled" : "disabled");
    log_this("WebSocket", "    Port: %d", LOG_LEVEL_STATE, 
             app_config->websocket.port);
    log_this("WebSocket", "    Protocol: %s", LOG_LEVEL_STATE, protocol);
    log_this("WebSocket", "    Max Message Size: %zu", LOG_LEVEL_STATE, 
             app_config->websocket.max_message_size);
    log_this("WebSocket", "    Exit Wait Seconds: %d", LOG_LEVEL_STATE, 
             app_config->websocket.connection_timeouts.exit_wait_seconds);

    // Step 4: Start websocket server (which creates its thread)
    log_this("WebSocket", "  Step 4: Starting websocket server", LOG_LEVEL_STATE);
    if (start_websocket_server() != 0) {
        log_this("WebSocket", "Failed to start websocket server", LOG_LEVEL_ERROR);
        cleanup_websocket_server();
        return 0;
    }

    // Step 5: Wait for server to fully initialize (up to 10 seconds)
    log_this("WebSocket", "  Step 5: Waiting for initialization", LOG_LEVEL_STATE);
    struct timespec wait_time = {0, 100000000}; // 100ms intervals
    int max_tries = 100; // 10 seconds total
    int tries = 0;
    bool server_ready = false;

    while (tries < max_tries) {
        nanosleep(&wait_time, NULL);
        
        // Check if server is running and bound to port
        int bound_port = get_websocket_port();
        if (bound_port > 0) {
            log_this("WebSocket", "    Server status:", LOG_LEVEL_STATE);
            log_this("WebSocket", "    -> Bound to port: %d", LOG_LEVEL_STATE, bound_port);
            log_this("WebSocket", "    -> IPv6: %s", LOG_LEVEL_STATE, 
                     app_config->websocket.enable_ipv6 ? "enabled" : "disabled");
            
            // Log network interfaces
            log_this("WebSocket", "    Network interfaces:", LOG_LEVEL_STATE);
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
                            log_this("WebSocket", "    -> %s: %s (%s)", LOG_LEVEL_STATE,
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
        tries++;
        
        if (tries % 10 == 0) { // Log every second
            log_this("WebSocket", "Still waiting for websocket server... (%d seconds)", LOG_LEVEL_STATE, tries / 10);
        }
    }

    if (!server_ready) {
        log_this("WebSocket", "Websocket server failed to start within timeout", LOG_LEVEL_ERROR);
        stop_websocket_server();
        cleanup_websocket_server();
        return 0;
    }

    // Step 6: Update registry and verify state
    log_this("WebSocket", "  Step 6: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("WebSocket", true);
    
    SubsystemState final_state = get_subsystem_state(websocket_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Successfully launched and running", LOG_LEVEL_STATE);
    } else {
        log_this("WebSocket", "LAUNCH: WEBSOCKETS - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                 subsystem_state_to_string(final_state));
        return 0;
    }
    return 1;
}

// Check if websocket server is running
// This function checks if the websocket server is currently running
// and available to handle requests.
int is_websocket_server_running(void) {
    extern volatile sig_atomic_t websocket_server_shutdown;
    
    // Server is running if:
    // 1. Enabled in config
    // 2. Not in shutdown state
    return (app_config && app_config->websocket.enabled && !websocket_server_shutdown);
}
