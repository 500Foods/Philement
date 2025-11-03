/*
 * Core Web Server Implementation
 *
 * Provides the implementation of the web server's core functionality,
 * including initialization, request handling, and shutdown procedures.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "web_server_core.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

// Endpoint registry
static WebServerEndpoint registered_endpoints[MAX_ENDPOINTS];
static size_t endpoint_count = 0;
static pthread_mutex_t endpoint_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global server state
struct MHD_Daemon *webserver_daemon = NULL;
WebServerConfig *server_web_config = NULL;

// Endpoint registration functions
bool register_web_endpoint(const WebServerEndpoint* endpoint) {
    if (!endpoint || !endpoint->prefix || !endpoint->validator || !endpoint->handler) {
        log_this(SR_WEBSERVER, "Invalid endpoint registration parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    MutexResult lock_result = MUTEX_LOCK(&endpoint_mutex, SR_WEBSERVER);
    if (lock_result == MUTEX_SUCCESS) {
        // Check for existing registration with same prefix
        for (size_t i = 0; i < endpoint_count; i++) {
            if (strcmp(registered_endpoints[i].prefix, endpoint->prefix) == 0) {
                mutex_unlock(&endpoint_mutex);
                log_this(SR_WEBSERVER, "Endpoint with prefix %s already registered", LOG_LEVEL_ERROR, 1, endpoint->prefix);
                return false;
            }
        }

        // Register new endpoint if space available
        if (endpoint_count < MAX_ENDPOINTS) {
            registered_endpoints[endpoint_count] = *endpoint;
            endpoint_count++;
            log_this(SR_WEBSERVER, "Registered endpoint with prefix: %s", LOG_LEVEL_DEBUG, 1, endpoint->prefix);
            mutex_unlock(&endpoint_mutex);
            return true;
        }

        mutex_unlock(&endpoint_mutex);
        log_this(SR_WEBSERVER, "Maximum number of endpoints reached", LOG_LEVEL_ERROR, 0);
        return false;
    }
    return false;
}

void unregister_web_endpoint(const char* prefix) {
    if (!prefix) return;

    MutexResult lock_result = MUTEX_LOCK(&endpoint_mutex, SR_WEBSERVER);
    if (lock_result == MUTEX_SUCCESS) {
        // Find and remove endpoint
        for (size_t i = 0; i < endpoint_count; i++) {
            if (strcmp(registered_endpoints[i].prefix, prefix) == 0) {
                // Shift remaining endpoints
                for (size_t j = i; j < endpoint_count - 1; j++) {
                    registered_endpoints[j] = registered_endpoints[j + 1];
                }
                endpoint_count--;
                log_this(SR_WEBSERVER, "Unregistered endpoint with prefix: %s", LOG_LEVEL_DEBUG, 1, prefix);
                break;
            }
        }

        mutex_unlock(&endpoint_mutex);
    }
}

// Get registered endpoint for URL
const WebServerEndpoint* get_endpoint_for_url(const char* url) {
    if (!url) return NULL;

    MutexResult lock_result = MUTEX_LOCK(&endpoint_mutex, SR_WEBSERVER);
    const WebServerEndpoint* matched_endpoint = NULL;

    if (lock_result == MUTEX_SUCCESS) {
        // Find matching endpoint
        for (size_t i = 0; i < endpoint_count; i++) {
            const WebServerEndpoint* endpoint = &registered_endpoints[i];
            if (strncmp(url, endpoint->prefix, strlen(endpoint->prefix)) == 0) {
                if (endpoint->validator(url)) {
                    matched_endpoint = endpoint;
                    break;
                }
            }
        }

        mutex_unlock(&endpoint_mutex);
    }
    return matched_endpoint;
}

// External state
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Check if a network port is available for binding
bool is_port_available(int port, bool check_ipv6) {
    bool ipv4_ok = false;

    // Check IPv4
    int sock_v4 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_v4 != -1) {
        // Enable SO_REUSEADDR to match the actual server behavior
        int reuse = 1;
        if (setsockopt(sock_v4, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            log_this(SR_WEBSERVER, "Failed to set SO_REUSEADDR on IPv4 test socket: %s", LOG_LEVEL_ALERT, 1, strerror(errno));
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)port);
        addr.sin_addr.s_addr = INADDR_ANY;

        int result = bind(sock_v4, (struct sockaddr*)&addr, sizeof(addr));
        ipv4_ok = (result == 0);

        if (!ipv4_ok) {
            log_this(SR_WEBSERVER, "IPv4 port %d availability check failed: %s", LOG_LEVEL_DEBUG, 2, port, strerror(errno));
        } else {
            log_this(SR_WEBSERVER, "IPv4 port %d is available (SO_REUSEADDR enabled)", LOG_LEVEL_DEBUG, 1, port);
        }

        close(sock_v4);
    }

    // Check IPv6 if requested
    if (check_ipv6) {
        bool ipv6_ok = false;
        int sock_v6 = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock_v6 != -1) {
            // Enable SO_REUSEADDR to match the actual server behavior
            int reuse = 1;
            if (setsockopt(sock_v6, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
                log_this(SR_WEBSERVER, "Failed to set SO_REUSEADDR on IPv6 test socket: %s", LOG_LEVEL_ALERT, 2, strerror(errno));
            }

            struct sockaddr_in6 addr;
            addr.sin6_family = AF_INET6;
            addr.sin6_port = htons((uint16_t)port);
            addr.sin6_addr = in6addr_any;

            // Enable dual-stack (IPv4 + IPv6) if possible
            int off = 0;
            setsockopt(sock_v6, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

            int result = bind(sock_v6, (struct sockaddr*)&addr, sizeof(addr));
            ipv6_ok = (result == 0);

            if (!ipv6_ok) {
                log_this(SR_WEBSERVER, "IPv6 port %d availability check failed: %s", LOG_LEVEL_DEBUG, 2, port, strerror(errno));
            } else {
                log_this(SR_WEBSERVER, "IPv6 port %d is available (SO_REUSEADDR enabled)", LOG_LEVEL_DEBUG, 1, port);
            }

            close(sock_v6);
        }
        return ipv6_ok;
    }

    return ipv4_ok;
}

void add_cors_headers(struct MHD_Response *response) {
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
}

bool init_web_server(WebServerConfig *web_config) {
    // Check all shutdown flags atomically
    
    // Prevent initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this(SR_WEBSERVER, "Cannot initialize web server during shutdown", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Check if we're already initialized
    if (webserver_daemon != NULL) {
        log_this(SR_WEBSERVER, "Web server already initialized", LOG_LEVEL_ALERT, 0);
        return false;
    }

    // Double-check shutdown state before proceeding
    if (server_stopping || web_server_shutdown) {
        log_this(SR_WEBSERVER, "Shutdown initiated, aborting web server initialization", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Store config only after checks pass
    server_web_config = web_config;

    // Check port availability before initializing resources
    if (!is_port_available(web_config->port, web_config->enable_ipv6)) {
        log_this(SR_WEBSERVER, "Port %u is not available", LOG_LEVEL_ERROR, 1, web_config->port);
        server_web_config = NULL;  // Reset config on failure
        return false;
    }

    log_this(SR_WEBSERVER, "Starting web server initialization", LOG_LEVEL_DEBUG, 0);
    if (web_config->enable_ipv6) {
        log_this(SR_WEBSERVER, "IPv6 support enabled", LOG_LEVEL_DEBUG, 0);
    }

    // Log server configuration
    log_this(SR_WEBSERVER, "Server Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_WEBSERVER, "― Port: %u", LOG_LEVEL_DEBUG, 1, server_web_config->port);
    log_this(SR_WEBSERVER, "― WebRoot: %s", LOG_LEVEL_DEBUG, 1, server_web_config->web_root);
    log_this(SR_WEBSERVER, "― Upload Path: %s", LOG_LEVEL_DEBUG, 1, server_web_config->upload_path);
    log_this(SR_WEBSERVER, "― Upload Dir: %s", LOG_LEVEL_DEBUG, 1, server_web_config->upload_dir);
    log_this(SR_WEBSERVER, "― Thread Pool Size: %d", LOG_LEVEL_DEBUG, 1, web_config->thread_pool_size);
    log_this(SR_WEBSERVER, "― Max Connections: %d", LOG_LEVEL_DEBUG, 1, web_config->max_connections);
    log_this(SR_WEBSERVER, "― Max Connections Per IP: %d", LOG_LEVEL_DEBUG, 1, web_config->max_connections_per_ip);
    log_this(SR_WEBSERVER, "― Connection Timeout: %d seconds", LOG_LEVEL_DEBUG, 1, web_config->connection_timeout);

    // Create upload directory if it doesn't exist
    struct stat st = {0};
    if (stat(server_web_config->upload_dir, &st) == -1) {
        log_this(SR_WEBSERVER, "Upload directory does not exist, attempting to create", LOG_LEVEL_DEBUG, 0);
        if (mkdir(server_web_config->upload_dir, 0700) != 0) {
            char error_buffer[256];
            snprintf(error_buffer, sizeof(error_buffer), "Failed to create upload directory: %s", strerror(errno));
            log_this(SR_WEBSERVER, error_buffer, LOG_LEVEL_DEBUG, 0);
            return false;
        }
        log_this(SR_WEBSERVER, "Created upload directory", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_WEBSERVER, "Upload directory already exists", LOG_LEVEL_DEBUG, 0);
    }

    return true;
}

void* run_web_server(void* arg) {
    (void)arg; // Unused parameter

    // Check all shutdown flags atomically
    
    // Prevent initialization during any shutdown state
    if (server_stopping || web_server_shutdown) {
        log_this(SR_WEBSERVER, "Cannot start web server during shutdown", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this(SR_WEBSERVER, "Cannot start web server outside startup phase", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Check if we already have a daemon running
    if (webserver_daemon != NULL) {
        log_this(SR_WEBSERVER, "Web server daemon already exists", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Double-check shutdown state before proceeding with resource allocation
    if (server_stopping || web_server_shutdown) {
        log_this(SR_WEBSERVER, "Shutdown initiated, aborting web server startup", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Triple-check shutdown state before thread registration
    if (server_stopping || web_server_shutdown || !server_starting) {
        log_this(SR_WEBSERVER, "Invalid system state, aborting web server startup", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    log_this(SR_WEBSERVER, "Starting web server daemon", LOG_LEVEL_DEBUG, 0);
    
    // Initialize network interface logging
    log_this(SR_WEBSERVER, "Initializing network interfaces", LOG_LEVEL_DEBUG, 0);
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        log_this(SR_WEBSERVER, "Failed to get interface addresses", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Log available interfaces and their addresses
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || (family == AF_INET6 && server_web_config->enable_ipv6)) {
            char host[NI_MAXHOST];
            int s = getnameinfo(ifa->ifa_addr,
                              (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                  sizeof(struct sockaddr_in6),
                              host, NI_MAXHOST,
                              NULL, 0, NI_NUMERICHOST);
            if (s == 0) {
                log_this(SR_WEBSERVER, "Interface %s: %s (%s)", LOG_LEVEL_DEBUG, 3,
                        ifa->ifa_name, 
                        host,
                        (family == AF_INET) ? "IPv4" : "IPv6");
            }
        }
    }
    freeifaddrs(ifaddr);

    // Configure server flags for proper thread handling
    unsigned int flags = MHD_USE_INTERNAL_POLLING_THREAD;  // Base flag for internal polling
    
    // Add thread pool support
    flags |= MHD_USE_SELECT_INTERNALLY;
    
    if (server_web_config->enable_ipv6) {
        flags |= MHD_USE_DUAL_STACK;
        log_this(SR_WEBSERVER, "Starting with IPv6 dual-stack support", LOG_LEVEL_DEBUG, 0);
    }

    log_this(SR_WEBSERVER, "Setting SO_REUSEADDR to enable immediate socket rebinding", LOG_LEVEL_DEBUG, 0);
    log_this(SR_WEBSERVER, "Using internal polling thread with select", LOG_LEVEL_DEBUG, 0);
    log_this(SR_WEBSERVER, "Maximum connections: %d", LOG_LEVEL_DEBUG, 1, server_web_config->max_connections);
    log_this(SR_WEBSERVER, "Maximum connections per IP: %d", LOG_LEVEL_DEBUG, 1, server_web_config->max_connections_per_ip);
    log_this(SR_WEBSERVER, "Connection timeout: %d seconds", LOG_LEVEL_DEBUG, 1, server_web_config->connection_timeout);
    
    // Start the daemon with proper thread configuration
    webserver_daemon = MHD_start_daemon(flags | MHD_USE_DEBUG | MHD_USE_ERROR_LOG,
                                (uint16_t)server_web_config->port, 
                                NULL, NULL,
                                &handle_request, NULL,
                                MHD_OPTION_THREAD_POOL_SIZE, server_web_config->thread_pool_size,
                                MHD_OPTION_CONNECTION_LIMIT, server_web_config->max_connections,
                                MHD_OPTION_PER_IP_CONNECTION_LIMIT, server_web_config->max_connections_per_ip,
                                MHD_OPTION_CONNECTION_TIMEOUT, server_web_config->connection_timeout,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                                MHD_OPTION_LISTENING_ADDRESS_REUSE, 1, // Enable SO_REUSEADDR for port rebinding
                                MHD_OPTION_THREAD_STACK_SIZE, (1024 * 1024), // 1MB stack size
                                MHD_OPTION_END);
    if (webserver_daemon == NULL) {
        log_this(SR_WEBSERVER, "Failed to start web server daemon", LOG_LEVEL_DEBUG, 0);
        server_web_config = NULL;  // Reset config on failure
        log_this(SR_WEBSERVER, "Web server initialization failed", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Check if the web server is actually running
    const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
    if (info == NULL) {
        log_this(SR_WEBSERVER, "Failed to get daemon info", LOG_LEVEL_DEBUG, 0);
        MHD_stop_daemon(webserver_daemon);
        webserver_daemon = NULL;
        return NULL;
    }

    unsigned int actual_port = info->port;
    if (actual_port == 0) {
        log_this(SR_WEBSERVER, "Web server failed to bind to the specified port", LOG_LEVEL_DEBUG, 0);
        MHD_stop_daemon(webserver_daemon);
        webserver_daemon = NULL;
        return NULL;
    }

    char port_info[64];
    snprintf(port_info, sizeof(port_info), "Web server bound to port: %u", actual_port);
    log_this(SR_WEBSERVER, port_info, LOG_LEVEL_DEBUG, 0);

    log_this(SR_WEBSERVER, "Web server started successfully", LOG_LEVEL_DEBUG, 0);

    return NULL;
}

void shutdown_web_server(void) {
    // Set shutdown flag first to prevent reinitialization
    __sync_bool_compare_and_swap(&web_server_shutdown, 0, 1);
    __sync_synchronize();  // Memory barrier

    log_this(SR_WEBSERVER, "Shutdown: Initiating web server shutdown", LOG_LEVEL_DEBUG, 0);
    
    // Stop the web server daemon
    if (webserver_daemon != NULL) {
        log_this(SR_WEBSERVER, "Stopping web server daemon", LOG_LEVEL_DEBUG, 0);
        MHD_stop_daemon(webserver_daemon);
        webserver_daemon = NULL;
        log_this(SR_WEBSERVER, "Web server daemon stopped", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_WEBSERVER, "Web server was not running", LOG_LEVEL_DEBUG, 0);
    }

    // Clear configuration
    server_web_config = NULL;

    log_this(SR_WEBSERVER, "Web server shutdown complete", LOG_LEVEL_DEBUG, 0);
}

const char* get_upload_path(void) {
    return server_web_config->upload_path;
}

// WebRoot path resolution functions

/**
 * Resolve a WebRoot specification to an actual path
 * Supports PAYLOAD:/ paths and filesystem paths
 */
