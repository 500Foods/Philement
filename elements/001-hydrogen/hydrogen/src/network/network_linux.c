/*
 * Network Management
 * 
 */

// Global includes
#include "../hydrogen.h"
#include <time.h>

// Local includes
#include "network.h"

// Network headers
#include <net/if.h>          // Must be first for interface definitions
#include <linux/if.h>        // Additional interface definitions
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/select.h>

// Check if an interface is configured in the Available section
bool is_interface_configured(const char* interface_name, bool* is_available) {
    if (!app_config || !interface_name || !is_available) {
        if (is_available) *is_available = true;  // Default to available if no config
        return false;          // Not explicitly configured
    }

    // Check if the available_interfaces array is NULL
    if (!app_config->network.available_interfaces || app_config->network.available_interfaces_count == 0) {
        *is_available = true;  // Default to available if no available_interfaces section exists
        return false;          // Not explicitly configured
    }

    // Check if the interface is in the available_interfaces array
    for (size_t i = 0; i < app_config->network.available_interfaces_count; i++) {
        if (app_config->network.available_interfaces[i].interface_name) {
            if (strcmp(interface_name, app_config->network.available_interfaces[i].interface_name) == 0) {
                // Interface is explicitly configured
                *is_available = app_config->network.available_interfaces[i].available;
                return true;  // Explicitly configured
            }
        }
    }

    // Interface is not in the configuration - if Network.Available section exists,
    // interfaces not listed should be ENABLED by default (unless explicitly disabled)
    *is_available = true;  // Default to available for unlisted interfaces
    return false;           // Not explicitly configured
}

// Test a single interface with UDP socket
static bool test_interface(bool is_ipv6, struct ifreq *ifr, int *mtu) {
    int sockfd;
    bool success = false;
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

    // Create UDP socket (doesn't require root)
    if (is_ipv6) {
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    } else {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (sockfd < 0) {
        return false;
    }

    // Set timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Get interface flags
    if (ioctl(sockfd, SIOCGIFFLAGS, ifr) >= 0) {
        // Check if interface is up and running
        if ((ifr->ifr_flags & IFF_UP) && (ifr->ifr_flags & IFF_RUNNING)) {
            success = true;
        }
    }

    // Get interface MTU
    if (ioctl(sockfd, SIOCGIFMTU, ifr) >= 0) {
        *mtu = ifr->ifr_mtu;
    }

    close(sockfd);
    return success;
}

// Test all network interfaces
bool test_network_interfaces(network_info_t *info) {
    if (!info) return false;

    bool success = false;
    struct ifreq ifr;

    for (int i = 0; i < info->count; i++) {
        interface_t *iface = &info->interfaces[i];

        // Test all interfaces including loopback

        // Check if interface is enabled in config
        bool is_available = true;
        bool is_configured = is_interface_configured(iface->name, &is_available);

        // Skip if explicitly disabled in config
        if (is_configured && !is_available) {
            log_this(SR_NETWORK, "― Interface %s: skipped (disabled in config)", LOG_LEVEL_STATE, 1, iface->name);
            continue;
        }

        // Prepare interface request structure
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, iface->name, IFNAMSIZ - 1);

        for (int j = 0; j < iface->ip_count; j++) {
            // Determine if IPv6
            iface->is_ipv6[j] = (strchr(iface->ips[j], ':') != NULL);

            // Test interface
            int mtu = 0;
            bool interface_up = test_interface(iface->is_ipv6[j], &ifr, &mtu);

            if (interface_up) {
                success = true;
                log_this(SR_NETWORK, "― Interface %s (%s): up, MTU %d", LOG_LEVEL_DEBUG, 3,
                        iface->name,
                        iface->is_ipv6[j] ? "IPv6" : "IPv4",
                        mtu);
            } else {
                log_this(SR_NETWORK, "― Interface %s (%s): down", LOG_LEVEL_DEBUG, 2,
                        iface->name,
                        iface->is_ipv6[j] ? "IPv6" : "IPv4");
            }
        }
    }

    return success;
}

