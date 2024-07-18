#include "network.h"
#include "utils.h"
#include "logging.h"
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

static int count_interfaces() __attribute__((unused));
static int count_interfaces() {
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        log_this("Network", "getifaddrs failed: %s", 3, true, true, true, strerror(errno));
        return 0;
    }

    int count = 0;
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET) {
            count++;
        }
    }

    freeifaddrs(ifaddr);
    return count;
}

network_info_t *get_network_info() {
    network_info_t *info = malloc(sizeof(network_info_t));
    if (!info) {
        perror("malloc");
        return NULL;
    }

    info->primary_index = -1;
    info->count = 0;

    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        log_this("Network", "getifaddrs failed: %s", 3, true, true, true, strerror(errno));
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
                strncpy(info->interfaces[info->count].name, ifa->ifa_name, IFNAMSIZ);
                info->interfaces[info->count].name[IFNAMSIZ - 1] = '\0';
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

int find_available_port(int start_port) {
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        log_this("Network", "Failed to create socket: %s", 0, true, true, true, strerror(errno));
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
    log_this("Network", "No available ports found", 3, true, true, true);
    return -1;
}

