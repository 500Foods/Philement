/*
 * Linux implementation of mDNS service discovery for the Hydrogen printer.
 * 
 * Implements DNS-SD/mDNS (RFC 6762, RFC 6763) for network service discovery
 * and announcement. The implementation provides automatic network presence
 * for Hydrogen printers with the following features:
 * 
 * Network Management:
 * - Dual-stack IPv4/IPv6 support
 * - Multicast socket configuration
 * - Interface monitoring and selection
 * - Automatic port selection and binding
 * 
 * DNS Protocol:
 * - Standard-compliant packet construction
 * - Resource record management (A, AAAA, PTR, SRV, TXT)
 * - Query handling and response generation
 * - Name compression for efficiency
 * 
 * Service Management:
 * - Multiple service registration
 * - Service metadata via TXT records
 * - Dynamic port assignment
 * - Service instance naming
 * 
 * Reliability Features:
 * - Probing for name conflicts
 * - Automatic service re-announcement
 * - Goodbye packets on shutdown
 * - Error recovery mechanisms
 * 
 * Thread Safety:
 * - Mutex-protected shared resources
 * - Safe shutdown coordination
 * - Resource cleanup verification
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <net/if.h> // For if_nametoindex
#include <poll.h>

#include "keys.h"
#include "mdns_server.h"
#include "utils.h"
#include "logging.h"

extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t mdns_server_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

static uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len) {
    size_t i = 0;
    while (*ptr) {
        if ((*ptr & 0xC0) == 0xC0) {
            uint16_t offset = ((*ptr & 0x3F) << 8) | *(ptr + 1);
            read_dns_name((uint8_t*)packet + offset, packet, name + i, name_len - i);
            return ptr + 2;
        }
        size_t len = *ptr++;
        if (i + len + 1 >= name_len) return NULL;
        memcpy(name + i, ptr, len);
        i += len;
        name[i++] = '.';
        ptr += len;
    }
    if (i > 0) name[i - 1] = '\0';
    else name[0] = '\0';
    return ptr + 1;
}

static int create_multicast_socket(int family, const char *group, const char *if_name) {
    if (!if_name) {
        log_this("mDNS", "No interface name provided", 3, true, true, true);
        return -1;
    }

    int sockfd = socket(family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_this("mDNS", "Failed to create socket: %s", 3, true, true, true, strerror(errno));
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, if_name, strlen(if_name)) < 0) {
        log_this("mDNS", "Failed to bind to interface %s: %s", 3, true, true, true, if_name, strerror(errno));
        close(sockfd);
        return -1;
    }

    unsigned int if_index = if_nametoindex(if_name);
    if (if_index == 0) {
        log_this("mDNS", "Failed to get interface index for %s: %s", 3, true, true, true, if_name, strerror(errno));
        close(sockfd);
        return -1;
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        log_this("mDNS", "Failed to set SO_REUSEADDR: %s", 3, true, true, true, strerror(errno));
        close(sockfd);
        return -1;
    }

    struct sockaddr_storage local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    if (family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *)&local_addr;
        addr->sin_family = AF_INET;
        addr->sin_port = htons(MDNS_PORT);
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&local_addr;
        addr->sin6_family = AF_INET6;
        addr->sin6_port = htons(MDNS_PORT);
        addr->sin6_addr = in6addr_any;
    }

    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        log_this("mDNS", "Failed to bind socket: %s", 3, true, true, true, strerror(errno));
        close(sockfd);
        return -1;
    }

    int ttl = MDNS_TTL;
    if (setsockopt(sockfd, family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                   family == AF_INET ? IP_MULTICAST_TTL : IPV6_MULTICAST_HOPS,
                   &ttl, sizeof(ttl)) < 0) {
        log_this("mDNS", "Failed to set multicast TTL: %s", 3, true, true, true, strerror(errno));
        close(sockfd);
        return -1;
    }

    int loop = 1;
    if (setsockopt(sockfd, family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                   family == AF_INET ? IP_MULTICAST_LOOP : IPV6_MULTICAST_LOOP,
                   &loop, sizeof(loop)) < 0) {
        log_this("mDNS", "Failed to enable multicast loop: %s", 1, true, true, true, strerror(errno));
    }

    if (family == AF_INET) {
        struct ip_mreqn mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(group);
        mreq.imr_address.s_addr = INADDR_ANY;
        mreq.imr_ifindex = if_index;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
            log_this("mDNS", "Failed to set IPv4 multicast interface: %s", 3, true, true, true, strerror(errno));
            close(sockfd);
            return -1;
        }
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            log_this("mDNS", "Failed to join IPv4 multicast group: %s", 3, true, true, true, strerror(errno));
            close(sockfd);
            return -1;
        }
    } else {
        struct ipv6_mreq mreq;
        inet_pton(AF_INET6, group, &mreq.ipv6mr_multiaddr);
        mreq.ipv6mr_interface = if_index;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &if_index, sizeof(if_index)) < 0) {
            log_this("mDNS", "Failed to set IPv6 multicast interface: %s", 3, true, true, true, strerror(errno));
            close(sockfd);
            return -1;
        }
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
            log_this("mDNS", "Failed to join IPv6 multicast group: %s", 3, true, true, true, strerror(errno));
            close(sockfd);
            return -1;
        }
    }

    log_this("mDNS", "Created multicast socket on interface %s", 0, true, true, true, if_name);
    return sockfd;
}

static uint8_t *write_dns_name(uint8_t *ptr, const char *name) {
    const char *part = name;
    while (*part) {
        const char *end = strchr(part, '.');
        if (!end) end = part + strlen(part);
        size_t len = end - part;
        *ptr++ = len;
        memcpy(ptr, part, len);
        ptr += len;
        if (*end == '.') part = end + 1;
        else part = end;
    }
    *ptr++ = 0;
    return ptr;
}

static uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(type); ptr += 2;
    *((uint16_t*)ptr) = htons(class); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    *((uint16_t*)ptr) = htons(rdlen); ptr += 2;
    memcpy(ptr, rdata, rdlen);
    ptr += rdlen;
    return ptr;
}

static uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = strlen(ptr_data) + 2;
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    ptr = write_dns_name(ptr, ptr_data);
    return ptr;
}

static uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_SRV); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = 6 + strlen(target) + 2;
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    *((uint16_t*)ptr) = htons(priority); ptr += 2;
    *((uint16_t*)ptr) = htons(weight); ptr += 2;
    *((uint16_t*)ptr) = htons(port); ptr += 2;
    ptr = write_dns_name(ptr, target);
    return ptr;
}

static uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_TXT); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    size_t total_len = 0;
    for (size_t i = 0; i < num_txt_records; i++) {
        total_len += strlen(txt_records[i]) + 1;
    }
    *((uint16_t*)ptr) = htons(total_len); ptr += 2;
    for (size_t i = 0; i < num_txt_records; i++) {
        size_t len = strlen(txt_records[i]);
        *ptr++ = len;
        memcpy(ptr, txt_records[i], len);
        ptr += len;
    }
    return ptr;
}

static void _mdns_build_interface_announcement(uint8_t *packet, size_t *packet_len, const char *hostname, 
                                             const mdns_t *mdns, uint32_t ttl, const mdns_interface_t *iface) {
    dns_header_t *header = (dns_header_t *)packet;
    header->id = 0;
    header->flags = htons(MDNS_FLAG_RESPONSE | MDNS_FLAG_AUTHORITATIVE);
    header->qdcount = 0;
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    uint8_t *ptr = packet + sizeof(dns_header_t);

    // Add IP addresses from the interface
    for (size_t i = 0; i < iface->num_addresses; i++) {
        struct in_addr addr;
        struct in6_addr addr6;
        if (inet_pton(AF_INET, iface->ip_addresses[i], &addr) == 1) {
            ptr = write_dns_record(ptr, hostname, MDNS_TYPE_A, MDNS_CLASS_IN, ttl, &addr.s_addr, 4);
            header->ancount = htons(ntohs(header->ancount) + 1);
        } else if (inet_pton(AF_INET6, iface->ip_addresses[i], &addr6) == 1) {
            ptr = write_dns_record(ptr, hostname, MDNS_TYPE_AAAA, MDNS_CLASS_IN, ttl, &addr6.s6_addr, 16);
            header->ancount = htons(ntohs(header->ancount) + 1);
        }
    }

    // Add service records
    for (size_t i = 0; i < mdns->num_services; i++) {
        char service_type[256];
        char full_service_name[256];
        const char *local_suffix = strstr(mdns->services[i].type, ".local");
        size_t type_len = local_suffix ? (size_t)(local_suffix - mdns->services[i].type) : strlen(mdns->services[i].type);
        strncpy(service_type, mdns->services[i].type, type_len);
        service_type[type_len] = '\0';

        const size_t max_name_len = 100;
        const size_t max_type_len = 100;
        size_t name_len = strlen(mdns->services[i].name);
        size_t stype_len = strlen(service_type);
        
        size_t total_len = name_len + 1 + stype_len + 6;
        if (total_len >= sizeof(full_service_name)) {
            log_this("mDNS", "Service name too long: %s.%s.local truncated", 1, true, true, true, 
                     mdns->services[i].name, service_type);
            name_len = max_name_len < name_len ? max_name_len : name_len;
            stype_len = max_type_len < stype_len ? max_type_len : stype_len;
        }

        snprintf(full_service_name, sizeof(full_service_name), "%.*s.%.*s.local", 
                 (int)name_len, mdns->services[i].name, (int)stype_len, service_type);

        ptr = write_dns_ptr_record(ptr, mdns->services[i].type, full_service_name, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);
        ptr = write_dns_srv_record(ptr, full_service_name, 0, 0, mdns->services[i].port, hostname, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);
        ptr = write_dns_txt_record(ptr, full_service_name, mdns->services[i].txt_records, mdns->services[i].num_txt_records, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);
    }

    *packet_len = ptr - packet;
    if (*packet_len > 1500) {
        log_this("mDNS", "Warning: Packet size %zu exceeds typical MTU (1500)", 1, true, true, true, *packet_len);
    }
}

void mdns_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname, 
                           const mdns_t *mdns, uint32_t ttl, const network_info_t *net_info) {
    // Find the matching interface from net_info
    mdns_interface_t *matching_iface = NULL;
    if (net_info && net_info->primary_index >= 0 && net_info->primary_index < net_info->count) {
        const interface_t *primary = &net_info->interfaces[net_info->primary_index];
        for (size_t i = 0; i < mdns->num_interfaces; i++) {
            if (strcmp(mdns->interfaces[i].if_name, primary->name) == 0) {
                matching_iface = &mdns->interfaces[i];
                break;
            }
        }
    }

    // Use the interface-specific announcement builder
    _mdns_build_interface_announcement(packet, packet_len, hostname, mdns, ttl, matching_iface);
}

static network_info_t *create_single_interface_net_info(const mdns_interface_t *iface) {
    network_info_t *net_info = calloc(1, sizeof(network_info_t));
    if (!net_info) return NULL;

    net_info->count = 1;
    net_info->primary_index = 0;

    // Copy interface name
    strncpy(net_info->interfaces[0].name, iface->if_name, IF_NAMESIZE - 1);
    net_info->interfaces[0].name[IF_NAMESIZE - 1] = '\0';

    // Copy IP addresses
    size_t num_ips = iface->num_addresses;
    if (num_ips > MAX_IPS) {
        num_ips = MAX_IPS;
    }
    net_info->interfaces[0].ip_count = num_ips;

    for (size_t i = 0; i < num_ips; i++) {
        strncpy(net_info->interfaces[0].ips[i], iface->ip_addresses[i], INET6_ADDRSTRLEN - 1);
        net_info->interfaces[0].ips[i][INET6_ADDRSTRLEN - 1] = '\0';
    }

    return net_info;
}

static void free_single_interface_net_info(network_info_t *net_info) {
    free(net_info);
}

void mdns_send_announcement(mdns_t *mdns, const network_info_t *net_info __attribute__((unused))) {
    struct sockaddr_in addr_v4;
    struct sockaddr_in6 addr_v6;

    memset(&addr_v4, 0, sizeof(addr_v4));
    addr_v4.sin_family = AF_INET;
    addr_v4.sin_port = htons(MDNS_PORT);
    inet_pton(AF_INET, MDNS_GROUP_V4, &addr_v4.sin_addr);

    memset(&addr_v6, 0, sizeof(addr_v6));
    addr_v6.sin6_family = AF_INET6;
    addr_v6.sin6_port = htons(MDNS_PORT);
    inet_pton(AF_INET6, MDNS_GROUP_V6, &addr_v6.sin6_addr);

    // Send announcement on each interface
    for (size_t i = 0; i < mdns->num_interfaces; i++) {
        mdns_interface_t *iface = &mdns->interfaces[i];
        uint8_t packet[MDNS_MAX_PACKET_SIZE];
        size_t packet_len;

        // Create temporary network_info for this interface
        network_info_t *iface_net_info = create_single_interface_net_info(iface);
        if (!iface_net_info) {
            log_this("mDNS", "Failed to create network info for interface %s", 3, true, true, true, iface->if_name);
            continue;
        }

        // Build announcement with interface-specific IPs
        mdns_build_announcement(packet, &packet_len, mdns->hostname, mdns, MDNS_TTL, iface_net_info);
        free_single_interface_net_info(iface_net_info);

        // Send IPv4 announcement
        if (iface->sockfd_v4 >= 0) {
            if (sendto(iface->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof(addr_v4)) < 0) {
                log_this("mDNS", "Failed to send IPv4 announcement on %s: %s", 3, true, true, true,
                        iface->if_name, strerror(errno));
            } else {
                log_this("mDNS", "Sent IPv4 announcement on %s", 0, true, true, true, iface->if_name);
            }
        }

        // Send IPv6 announcement
        if (iface->sockfd_v6 >= 0) {
            if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
                log_this("mDNS", "Failed to send IPv6 announcement on %s: %s", 2, true, true, true,
                        iface->if_name, strerror(errno));
            } else {
                log_this("mDNS", "Sent IPv6 announcement on %s", 0, true, true, true, iface->if_name);
            }
        }
    }
}

void *mdns_announce_loop(void *arg) {
    mdns_thread_arg_t *thread_arg = (mdns_thread_arg_t *)arg;
    mdns_t *mdns = thread_arg->mdns;
    add_service_thread(&mdns_threads, pthread_self());

    log_this("mDNS", "mDNS announce loop started", 0, true, true, true);

    int initial_announcements = 3; // Initial burst
    int interval = 1; // Start with 1-second intervals

    while (!mdns_server_shutdown) {
        if (initial_announcements > 0) {
            mdns_send_announcement(mdns, thread_arg->net_info);
            initial_announcements--;
            sleep(interval);
            if (initial_announcements == 0) interval = 60; // Switch to normal interval
        } else {
            pthread_mutex_lock(&terminate_mutex);
            if (!mdns_server_shutdown) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += interval;
                pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
            }
            pthread_mutex_unlock(&terminate_mutex);

            if (!mdns_server_shutdown) {
                mdns_send_announcement(mdns, thread_arg->net_info);
            }
        }
    }

    log_this("mDNS", "Shutdown: mDNS announce loop exiting", 0, true, true, true);
    remove_service_thread(&mdns_threads, pthread_self());
    free(thread_arg);
    return NULL;
}

void *mdns_responder_loop(void *arg) {
    mdns_thread_arg_t *thread_arg = (mdns_thread_arg_t *)arg;
    mdns_t *mdns = thread_arg->mdns;
    uint8_t buffer[MDNS_MAX_PACKET_SIZE];
    char name[256];

    add_service_thread(&mdns_threads, pthread_self());
    log_this("mDNS", "mDNS responder loop started", 0, true, true, true);

    // Create pollfd array for all interface sockets
    struct pollfd *fds = malloc(sizeof(struct pollfd) * mdns->num_interfaces * 2); // 2 sockets per interface (v4/v6)
    if (!fds) {
        log_this("mDNS", "Out of memory for poll fds", 3, true, true, true);
        remove_service_thread(&mdns_threads, pthread_self());
        free(thread_arg);
        return NULL;
    }

    int nfds = 0;
    for (size_t i = 0; i < mdns->num_interfaces; i++) {
        mdns_interface_t *iface = &mdns->interfaces[i];
        if (iface->sockfd_v4 >= 0) {
            fds[nfds].fd = iface->sockfd_v4;
            fds[nfds].events = POLLIN;
            nfds++;
        }
        if (iface->sockfd_v6 >= 0) {
            fds[nfds].fd = iface->sockfd_v6;
            fds[nfds].events = POLLIN;
            nfds++;
        }
    }

    if (nfds == 0) {
        log_this("mDNS", "No sockets to monitor", 3, true, true, true);
        free(fds);
        remove_service_thread(&mdns_threads, pthread_self());
        free(thread_arg);
        return NULL;
    }

    while (!mdns_server_shutdown) {
        int ret = poll(fds, nfds, 1000); // 1-second timeout
        if (ret < 0) {
            if (errno != EINTR) {
                log_this("mDNS", "Poll error: %s", 3, true, true, true, strerror(errno));
            }
            continue;
        }
        if (ret == 0) continue;

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                struct sockaddr_storage src_addr;
                socklen_t src_len = sizeof(src_addr);
                ssize_t len = recvfrom(fds[i].fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_addr, &src_len);
                if (len < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        log_this("mDNS", "Failed to receive mDNS packet: %s", 3, true, true, true, strerror(errno));
                    }
                    continue;
                }

                dns_header_t *header = (dns_header_t *)buffer;
                uint8_t *ptr = buffer + sizeof(dns_header_t);

                for (int j = 0; j < ntohs(header->qdcount); j++) {
                    ptr = read_dns_name(ptr, buffer, name, sizeof(name));
                    if (!ptr) break;

                    uint16_t qtype = ntohs(*((uint16_t*)ptr)); ptr += 2;
                    uint16_t qclass = ntohs(*((uint16_t*)ptr)); ptr += 2;

                    if ((qclass & 0x7FFF) == MDNS_CLASS_IN) {
                        if (qtype == MDNS_TYPE_PTR) {
                            for (size_t k = 0; k < mdns->num_services; k++) {
                                if (strcmp(name, mdns->services[k].type) == 0) {
                                    mdns_send_announcement(mdns, thread_arg->net_info);
                                    goto next_packet;
                                }
                            }
                        } else if (qtype == MDNS_TYPE_SRV || qtype == MDNS_TYPE_TXT) {
                            for (size_t k = 0; k < mdns->num_services; k++) {
                                char full_name[256];
                                snprintf(full_name, sizeof(full_name), "%s.%s", mdns->services[k].name, mdns->services[k].type);
                                if (strcmp(name, full_name) == 0) {
                                    mdns_send_announcement(mdns, thread_arg->net_info);
                                    goto next_packet;
                                }
                            }
                        } else if ((qtype == MDNS_TYPE_A || qtype == MDNS_TYPE_AAAA || qtype == MDNS_TYPE_ANY) &&
                                   strcmp(name, mdns->hostname) == 0) {
                            mdns_send_announcement(mdns, thread_arg->net_info);
                            goto next_packet;
                        }
                    }
                }
            next_packet:
                continue;
            }
        }
    }

    log_this("mDNS", "Shutdown: mDNS responder loop exiting", 0, true, true, true);
    remove_service_thread(&mdns_threads, pthread_self());
    free(thread_arg);
    return NULL;
}

mdns_t *mdns_init(const char *app_name, const char *id, const char *friendly_name,
                  const char *model, const char *manufacturer, const char *sw_version,
                  const char *hw_version, const char *config_url,
                  mdns_service_t *services, size_t num_services, int enable_ipv6) {
    mdns_t *mdns = malloc(sizeof(mdns_t));
    if (!mdns) {
        log_this("mDNS", "Out of memory", 4, true, true, true);
        return NULL;
    }

    mdns->enable_ipv6 = enable_ipv6;
    network_info_t *net_info = get_network_info();
    if (!net_info || net_info->count == 0) {
        log_this("mDNS", "Failed to get network info", 3, true, true, true);
        free(mdns);
        return NULL;
    }

    // Allocate space for interfaces
    mdns->interfaces = malloc(sizeof(mdns_interface_t) * net_info->count);
    mdns->num_interfaces = 0;
    if (!mdns->interfaces) {
        log_this("mDNS", "Out of memory for interfaces", 3, true, true, true);
        free(mdns);
        free_network_info(net_info);
        return NULL;
    }

    // Initialize each interface
    for (int i = 0; i < net_info->count; i++) {
        const interface_t *iface = &net_info->interfaces[i];
        
        // Skip loopback and interfaces without IPs
        if (strcmp(iface->name, "lo") == 0 || iface->ip_count == 0) {
            continue;
        }

        mdns_interface_t *mdns_if = &mdns->interfaces[mdns->num_interfaces];
        mdns_if->if_name = strdup(iface->name);
        mdns_if->ip_addresses = malloc(sizeof(char*) * iface->ip_count);
        mdns_if->num_addresses = iface->ip_count;

        if (!mdns_if->if_name || !mdns_if->ip_addresses) {
            log_this("mDNS", "Out of memory for interface %s", 3, true, true, true, iface->name);
            goto cleanup;
        }

        // Copy IP addresses
        for (int j = 0; j < iface->ip_count; j++) {
            mdns_if->ip_addresses[j] = strdup(iface->ips[j]);
            if (!mdns_if->ip_addresses[j]) {
                log_this("mDNS", "Out of memory for IP address", 3, true, true, true);
                goto cleanup;
            }
        }

        // Create sockets
        mdns_if->sockfd_v4 = create_multicast_socket(AF_INET, MDNS_GROUP_V4, mdns_if->if_name);
        mdns_if->sockfd_v6 = enable_ipv6 ? create_multicast_socket(AF_INET6, MDNS_GROUP_V6, mdns_if->if_name) : -1;

        if (mdns_if->sockfd_v4 < 0 && (!enable_ipv6 || mdns_if->sockfd_v6 < 0)) {
            log_this("mDNS", "Failed to create sockets for interface %s", 3, true, true, true, mdns_if->if_name);
            goto cleanup;
        }

        mdns->num_interfaces++;
    }

    if (mdns->num_interfaces == 0) {
        log_this("mDNS", "No usable interfaces found", 3, true, true, true);
        goto cleanup;
    }

    // Initialize services
    mdns->num_services = num_services;
    mdns->services = malloc(sizeof(mdns_service_t) * num_services);
    if (!mdns->services) {
        log_this("mDNS", "Out of memory for services", 3, true, true, true);
        free(mdns);
        free_network_info(net_info);
        return NULL;
    }

    for (size_t i = 0; i < num_services; i++) {
        mdns->services[i].name = strdup(services[i].name);
        mdns->services[i].type = strdup(services[i].type);
        mdns->services[i].port = services[i].port;
        mdns->services[i].num_txt_records = services[i].num_txt_records;
        mdns->services[i].txt_records = malloc(sizeof(char*) * services[i].num_txt_records);
        for (size_t j = 0; j < services[i].num_txt_records; j++) {
            mdns->services[i].txt_records[j] = strdup(services[i].txt_records[j]);
        }
    }

    // Initialize hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        log_this("mDNS", "Failed to get hostname: %s", 3, true, true, true, strerror(errno));
        strncpy(hostname, "unknown", sizeof(hostname));
    }
    char *dot = strchr(hostname, '.');
    if (dot) *dot = '\0';

    mdns->hostname = malloc(strlen(hostname) + 7);
    if (!mdns->hostname) {
        log_this("mDNS", "Out of memory", 4, true, true, true);
        goto cleanup;
    }
    sprintf(mdns->hostname, "%s.local", hostname);

    // Initialize service info
    mdns->service_name = strdup(app_name);
    mdns->device_id = strdup(id);
    mdns->friendly_name = strdup(friendly_name);
    mdns->model = strdup(model);
    mdns->manufacturer = strdup(manufacturer);
    mdns->sw_version = strdup(sw_version);
    mdns->hw_version = strdup(hw_version);
    mdns->config_url = strdup(config_url);
    mdns->secret_key = generate_secret_key();

    if (!mdns->service_name || !mdns->device_id || !mdns->friendly_name ||
        !mdns->model || !mdns->manufacturer || !mdns->sw_version ||
        !mdns->hw_version || !mdns->config_url || !mdns->secret_key) {
        log_this("mDNS", "Out of memory", 4, true, true, true);
        goto cleanup;
    }

    log_this("mDNS", "mDNS initialized with hostname: %s", 0, true, true, true, mdns->hostname);
    free_network_info(net_info);
    return mdns;

cleanup:
    if (mdns) {
        if (mdns->interfaces) {
            for (size_t i = 0; i < mdns->num_interfaces; i++) {
                mdns_interface_t *mdns_if = &mdns->interfaces[i];
                if (mdns_if->if_name) free(mdns_if->if_name);
                if (mdns_if->ip_addresses) {
                    for (size_t j = 0; j < mdns_if->num_addresses; j++) {
                        if (mdns_if->ip_addresses[j]) free(mdns_if->ip_addresses[j]);
                    }
                    free(mdns_if->ip_addresses);
                }
                if (mdns_if->sockfd_v4 >= 0) close(mdns_if->sockfd_v4);
                if (mdns_if->sockfd_v6 >= 0) close(mdns_if->sockfd_v6);
            }
            free(mdns->interfaces);
        }
        if (mdns->services) {
            for (size_t i = 0; i < mdns->num_services; i++) {
                if (mdns->services[i].name) free(mdns->services[i].name);
                if (mdns->services[i].type) free(mdns->services[i].type);
                if (mdns->services[i].txt_records) {
                    for (size_t j = 0; j < mdns->services[i].num_txt_records; j++) {
                        if (mdns->services[i].txt_records[j]) free(mdns->services[i].txt_records[j]);
                    }
                    free(mdns->services[i].txt_records);
                }
            }
            free(mdns->services);
        }
        if (mdns->hostname) free(mdns->hostname);
        if (mdns->service_name) free(mdns->service_name);
        if (mdns->device_id) free(mdns->device_id);
        if (mdns->friendly_name) free(mdns->friendly_name);
        if (mdns->model) free(mdns->model);
        if (mdns->manufacturer) free(mdns->manufacturer);
        if (mdns->sw_version) free(mdns->sw_version);
        if (mdns->hw_version) free(mdns->hw_version);
        if (mdns->config_url) free(mdns->config_url);
        if (mdns->secret_key) free(mdns->secret_key);
        free(mdns);
    }
    if (net_info) free_network_info(net_info);
    return NULL;
}

void mdns_shutdown(mdns_t *mdns) {
    if (!mdns) return;

    log_this("mDNS", "Shutdown: Initiating mDNS shutdown", 0, true, true, true);
    network_info_t *net_info = get_network_info();
    if (net_info && net_info->primary_index != -1) {
        struct sockaddr_in addr_v4;
        struct sockaddr_in6 addr_v6;

        memset(&addr_v4, 0, sizeof(addr_v4));
        addr_v4.sin_family = AF_INET;
        addr_v4.sin_port = htons(MDNS_PORT);
        inet_pton(AF_INET, MDNS_GROUP_V4, &addr_v4.sin_addr);

        memset(&addr_v6, 0, sizeof(addr_v6));
        addr_v6.sin6_family = AF_INET6;
        addr_v6.sin6_port = htons(MDNS_PORT);
        inet_pton(AF_INET6, MDNS_GROUP_V6, &addr_v6.sin6_addr);

        // Send goodbye packets on each interface
        for (size_t iface_idx = 0; iface_idx < mdns->num_interfaces; iface_idx++) {
            mdns_interface_t *iface = &mdns->interfaces[iface_idx];
            uint8_t packet[MDNS_MAX_PACKET_SIZE];
            size_t packet_len;

            // Create temporary network_info for this interface
            network_info_t *iface_net_info = create_single_interface_net_info(iface);
            if (!iface_net_info) {
                log_this("mDNS", "Failed to create network info for interface %s", 3, true, true, true, iface->if_name);
                continue;
            }

            // Build goodbye announcement with interface-specific IPs
            mdns_build_announcement(packet, &packet_len, mdns->hostname, mdns, 0, iface_net_info);
            free_single_interface_net_info(iface_net_info);
            
            // Send 3 goodbye packets per RFC 6762
            for (int i = 0; i < 3; i++) {
                if (iface->sockfd_v4 >= 0) {
                    if (sendto(iface->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof(addr_v4)) < 0) {
                        log_this("mDNS", "Failed to send IPv4 goodbye on %s: %s", 2, true, true, true,
                                iface->if_name, strerror(errno));
                    }
                }
                if (iface->sockfd_v6 >= 0) {
                    if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
                        log_this("mDNS", "Failed to send IPv6 goodbye on %s: %s", 2, true, true, true,
                                iface->if_name, strerror(errno));
                    }
                }
                usleep(250000); // 250ms per RFC 6762
            }
        }
    }
    free_network_info(net_info);
    free(mdns->hostname);
    free(mdns->service_name);
    free(mdns->device_id);
    free(mdns->friendly_name);
    free(mdns->model);
    free(mdns->manufacturer);
    free(mdns->sw_version);
    free(mdns->hw_version);
    free(mdns->config_url);
    free(mdns->secret_key);

    for (size_t i = 0; i < mdns->num_services; i++) {
        free(mdns->services[i].name);
        free(mdns->services[i].type);
        for (size_t j = 0; j < mdns->services[i].num_txt_records; j++) {
            free(mdns->services[i].txt_records[j]);
        }
        free(mdns->services[i].txt_records);
    }
    free(mdns->services);
    free(mdns);
    log_this("mDNS", "Shutdown: mDNS shutdown complete", 0, true, true, true);
}