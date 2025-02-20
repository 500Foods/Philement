// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "web_server_core.h"
#include "utils_threads.h"
#include "logging.h"

// System headers
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

// Global server state
struct MHD_Daemon *web_daemon = NULL;
WebConfig *server_web_config = NULL;

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

bool init_web_server(WebConfig *web_config) {
    if (!is_port_available(web_config->port, web_config->enable_ipv6)) {
        log_this("WebServer", "Port is not available", 3, true, false, true);
        return false;
    }

    server_web_config = web_config;

    log_this("WebServer", "Initializing web server", 0, true, false, true);
    if (web_config->enable_ipv6) {
        log_this("WebServer", "IPv6 support enabled", 0, true, false, true);
    }
    log_this("WebServer", "-> Port: %u", 0, true, true, true, server_web_config->port);
    log_this("WebServer", "-> WebRoot: %s", 0, true, true, true, server_web_config->web_root);
    log_this("WebServer", "-> Upload Path: %s", 0, true, true, true, server_web_config->upload_path);
    log_this("WebServer", "-> Upload Dir: %s", 0, true, true, true, server_web_config->upload_dir);
    log_this("WebServer", "-> LogLevel: %s", 0, true, true, true, server_web_config->log_level);

    // Create upload directory if it doesn't exist
    struct stat st = {0};
    if (stat(server_web_config->upload_dir, &st) == -1) {
        log_this("WebServer", "Upload directory does not exist, attempting to create", 2, true, false, true);
        if (mkdir(server_web_config->upload_dir, 0700) != 0) {
            char error_buffer[256];
            snprintf(error_buffer, sizeof(error_buffer), "Failed to create upload directory: %s", strerror(errno));
            log_this("WebServer", error_buffer, 3, true, false, true);
            return false;
        }
        log_this("WebServer", "Created upload directory", 0, true, false, true);
    } else {
        log_this("WebServer", "Upload directory already exists", 2, true, false, true);
    }

    return true;
}

void* run_web_server(void* arg) {
    (void)arg; // Unused parameter

    log_this("WebServer", "Starting web server", 0, true, false, true);

    // Register main web server thread
    extern ServiceThreads web_threads;
    add_service_thread(&web_threads, pthread_self());
    unsigned int flags = MHD_USE_THREAD_PER_CONNECTION;
    if (server_web_config->enable_ipv6) {
        flags |= MHD_USE_DUAL_STACK;
        log_this("WebServer", "Starting with IPv6 dual-stack support", 0, true, false, true);
    }

    web_daemon = MHD_start_daemon(flags, server_web_config->port, NULL, NULL,
                                &handle_request, NULL,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                                MHD_OPTION_THREAD_STACK_SIZE, (1024 * 1024), // 1MB stack size
                                MHD_OPTION_END);
    if (web_daemon == NULL) {
        log_this("WebServer", "Failed to start web server", 4, true, false, true);
        return NULL;
    }

    // Check if the web server is actually running
    const union MHD_DaemonInfo *info = MHD_get_daemon_info(web_daemon, MHD_DAEMON_INFO_BIND_PORT);
    if (info == NULL) {
        log_this("WebServer", "Failed to get daemon info", 4, true, false, true);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        return NULL;
    }

    unsigned int actual_port = info->port;
    if (actual_port == 0) {
        log_this("WebServer", "Web server failed to bind to the specified port", 4, true, false, true);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        return NULL;
    }

    char port_info[64];
    snprintf(port_info, sizeof(port_info), "Web server bound to port: %u", actual_port);
    log_this("WebServer", port_info, 0, true, false, true);

    log_this("WebServer", "Web server started successfully", 0, true, false, true);

    return NULL;
}

void shutdown_web_server(void) {
    log_this("WebServer", "Shutdown: Shutting down web server", 0, true, false, true);
    if (web_daemon != NULL) {
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        log_this("WebServer", "Web server shut down successfully", 0, true, false, true);
    } else {
        log_this("WebServer", "Web server was not running", 1, true, false, true);
    }
}

const char* get_upload_path(void) {
    return server_web_config->upload_path;
}