// Network constants
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x02
#endif

// Include core header first for default constants
#include "web_server_core.h"

// System headers
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Project headers
#include "../swagger/swagger.h"
#include "../utils/utils_threads.h"
#include "../logging/logging.h"

// Global server state
struct MHD_Daemon *web_daemon = NULL;
WebServerConfig *server_web_config = NULL;

// External state
extern AppConfig *app_config;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Check if a network port is available for binding
static bool is_port_available(int port, bool check_ipv6) {
    bool ipv4_ok = false;
    bool ipv6_ok = false;

    // Check IPv4
    int sock_v4 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_v4 != -1) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        int result = bind(sock_v4, (struct sockaddr*)&addr, sizeof(addr));
        ipv4_ok = (result == 0);
        close(sock_v4);
    }

    // Check IPv6 if requested
    if (check_ipv6) {
        int sock_v6 = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock_v6 != -1) {
            struct sockaddr_in6 addr;
            addr.sin6_family = AF_INET6;
            addr.sin6_port = htons(port);
            addr.sin6_addr = in6addr_any;

            // Enable dual-stack (IPv4 + IPv6) if possible
            int off = 0;
            setsockopt(sock_v6, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

            int result = bind(sock_v6, (struct sockaddr*)&addr, sizeof(addr));
            ipv6_ok = (result == 0);
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
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t web_server_shutdown;
    
    // Prevent initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this("WebServer", "Cannot initialize web server during shutdown", LOG_LEVEL_STATE);
        return false;
    }

    // Check if we're already initialized
    if (web_daemon != NULL) {
        log_this("WebServer", "Web server already initialized", LOG_LEVEL_ALERT);
        return false;
    }

    // Double-check shutdown state before proceeding
    if (server_stopping || web_server_shutdown) {
        log_this("WebServer", "Shutdown initiated, aborting web server initialization", LOG_LEVEL_STATE);
        return false;
    }

    // Store config only after checks pass
    server_web_config = web_config;

    // Check port availability before initializing resources
    if (!is_port_available(web_config->port, web_config->enable_ipv6)) {
        log_this("WebServer", "Port %u is not available", LOG_LEVEL_ERROR, web_config->port);
        server_web_config = NULL;  // Reset config on failure
        return false;
    }

    log_this("WebServer", "Starting web server initialization", LOG_LEVEL_STATE);
    if (web_config->enable_ipv6) {
        log_this("WebServer", "IPv6 support enabled", LOG_LEVEL_STATE);
    }
    log_this("WebServer", "-> Port: %u", LOG_LEVEL_STATE, server_web_config->port);
    log_this("WebServer", "-> WebRoot: %s", LOG_LEVEL_STATE, server_web_config->web_root);
    log_this("WebServer", "-> Upload Path: %s", LOG_LEVEL_STATE, server_web_config->upload_path);
    log_this("WebServer", "-> Upload Dir: %s", LOG_LEVEL_STATE, server_web_config->upload_dir);

    // Initialize thread pool and connection settings with defaults if not set
    if (web_config->thread_pool_size == 0) {
        web_config->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    }
    if (web_config->max_connections == 0) {
        web_config->max_connections = DEFAULT_MAX_CONNECTIONS;
    }
    if (web_config->max_connections_per_ip == 0) {
        web_config->max_connections_per_ip = DEFAULT_MAX_CONNECTIONS_PER_IP;
    }
    if (web_config->connection_timeout == 0) {
        web_config->connection_timeout = DEFAULT_CONNECTION_TIMEOUT;
    }

    log_this("WebServer", "-> Thread Pool Size: %d", LOG_LEVEL_STATE, web_config->thread_pool_size);
    log_this("WebServer", "-> Max Connections: %d", LOG_LEVEL_STATE, web_config->max_connections);
    log_this("WebServer", "-> Max Connections Per IP: %d", LOG_LEVEL_STATE, web_config->max_connections_per_ip);
    log_this("WebServer", "-> Connection Timeout: %d seconds", LOG_LEVEL_STATE, web_config->connection_timeout);

    // Initialize Swagger support if enabled
    if (web_config->swagger->enabled) {
        log_this("WebServer", "Initializing Swagger UI support", LOG_LEVEL_STATE);
        if (init_swagger_support(web_config)) {
            log_this("WebServer", "-> Swagger UI enabled at prefix: %s", LOG_LEVEL_STATE, 
                    web_config->swagger->prefix);
        } else {
            log_this("WebServer", "-> Swagger UI initialization failed", LOG_LEVEL_ALERT);
            return false;
        }
    }

    // Create upload directory if it doesn't exist
    struct stat st = {0};
    if (stat(server_web_config->upload_dir, &st) == -1) {
        log_this("WebServer", "Upload directory does not exist, attempting to create", LOG_LEVEL_ALERT);
        if (mkdir(server_web_config->upload_dir, 0700) != 0) {
            char error_buffer[256];
            snprintf(error_buffer, sizeof(error_buffer), "Failed to create upload directory: %s", strerror(errno));
            log_this("WebServer", error_buffer, LOG_LEVEL_DEBUG);
            return false;
        }
        log_this("WebServer", "Created upload directory", LOG_LEVEL_STATE);
    } else {
        log_this("WebServer", "Upload directory already exists", LOG_LEVEL_ALERT);
    }

    return true;
}

