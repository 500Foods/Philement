/*
 * Launch WebServer Helper Functions
 * 
 * Helper functions extracted from launch_webserver.c to improve testability
 * and reduce code duplication in the server initialization polling logic.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch_webserver_helpers.h"
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

// External declarations
extern struct MHD_Daemon *webserver_daemon;
extern AppConfig *app_config;

/*
 * Check if web server daemon is ready and log its status
 * 
 * This function encapsulates the repeated logic of checking if the webserver
 * daemon is bound to a port and logging detailed status information.
 * 
 * @return true if server is ready (daemon bound to port), false otherwise
 */
bool check_webserver_daemon_ready(void) {
    if (webserver_daemon == NULL) {
        return false;
    }
    
    const union MHD_DaemonInfo *info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_BIND_PORT);
    if (info == NULL || info->port == 0) {
        return false;
    }
    
    // Server is bound to a port - get additional info for logging
    const union MHD_DaemonInfo *conn_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
    unsigned int num_connections = conn_info ? conn_info->num_connections : 0;
    
    const union MHD_DaemonInfo *thread_info = MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_FLAGS);
    bool using_threads = thread_info && (thread_info->flags & MHD_USE_THREAD_PER_CONNECTION);
    
    log_this(SR_WEBSERVER, "Server status:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_WEBSERVER, "― Bound to port: %u", LOG_LEVEL_DEBUG, 1, info->port);
    log_this(SR_WEBSERVER, "― Active connections: %u", LOG_LEVEL_DEBUG, 1, num_connections);
    log_this(SR_WEBSERVER, "― Thread mode: %s", LOG_LEVEL_DEBUG, 1, using_threads ? "Thread per connection" : "Single thread");
    log_this(SR_WEBSERVER, "― IPv6: %s", LOG_LEVEL_DEBUG, 1, app_config->webserver.enable_ipv6 ? "enabled" : "disabled");
    log_this(SR_WEBSERVER, "― Max connections: %d", LOG_LEVEL_DEBUG, 1, app_config->webserver.max_connections);
    
    // Log network interfaces
    log_this(SR_WEBSERVER, "― Network interfaces:", LOG_LEVEL_DEBUG, 0);
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
                    log_this(SR_WEBSERVER, "――― %s: %s (%s)", LOG_LEVEL_DEBUG, 3,
                            ifa->ifa_name, 
                            host,
                            (family == AF_INET) ? "IPv4" : "IPv6");
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    
    return true;
}