// Test all network interfaces with optional quiet mode
bool test_network_interfaces_quiet(network_info_t *info, bool quiet) {
    if (!info) return false;

    bool success = false;
    struct ifreq ifr;
    
    for (int i = 0; i < info->count; i++) {
        interface_t *iface = &info->interfaces[i];
        
        // Test all interfaces including loopback

        // Check if interface is enabled in config
        bool is_available = true;
        bool is_configured = is_interface_configured(iface->name, &is_available);
        
        // Skip if explicitly disabled in config
        if (is_configured && !is_available) {
            log_this(SR_NETWORK, "― Interface %s: skipped (disabled in config)", LOG_LEVEL_STATE, 1, iface->name);
            continue;
        }

        // Prepare interface request structure
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, iface->name, IFNAMSIZ - 1);

        for (int j = 0; j < iface->ip_count; j++) {
            // Determine if IPv6
            iface->is_ipv6[j] = (strchr(iface->ips[j], ':') != NULL);
            
            // Test interface
            int mtu = 0;
            bool interface_up = test_interface(iface->is_ipv6[j], &ifr, &mtu);
            
            if (interface_up) {
                success = true;
                if (!quiet) {
                    log_this(SR_NETWORK, "― Interface %s (%s): up, MTU %d", LOG_LEVEL_DEBUG, 3,
                            iface->name,
                            iface->is_ipv6[j] ? "IPv6" : "IPv4",
                            mtu);
                }
            } else {
                if (!quiet) {
                    log_this(SR_NETWORK, "― Interface %s (%s): down", LOG_LEVEL_DEBUG, 2,
                            iface->name,
                            iface->is_ipv6[j] ? "IPv6" : "IPv4");
                }
            }
        }
    }

    return success;
}

// Discover and analyze network interfaces with comprehensive enumeration

network_info_t *get_network_info(void) {
    network_info_t *info = malloc(sizeof(network_info_t));
    if (!info) {
        perror("malloc");
        return NULL;
    }

    info->primary_index = -1;
    info->count = 0;

    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        log_this(SR_NETWORK, "getifaddrs failed: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        free(info);
        return NULL;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6)) {
            int new_interface = 1;
            for (int i = 0; i < info->count; i++) {
                if (strcmp(info->interfaces[i].name, ifa->ifa_name) == 0) {
                    new_interface = 0;
                    break;
                }
            }

            if (new_interface && info->count < MAX_INTERFACES) {
                strncpy(info->interfaces[info->count].name, ifa->ifa_name, IF_NAMESIZE);
                info->interfaces[info->count].name[IF_NAMESIZE - 1] = '\0';
                info->interfaces[info->count].ip_count = 0;
                info->count++;
            }

            int interface_index = -1;
            for (int i = 0; i < info->count; i++) {
                if (strcmp(info->interfaces[i].name, ifa->ifa_name) == 0) {
                    interface_index = i;
                    break;
                }
            }

            if (interface_index != -1 && info->interfaces[interface_index].ip_count < MAX_IPS) {
                char ip[INET6_ADDRSTRLEN];
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                    if (inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip))) {
                        strncpy(info->interfaces[interface_index].ips[info->interfaces[interface_index].ip_count], ip, INET6_ADDRSTRLEN);
                        info->interfaces[interface_index].ip_count++;
                    }
                } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                    struct sockaddr_in6 *sa = (struct sockaddr_in6 *)ifa->ifa_addr;
                    if (!IN6_IS_ADDR_LOOPBACK(&sa->sin6_addr) && inet_ntop(AF_INET6, &sa->sin6_addr, ip, sizeof(ip))) {
                        strncpy(info->interfaces[interface_index].ips[info->interfaces[interface_index].ip_count], ip, INET6_ADDRSTRLEN);
                        info->interfaces[interface_index].ip_count++;
                    }
                }
            }

            if (info->primary_index == -1 && strcmp(ifa->ifa_name, "lo") != 0) {
                info->primary_index = interface_index;
            }
        }
    }

    freeifaddrs(ifaddr);
    return info;
}

void free_network_info(network_info_t *info) {
    if (info) {
        free(info);
    }
}

// Find available network port with collision avoidance

int find_available_port(int start_port) {
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        log_this(SR_NETWORK, "Failed to create socket: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    for (int port = start_port; port < 65536; port++) {
        addr.sin_port = htons((uint16_t)port);
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            close(sock);
            return port;
        }
    }

    close(sock);
    log_this(SR_NETWORK, "No available ports found", LOG_LEVEL_DEBUG, 0);
    return -1;
}