char* resolve_webroot_path(const char* webroot_spec, const PayloadData* payload,
                          AppConfig* config __attribute__((unused))) {
    if (webroot_spec == NULL) return NULL;

    if (strncmp(webroot_spec, "PAYLOAD:", 8) == 0) {
        // Extract from payload using subdirectory function
        return get_payload_subdirectory_path(payload, webroot_spec + 8, config);
    } else {
        // Use as filesystem path directly
        return resolve_filesystem_path(webroot_spec, config);
    }
}

/**
 * Extract files from a payload subdirectory
 * Creates a virtual filesystem structure from tar data
 */
char* get_payload_subdirectory_path(const PayloadData* payload, const char* subdir,
                                 AppConfig* config __attribute__((unused))) {
    static char buffer[PATH_MAX];

    if (!payload || !subdir) {
        log_this(SR_WEBSERVER, "Invalid payload or subdirectory parameter", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Build prefix to search for (e.g., "terminal/")
    char prefix[256];
    if (snprintf(prefix, sizeof(prefix), "%s/", subdir) >= (int)sizeof(prefix)) {
        log_this(SR_WEBSERVER, "Subdirectory prefix too long", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Log the payload extraction request
    log_this(SR_WEBSERVER, "Resolving payload subdirectory: %s", LOG_LEVEL_DEBUG, 1, prefix);

    // For now, return a placeholder path since payload extraction requires tar parsing
    // This would need implementation based on existing swagger payload handling
    if (snprintf(buffer, sizeof(buffer), "/payload/%s", subdir) >= (int)sizeof(buffer)) {
        log_this(SR_WEBSERVER, "Payload path buffer overflow", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_WEBSERVER, "Resolved payload path: %s", LOG_LEVEL_DEBUG, 1, buffer);
    return strdup(buffer);
}

/**
 * Resolve a filesystem path specification
 * Handles relative paths and environment variable substitution
 */
char* resolve_filesystem_path(const char* path_spec, AppConfig* config __attribute__((unused))) {
    static char buffer[PATH_MAX];

    if (!path_spec) {
        log_this(SR_WEBSERVER, "No path specification provided", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // For absolute paths, use as-is
    if (path_spec[0] == '/') {
        log_this(SR_WEBSERVER, "Using absolute filesystem path: %s", LOG_LEVEL_DEBUG, 1, path_spec);
        return strdup(path_spec);
    }

    // Build relative path from web server webroot (if available)
    const char* base_path = server_web_config && server_web_config->web_root
                           ? server_web_config->web_root : ".";

    if (snprintf(buffer, sizeof(buffer), "%s/%s", base_path, path_spec) >= (int)sizeof(buffer)) {
        log_this(SR_WEBSERVER, "Filesystem path buffer overflow", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_WEBSERVER, "Resolved filesystem path: %s", LOG_LEVEL_DEBUG, 1, buffer);
    return strdup(buffer);
}
