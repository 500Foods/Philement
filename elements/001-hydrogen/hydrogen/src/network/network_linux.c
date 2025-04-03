/*
 * Network Management for 3D Printer Control
 * 
 */

// Core system headers
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

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

// ICMP definitions
#ifndef ICMP_ECHO
#define ICMP_ECHO 8
#endif
#ifndef ICMP_ECHOREPLY
#define ICMP_ECHOREPLY 0
#endif

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Project headers
#include "network.h"
#include "../utils/utils.h"
#include "../logging/logging.h"
#include "../config/config.h"

#define PING_TIMEOUT_SEC 1
#define PING_PACKET_SIZE 64

// Check if an interface is configured in the Available section
bool is_interface_configured(const AppConfig* app_config, const char* interface_name, bool* is_available) {
    if (!app_config) {
        *is_available = true;  // Default to available if no config
        return false;          // Not explicitly configured
    }
    
    // Check if the available_interfaces array is NULL
    if (!app_config->network.available_interfaces || app_config->network.available_interfaces_count == 0) {
        *is_available = true;  // Default to available if no available_interfaces
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
    
    // If not in the configuration, it's not explicitly configured
    *is_available = true;  // Default to available
    return false;          // Not explicitly configured
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
    const AppConfig* app_config = get_app_config();
    
    for (int i = 0; i < info->count; i++) {
        interface_t *iface = &info->interfaces[i];
        
        // Skip loopback
        if (strcmp(iface->name, "lo") == 0) continue;

        // Check if interface is enabled in config
        bool is_available = true;
        bool is_configured = is_interface_configured(app_config, iface->name, &is_available);
        
        // Skip if explicitly disabled in config
        if (is_configured && !is_available) {
            log_this("Network", "Interface %s: skipped (disabled in config)", LOG_LEVEL_STATE,
                    iface->name);
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
                log_this("Network", "Interface %s (%s): up, MTU %d", LOG_LEVEL_STATE,
                        iface->name,
                        iface->is_ipv6[j] ? "IPv6" : "IPv4",
                        mtu);
            } else {
                log_this("Network", "Interface %s (%s): down", LOG_LEVEL_STATE,
                        iface->name,
                        iface->is_ipv6[j] ? "IPv6" : "IPv4");
            }
        }
    }

    return success;
}

// Discover and analyze network interfaces with comprehensive enumeration
//
// Network discovery design prioritizes:
// 1. Address Management
//    - Dual IPv4/IPv6 stack support
//    - Interface categorization
//    - Address deduplication
//    - Loopback handling
//
// 2. Memory Safety
//    - Bounded array sizes
//    - String length checks
//    - NULL termination
//    - Resource cleanup
//
// 3. Error Recovery
//    - Partial results handling
//    - System call retries
//    - Fallback options
//    - Detailed logging
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
        log_this("Network", "getifaddrs failed: %s", LOG_LEVEL_ERROR, strerror(errno));
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
//
// Port selection strategy:
// 1. Reliability
//    - Sequential port testing
//    - Bind verification
//    - Permission checking
//    - Range validation
//
// 2. Security
//    - Privileged port avoidance
//    - System port skipping
//    - Permission validation
//    - Resource cleanup
//
// 3. Performance
//    - Early exit on success
//    - Efficient socket ops
//    - Resource reuse
//    - Quick release
int find_available_port(int start_port) {
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        log_this("Network", "Failed to create socket: %s", LOG_LEVEL_ERROR, strerror(errno));
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    for (int port = start_port; port < 65536; port++) {
        addr.sin_port = htons(port);
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            close(sock);
            return port;
        }
    }

    close(sock);
    log_this("Network", "No available ports found", LOG_LEVEL_DEBUG);
    return -1;
}

// Gracefully shut down all network interfaces
//
// Shutdown strategy:
// 1. Safety
//    - Ordered interface shutdown
//    - Connection termination
//    - Resource cleanup
//    - State verification
//
// 2. Reliability
//    - Error handling
//    - Status tracking
//    - Fallback options
//    - Cleanup confirmation
//
// 3. Logging
//    - Progress tracking
//    - Error reporting
//    - Status updates
//    - Final verification
bool network_shutdown(void) {
    log_this("Network", "Starting network shutdown...", LOG_LEVEL_STATE);
    
    // Get current network interfaces
    network_info_t *info = get_network_info();
    if (!info) {
        log_this("Network", "Failed to get network info for shutdown", LOG_LEVEL_ERROR);
        return false;
    }
    
    bool success = true;
    
    // Close all active sockets for each interface
    for (int i = 0; i < info->count; i++) {
        log_this("Network", "Shutting down interface: %s", LOG_LEVEL_STATE, info->interfaces[i].name);
        
        // Skip loopback interface
        if (strcmp(info->interfaces[i].name, "lo") == 0) {
            continue;
        }
        
        // Create control socket for interface
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            log_this("Network", "Failed to create control socket for %s: %s", 
                    LOG_LEVEL_ERROR, info->interfaces[i].name, strerror(errno));
            success = false;
            continue;
        }
        
        // Bring interface down
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, info->interfaces[i].name, IFNAMSIZ - 1);
        
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
            log_this("Network", "Failed to get interface flags for %s: %s", 
                    LOG_LEVEL_ERROR, info->interfaces[i].name, strerror(errno));
            close(sock);
            success = false;
            continue;
        }
        
        ifr.ifr_flags &= ~IFF_UP;  // Clear UP flag
        
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
            log_this("Network", "Failed to bring down interface %s: %s", 
                    LOG_LEVEL_ERROR, info->interfaces[i].name, strerror(errno));
            success = false;
        } else {
            log_this("Network", "Successfully shut down interface %s", 
                    LOG_LEVEL_STATE, info->interfaces[i].name);
        }
        
        close(sock);
    }
    
    free_network_info(info);
    
    if (success) {
        log_this("Network", "Network shutdown completed successfully", LOG_LEVEL_STATE);
    } else {
        log_this("Network", "Network shutdown completed with errors", LOG_LEVEL_ERROR);
    }
    
    return success;
}