// Filter interfaces based on Network.Available configuration
// Returns a new network_info_t with only enabled interfaces
network_info_t *filter_enabled_interfaces(const network_info_t *raw_net_info, const struct AppConfig *config) {
    if (!raw_net_info) {
        return NULL;
    }

    network_info_t *filtered_info = malloc(sizeof(network_info_t));
    if (!filtered_info) {
        log_this(SR_NETWORK, "Failed to allocate filtered network info", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    filtered_info->primary_index = -1;
    filtered_info->count = 0;

    // Iterate through all interfaces and check if they are enabled
    for (int i = 0; i < raw_net_info->count && filtered_info->count < MAX_INTERFACES; i++) {
        const interface_t *iface = &raw_net_info->interfaces[i];

        // Test all interfaces including loopback

        // Check if interface is enabled in config
        bool is_available = true;
        bool is_configured = false;

        if (config) {
            is_configured = is_interface_configured(iface->name, &is_available);
        }

        // Skip if explicitly disabled in config
        if (is_configured && !is_available) {
            log_this(SR_NETWORK, "Interface %s: filtered out (disabled in config)", LOG_LEVEL_DEBUG, 1, iface->name);
            continue;
        }

        // Interface is enabled, add it to filtered list
        interface_t *filtered_iface = &filtered_info->interfaces[filtered_info->count];

        // Copy interface name
        strncpy(filtered_iface->name, iface->name, IF_NAMESIZE);
        filtered_iface->name[IF_NAMESIZE - 1] = '\0';

        // Copy IP addresses
        filtered_iface->ip_count = iface->ip_count;
        for (int j = 0; j < iface->ip_count && j < MAX_IPS; j++) {
            strncpy(filtered_iface->ips[j], iface->ips[j], INET6_ADDRSTRLEN);
            filtered_iface->ips[j][INET6_ADDRSTRLEN - 1] = '\0';
            filtered_iface->ping_ms[j] = iface->ping_ms[j];
            filtered_iface->is_ipv6[j] = iface->is_ipv6[j];
        }

        // Copy MAC address if available
        if (iface->mac[0]) {
            strncpy(filtered_iface->mac, iface->mac, sizeof(filtered_iface->mac));
            filtered_iface->mac[sizeof(filtered_iface->mac) - 1] = '\0';
        }

        log_this(SR_NETWORK, "Interface %s: enabled and included", LOG_LEVEL_DEBUG, 1, iface->name);

        // Set primary index if not set yet
        if (filtered_info->primary_index == -1) {
            filtered_info->primary_index = filtered_info->count;
        }

        filtered_info->count++;
    }

    log_this(SR_NETWORK, "Filtered %d interfaces, %d remaining", LOG_LEVEL_DEBUG, 2, raw_net_info->count, filtered_info->count);

    return filtered_info;
}

// Gracefully shut down all network interfaces

bool network_shutdown(void) {
    log_this(SR_NETWORK, "Starting network shutdown...", LOG_LEVEL_DEBUG, 0);

    // Get current network interfaces for status reporting
    network_info_t *info = get_network_info();
    if (!info) {
        log_this(SR_NETWORK, "Failed to get network info for shutdown", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Log interface status during shutdown
    for (int i = 0; i < info->count; i++) {
        // Test all interfaces including loopback

        log_this(SR_NETWORK, "Interface %s: cleaning up application resources",LOG_LEVEL_DEBUG, 1, info->interfaces[i].name);
    }

    // Clean up network info
    free_network_info(info);

    // Report successful shutdown - we don't modify system interfaces
    log_this(SR_NETWORK, "Network subsystem shutdown completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

// Forward declarations for static functions

// Find interface name for given IP (returns NULL if not found)
static char *find_iface_for_ip(const char *ip_str) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) return NULL;
    char *iface = NULL;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || !(ifa->ifa_flags & IFF_UP)) continue;
        char addr[INET6_ADDRSTRLEN];
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &s->sin_addr, addr, sizeof(addr));
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)ifa->ifa_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, addr, sizeof(addr));
        } else continue;
        if (strcmp(addr, ip_str) == 0) {
            iface = strdup(ifa->ifa_name);
            break;
        }
    }
    freeifaddrs(ifaddr);
    return iface;
}