// Option constants for microhttpd
#ifndef MHD_OPTION_LISTENING_ADDRESS_REUSE
#define MHD_OPTION_LISTENING_ADDRESS_REUSE 5
#endif

void* run_web_server(void* arg) {
    (void)arg; // Unused parameter

    // Check all shutdown flags atomically
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_starting;
    extern volatile sig_atomic_t web_server_shutdown;
    
    // Prevent initialization during any shutdown state
    if (server_stopping || web_server_shutdown) {
        log_this("WebServer", "Cannot start web server during shutdown", LOG_LEVEL_STATE);
        return NULL;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this("WebServer", "Cannot start web server outside startup phase", LOG_LEVEL_STATE);
        return NULL;
    }

    // Check if we already have a daemon running
    if (web_daemon != NULL) {
        log_this("WebServer", "Web server daemon already exists", LOG_LEVEL_ALERT);
        return NULL;
    }

    // Double-check shutdown state before proceeding with resource allocation
    if (server_stopping || web_server_shutdown) {
        log_this("WebServer", "Shutdown initiated, aborting web server startup", LOG_LEVEL_STATE);
        return NULL;
    }

    // Triple-check shutdown state before thread registration
    if (server_stopping || web_server_shutdown || !server_starting) {
        log_this("WebServer", "Invalid system state, aborting web server startup", LOG_LEVEL_STATE);
        return NULL;
    }

    log_this("WebServer", "Starting web server daemon", LOG_LEVEL_STATE);
    
    // Initialize network interface logging
    log_this("WebServer", "Initializing network interfaces", LOG_LEVEL_STATE);
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        log_this("WebServer", "Failed to get interface addresses", LOG_LEVEL_ERROR);
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
                log_this("WebServer", "Interface %s: %s (%s)", LOG_LEVEL_STATE,
                        ifa->ifa_name, host,
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
        log_this("WebServer", "Starting with IPv6 dual-stack support", LOG_LEVEL_STATE);
    }

    log_this("WebServer", "Setting SO_REUSEADDR to enable immediate socket rebinding", LOG_LEVEL_STATE);
    log_this("WebServer", "Using internal polling thread with select", LOG_LEVEL_STATE);
    log_this("WebServer", "Maximum connections: %d", LOG_LEVEL_STATE, server_web_config->max_connections);
    log_this("WebServer", "Maximum connections per IP: %d", LOG_LEVEL_STATE, server_web_config->max_connections_per_ip);
    log_this("WebServer", "Connection timeout: %d seconds", LOG_LEVEL_STATE, server_web_config->connection_timeout);
    
    // Start the daemon with proper thread configuration
    web_daemon = MHD_start_daemon(flags | MHD_USE_DEBUG | MHD_USE_ERROR_LOG,
                                server_web_config->port, 
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
    if (web_daemon == NULL) {
        log_this("WebServer", "Failed to start web server daemon", LOG_LEVEL_ERROR);
        server_web_config = NULL;  // Reset config on failure
        log_this("WebServer", "Web server initialization failed", LOG_LEVEL_DEBUG);
        return NULL;
    }

    // Check if the web server is actually running
    const union MHD_DaemonInfo *info = MHD_get_daemon_info(web_daemon, MHD_DAEMON_INFO_BIND_PORT);
    if (info == NULL) {
        log_this("WebServer", "Failed to get daemon info", LOG_LEVEL_ERROR);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        return NULL;
    }

    unsigned int actual_port = info->port;
    if (actual_port == 0) {
        log_this("WebServer", "Web server failed to bind to the specified port", LOG_LEVEL_ERROR);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        return NULL;
    }

    char port_info[64];
    snprintf(port_info, sizeof(port_info), "Web server bound to port: %u", actual_port);
    log_this("WebServer", port_info, LOG_LEVEL_STATE);

    log_this("WebServer", "Web server started successfully", LOG_LEVEL_STATE);

    return NULL;
}

void shutdown_web_server(void) {
    // Set shutdown flag first to prevent reinitialization
    extern volatile sig_atomic_t web_server_shutdown;
    __sync_bool_compare_and_swap(&web_server_shutdown, 0, 1);
    __sync_synchronize();  // Memory barrier

    log_this("WebServer", "Shutdown: Initiating web server shutdown", LOG_LEVEL_STATE);
    
    // Clean up Swagger resources if enabled
    if (server_web_config && server_web_config->swagger->enabled) {
        cleanup_swagger_support();
        log_this("WebServer", "Swagger UI resources cleaned up", LOG_LEVEL_STATE);
    }
    
    // Stop the web server daemon
    if (web_daemon != NULL) {
        log_this("WebServer", "Stopping web server daemon", LOG_LEVEL_STATE);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        log_this("WebServer", "Web server daemon stopped", LOG_LEVEL_STATE);
    } else {
        log_this("WebServer", "Web server was not running", LOG_LEVEL_STATE);
    }

    // Clear configuration
    server_web_config = NULL;

    log_this("WebServer", "Web server shutdown complete", LOG_LEVEL_STATE);
}

const char* get_upload_path(void) {
    return server_web_config->upload_path;
}