// Measure ping time for IP: local (binds to iface, 10ms timeout) or remote (default source, 500ms timeout)
double interface_time(const char *ip_str) {
    // Auto-detect iface for link-local scoping
    const char *iface = NULL;
    char *iface_copy = NULL;  // For freeing the strdup result
    char dest_str[INET6_ADDRSTRLEN + 10];
    strncpy(dest_str, ip_str, sizeof(dest_str) - 1);
    dest_str[sizeof(dest_str) - 1] = '\0';

    struct addrinfo hints = {0}, *res = NULL, *local = NULL;

    bool is_linklocal = (strstr(ip_str, "fe80::") != NULL);
    if (is_linklocal) {
        iface_copy = find_iface_for_ip(ip_str);
        if (iface_copy) {
            iface = iface_copy;
            size_t len = strlen(ip_str);
            snprintf(dest_str + len, sizeof(dest_str) - len, "%%%s", iface);
        } else {
            return 0.000000;  // Unreachable link-local (setup fail)
        }
    }

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags |= AI_NUMERICHOST;

    // Resolve destination (scoped if link-local)
    const char *target_addr = dest_str;
    if (getaddrinfo(target_addr, "65000", &hints, &res) != 0) {
        if (is_linklocal) return 0.000000;
        if (getaddrinfo(ip_str, "65000", &hints, &res) != 0) return 0.000000;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        if (iface_copy) free(iface_copy);
        return 0.000000;
    }

    // Detect if local: try to find/bind source addr
    char *temp_iface = find_iface_for_ip(ip_str);
    bool is_local = (temp_iface != NULL);
    if (temp_iface) free(temp_iface);
    if (is_local) {
        const char *local_addr = is_linklocal ? dest_str : ip_str;
        if (getaddrinfo(local_addr, NULL, &hints, &local) == 0) {
            if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
                // Bind fail even for local? Skip and treat as remote
                is_local = false;
                freeaddrinfo(local); local = NULL;
            }
        } else {
            is_local = false;  // Fallback
        }
    }

    // Non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int conn = connect(sock, res->ai_addr, res->ai_addrlen);

    if (conn == 0) {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        double sec_diff = (double)(end.tv_sec - start.tv_sec);
        double nsec_diff = (double)(end.tv_nsec - start.tv_nsec);
        if (nsec_diff < 0) { nsec_diff += 1e9; sec_diff -= 1.0; }
        close(sock);
        if (local) freeaddrinfo(local);
        freeaddrinfo(res);
        if (iface_copy) free(iface_copy);
        return (sec_diff + (nsec_diff / 1e9)) * 1000.0;  // ms
    }

    if (errno == EINPROGRESS) {
        fd_set wfds;
        FD_ZERO(&wfds); FD_SET(sock, &wfds);
        int timeout_us = is_local ? 10000 : 500000;  // 10ms local, 500ms remote
        struct timeval tv = {0, timeout_us};
        int sel = select(sock + 1, NULL, &wfds, NULL, &tv);

        if (sel > 0) {
            // Connection completed
            int so_error;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);

            struct timespec end;
            clock_gettime(CLOCK_MONOTONIC, &end);
            double sec_diff = (double)(end.tv_sec - start.tv_sec);
            double nsec_diff = (double)(end.tv_nsec - start.tv_nsec);
            if (nsec_diff < 0) { nsec_diff += 1e9; sec_diff -= 1.0; }

            close(sock);
            if (local) freeaddrinfo(local);
            freeaddrinfo(res);
            if (iface_copy) free(iface_copy);

            if (so_error == 0 || so_error == ECONNREFUSED || (!is_local && so_error == ETIMEDOUT)) {
                return (sec_diff + (nsec_diff / 1e9)) * 1000.0;  // ms (ETIMEDOUT ~500ms for unreachable remotes)
            }
        } else {
            // Timeout: return the timeout value for clarity
            close(sock);
            if (local) freeaddrinfo(local);
            freeaddrinfo(res);
            if (iface_copy) free(iface_copy);
            return is_local ? 10.000000 : 500.000000;
        }
    }

    close(sock);
    if (local) freeaddrinfo(local);
    freeaddrinfo(res);
    if (iface_copy) free(iface_copy);
    return 0.000000;  // Other errors (e.g., immediate fail)
}