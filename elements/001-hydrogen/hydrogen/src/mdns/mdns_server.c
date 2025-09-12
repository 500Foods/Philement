/*
 * Linux implementation of mDNS Server for the Hydrogen printer.
 * 
 * Implements DNS-SD/mDNS Server (RFC 6762, RFC 6763) for network service
 * announcements. The implementation provides automatic network presence
 * for Hydrogen printers with the following features:
 * 
 */

#include "../hydrogen.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h> 
#include <poll.h>

#include "mdns_keys.h"
#include "mdns_server.h"

extern volatile sig_atomic_t server_running;
// Get configured retry count for interface failure detection
int get_mdns_server_retry_count(const AppConfig* config) {
    if (!config) return 1;
    // Ensure retry_count is at least 1 to prevent division by zero or infinite loop
    return (config->mdns_server.retry_count > 0) ? config->mdns_server.retry_count : 1;
}

extern volatile sig_atomic_t mdns_server_system_shutdown;
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

int create_multicast_socket(int family, const char *group, const char *if_name);
uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len);
uint8_t *write_dns_name(uint8_t *ptr, const char *name);
uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen);
uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl);
uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl);
uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl);
void mdns_server_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname, const mdns_server_t *mdns_server_instance, uint32_t ttl, const network_info_t *net_info_instance);
void mdns_server_send_announcement(mdns_server_t *mdns_server_instance, const network_info_t *net_info_instance);

uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len) {
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


int create_multicast_socket(int family, const char *group, const char *if_name) {
    if (!if_name) {
        log_this(SR_MDNS_SERVER, "No interface name provided", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    int sockfd = socket(family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_this(SR_MDNS_SERVER, "Failed to create socket: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, if_name, (socklen_t)strlen(if_name)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to bind to interface %s: %s", LOG_LEVEL_DEBUG, 2, if_name, strerror(errno));
        close(sockfd);
        return -1;
    }

    unsigned int if_index = if_nametoindex(if_name);
    if (if_index == 0) {
        log_this(SR_MDNS_SERVER, "Failed to get interface index for %s: %s", LOG_LEVEL_DEBUG, 2, if_name, strerror(errno));
        close(sockfd);
        return -1;
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to set SO_REUSEADDR: %s", LOG_LEVEL_DEBUG, 2, strerror(errno));
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
        log_this(SR_MDNS_SERVER, "Failed to bind socket: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        close(sockfd);
        return -1;
    }

    int ttl = MDNS_TTL;
    if (setsockopt(sockfd, family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                   family == AF_INET ? IP_MULTICAST_TTL : IPV6_MULTICAST_HOPS,
                   &ttl, sizeof(ttl)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to set multicast TTL: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        close(sockfd);
        return -1;
    }

    int loop = 1;
    if (setsockopt(sockfd, family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                   family == AF_INET ? IP_MULTICAST_LOOP : IPV6_MULTICAST_LOOP,
                   &loop, sizeof(loop)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to enable multicast loop: %s", LOG_LEVEL_ALERT, 1, strerror(errno));
    }

    if (family == AF_INET) {
        struct ip_mreqn mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(group);
        mreq.imr_address.s_addr = INADDR_ANY;
        mreq.imr_ifindex = (if_index <= INT_MAX) ? (int)if_index : 0;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
            log_this(SR_MDNS_SERVER, "Failed to set IPv4 multicast interface: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            close(sockfd);
            return -1;
        }
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            log_this(SR_MDNS_SERVER, "Failed to join IPv4 multicast group: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            close(sockfd);
            return -1;
        }
    } else {
        struct ipv6_mreq mreq;
        inet_pton(AF_INET6, group, &mreq.ipv6mr_multiaddr);
        mreq.ipv6mr_interface = if_index;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &if_index, sizeof(if_index)) < 0) {
            log_this(SR_MDNS_SERVER, "Failed to set IPv6 multicast interface: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            close(sockfd);
            return -1;
        }
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
            log_this(SR_MDNS_SERVER, "Failed to join IPv6 multicast group: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            close(sockfd);
            return -1;
        }
    }

    log_this(SR_MDNS_SERVER, "Created multicast socket on interface %s", LOG_LEVEL_STATE, 1, if_name);
    return sockfd;
}

uint8_t *write_dns_name(uint8_t *ptr, const char *name) {
    // Defensive check for NULL name
    if (!name) {
        *ptr++ = 0;  // Just write null terminator for NULL name
        return ptr;
    }

    const char *part = name;
    while (*part) {
        const char *end = strchr(part, '.');
        if (!end) end = part + strlen(part);
        size_t len = (size_t)(end - part);
        *ptr++ = (uint8_t)len;
        memcpy(ptr, part, len);
        ptr += len;
        if (*end == '.') part = end + 1;
        else part = end;
    }
    *ptr++ = 0;
    return ptr;
}

uint8_t *write_dns_record(uint8_t *ptr, const char *name, uint16_t type, uint16_t class, uint32_t ttl, const void *rdata, uint16_t rdlen) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(type); ptr += 2;
    *((uint16_t*)ptr) = htons(class); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    *((uint16_t*)ptr) = htons(rdlen); ptr += 2;
    memcpy(ptr, rdata, rdlen);
    ptr += rdlen;
    return ptr;
}

uint8_t *write_dns_ptr_record(uint8_t *ptr, const char *name, const char *ptr_data, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = (uint16_t)(strlen(ptr_data) + 2);
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    ptr = write_dns_name(ptr, ptr_data);
    return ptr;
}

uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_SRV); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = (uint16_t)(6 + strlen(target) + 2);
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    *((uint16_t*)ptr) = htons(priority); ptr += 2;
    *((uint16_t*)ptr) = htons(weight); ptr += 2;
    *((uint16_t*)ptr) = htons(port); ptr += 2;
    ptr = write_dns_name(ptr, target);
    return ptr;
}

uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, char **txt_records, size_t num_txt_records, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_TXT); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    size_t total_len = 0;
    for (size_t i = 0; i < num_txt_records; i++) {
        total_len += strlen(txt_records[i]) + 1;
    }
    *((uint16_t*)ptr) = htons((uint16_t)total_len); ptr += 2;
    for (size_t i = 0; i < num_txt_records; i++) {
        size_t len = strlen(txt_records[i]);
        *ptr++ = (uint8_t)len;
        memcpy(ptr, txt_records[i], len);
        ptr += len;
    }
    return ptr;
}



void *mdns_server_announce_loop(void *arg) {
    mdns_server_thread_arg_t *thread_arg = (mdns_server_thread_arg_t *)arg;
    mdns_server_t *mdns_server_instance = thread_arg->mdns_server;
    add_service_thread(&mdns_server_threads, pthread_self());

    log_this(SR_MDNS_SERVER, "mDNS Server announce loop started", LOG_LEVEL_STATE, 0);

    int initial_announcements = 3; // Initial burst
    unsigned int interval = 1; // Start with 1-second intervals

    while (!mdns_server_system_shutdown) {
        if (initial_announcements > 0) {
            mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
            initial_announcements--;
            sleep(interval);
            if (initial_announcements == 0) interval = 60; // Switch to normal interval
        } else {
            pthread_mutex_lock(&terminate_mutex);
            if (!mdns_server_system_shutdown) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += interval;
                pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
            }
            pthread_mutex_unlock(&terminate_mutex);

            if (!mdns_server_system_shutdown) {
                mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
            }
        }
    }

    log_this(SR_MDNS_SERVER, "Shutdown: mDNS Server announce loop exiting", LOG_LEVEL_STATE, 0);
    remove_service_thread(&mdns_server_threads, pthread_self());
    free(thread_arg);
    return NULL;
}

void *mdns_server_responder_loop(void *arg) {
    mdns_server_thread_arg_t *thread_arg = (mdns_server_thread_arg_t *)arg;
    mdns_server_t *mdns_server_instance = thread_arg->mdns_server;
    uint8_t buffer[MDNS_MAX_PACKET_SIZE];
    char name[256];

    add_service_thread(&mdns_server_threads, pthread_self());
    log_this(SR_MDNS_SERVER, "mDNS Server responder loop started", LOG_LEVEL_STATE, 0);

    // Create pollfd array for all interface sockets
    size_t num_sockets = mdns_server_instance->num_interfaces * 2U; // 2 sockets per interface (v4/v6)
    struct pollfd *fds = malloc(sizeof(struct pollfd) * num_sockets);
    if (!fds) {
        log_this(SR_MDNS_SERVER, "Out of memory for poll fds", LOG_LEVEL_DEBUG, 0);
        remove_service_thread(&mdns_server_threads, pthread_self());
        free(thread_arg);
        return NULL;
    }

    nfds_t nfds = 0;  // Use nfds_t type to avoid sign conversion
    for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
        mdns_server_interface_t *iface = &mdns_server_instance->interfaces[i];
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
        log_this(SR_MDNS_SERVER, "No sockets to monitor", LOG_LEVEL_DEBUG, 0);
        free(fds);
        remove_service_thread(&mdns_server_threads, pthread_self());
        free(thread_arg);
        return NULL;
    }

    while (!mdns_server_system_shutdown) {
        int ret = poll(fds, nfds, 1000); // 1-second timeout, no cast needed
        if (ret < 0) {
            if (errno != EINTR) {
                log_this(SR_MDNS_SERVER, "Poll error: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            }
            continue;
        }
        if (ret == 0) continue;

        for (nfds_t i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                struct sockaddr_storage src_addr;
                socklen_t src_len = sizeof(src_addr);
                ssize_t len = recvfrom(fds[i].fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_addr, &src_len);
                if (len < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        log_this(SR_MDNS_SERVER, "Failed to receive mDNS Server packet: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
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
                            for (size_t k = 0; k < mdns_server_instance->num_services; k++) {
                                if (strcmp(name, mdns_server_instance->services[k].type) == 0) {
                                    mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
                                    goto next_packet;
                                }
                            }
                        } else if (qtype == MDNS_TYPE_SRV || qtype == MDNS_TYPE_TXT) {
                            for (size_t k = 0; k < mdns_server_instance->num_services; k++) {
                                char full_name[256];
                                snprintf(full_name, sizeof(full_name), "%s.%s", mdns_server_instance->services[k].name, mdns_server_instance->services[k].type);
                                if (strcmp(name, full_name) == 0) {
                                    mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
                                    goto next_packet;
                                }
                            }
                        } else if ((qtype == MDNS_TYPE_A || qtype == MDNS_TYPE_AAAA || qtype == MDNS_TYPE_ANY) &&
                                   strcmp(name, mdns_server_instance->hostname) == 0) {
                            mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
                            goto next_packet;
                        }
                    }
                }
            next_packet:
                continue;
            }
        }
    }

    log_this(SR_MDNS_SERVER, "Shutdown: mDNS Server responder loop exiting", LOG_LEVEL_STATE, 0);
    remove_service_thread(&mdns_server_threads, pthread_self());
    free(thread_arg);
    return NULL;
}

mdns_server_t *mdns_server_init(const char *app_name, const char *id, const char *friendly_name,
                  const char *model, const char *manufacturer, const char *sw_version,
                  const char *hw_version, const char *config_url,
                  mdns_server_service_t *services, size_t num_services, int enable_ipv6) {
    mdns_server_t *mdns_server_instance = malloc(sizeof(mdns_server_t));
    if (!mdns_server_instance) {
        log_this(SR_MDNS_SERVER, "Out of memory", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    mdns_server_instance->enable_ipv6 = enable_ipv6;

    // Get raw network info first
    network_info_t *raw_net_info = get_network_info();
    if (!raw_net_info) {
        log_this(SR_MDNS_SERVER, "Failed to get network info", LOG_LEVEL_DEBUG, 0);
        free(mdns_server_instance);
        return NULL;
    }

    // Filter interfaces based on Network.Available configuration
    network_info_t *net_info_instance = filter_enabled_interfaces(raw_net_info, app_config);
    free_network_info(raw_net_info); // Clean up raw network info

    if (!net_info_instance || net_info_instance->count == 0) {
        log_this(SR_MDNS_SERVER, "No enabled interfaces found after filtering", LOG_LEVEL_DEBUG, 0);
        free(mdns_server_instance);
        if (net_info_instance) {
            free_network_info(net_info_instance);
        }
        return NULL;
    }

    // Allocate space for interfaces
    size_t interface_count = (net_info_instance->count >= 0) ? (size_t)net_info_instance->count : 0;
    mdns_server_instance->interfaces = malloc(sizeof(mdns_server_interface_t) * interface_count);
    mdns_server_instance->num_interfaces = 0;
    if (!mdns_server_instance->interfaces) {
        log_this(SR_MDNS_SERVER, "Out of memory for interfaces", LOG_LEVEL_DEBUG, 0);
        free(mdns_server_instance);
        free_network_info(net_info_instance);
        return NULL;
    }

    // Initialize each interface
    for (int i = 0; i < net_info_instance->count; i++) {
        const interface_t *iface = &net_info_instance->interfaces[i];
        
        // Skip loopback and interfaces without IPs
        if (strcmp(iface->name, "lo") == 0 || iface->ip_count == 0) {
            continue;
        }

        mdns_server_interface_t *mdns_server_if = &mdns_server_instance->interfaces[mdns_server_instance->num_interfaces];
        mdns_server_if->if_name = strdup(iface->name);
        if (iface->ip_count > 0) {
            size_t ip_count = (size_t)iface->ip_count;
            mdns_server_if->ip_addresses = malloc(sizeof(char*) * ip_count);
            mdns_server_if->num_addresses = ip_count;
        } else {
            mdns_server_if->ip_addresses = NULL;
            mdns_server_if->num_addresses = 0;
        }

        if (!mdns_server_if->if_name || !mdns_server_if->ip_addresses) {
            log_this(SR_MDNS_SERVER, "Out of memory for interface %s", LOG_LEVEL_DEBUG, 1, iface->name);
            goto cleanup;
        }

        // Copy IP addresses
        for (int j = 0; j < iface->ip_count; j++) {
            mdns_server_if->ip_addresses[j] = strdup(iface->ips[j]);
            if (!mdns_server_if->ip_addresses[j]) {
                log_this(SR_MDNS_SERVER, "Out of memory for IP address", LOG_LEVEL_DEBUG, 0);
                goto cleanup;
            }
        }

    // Create sockets
    mdns_server_if->sockfd_v4 = create_multicast_socket(AF_INET, MDNS_GROUP_V4, mdns_server_if->if_name);
    mdns_server_if->sockfd_v6 = enable_ipv6 ? create_multicast_socket(AF_INET6, MDNS_GROUP_V6, mdns_server_if->if_name) : -1;

    if (mdns_server_if->sockfd_v4 < 0 && (!enable_ipv6 || mdns_server_if->sockfd_v6 < 0)) {
        log_this(SR_MDNS_SERVER, "Failed to create sockets for interface %s", LOG_LEVEL_DEBUG, 1, mdns_server_if->if_name);
        goto cleanup;
    }

    // Initialize failure tracking for automatic disable/enable
    mdns_server_if->consecutive_failures = 0;
    mdns_server_if->disabled = 0;

    // Initialize protocol-level failure tracking
    mdns_server_if->v4_consecutive_failures = 0;
    mdns_server_if->v6_consecutive_failures = 0;
    mdns_server_if->v4_disabled = 0;
    mdns_server_if->v6_disabled = 0;

    mdns_server_instance->num_interfaces++;
    }

    if (mdns_server_instance->num_interfaces == 0) {
        log_this(SR_MDNS_SERVER, "No usable interfaces found", LOG_LEVEL_DEBUG, 0);
        goto cleanup;
    }

    // Initialize services
    mdns_server_instance->num_services = num_services;
    mdns_server_instance->services = malloc(sizeof(mdns_server_service_t) * num_services);
    if (!mdns_server_instance->services) {
        log_this(SR_MDNS_SERVER, "Out of memory for services", LOG_LEVEL_DEBUG, 0);
        free(mdns_server_instance);
        free_network_info(net_info_instance);
        return NULL;
    }

    for (size_t i = 0; i < num_services; i++) {
        mdns_server_instance->services[i].name = strdup(services[i].name);
        mdns_server_instance->services[i].type = strdup(services[i].type);
        mdns_server_instance->services[i].port = services[i].port;
        mdns_server_instance->services[i].num_txt_records = services[i].num_txt_records;
        if (services[i].num_txt_records > 0) {
            size_t txt_count = (size_t)services[i].num_txt_records;
            mdns_server_instance->services[i].txt_records = malloc(sizeof(char*) * txt_count);
        } else {
            mdns_server_instance->services[i].txt_records = NULL;
        }
        for (size_t j = 0; j < (size_t)services[i].num_txt_records; j++) {
            mdns_server_instance->services[i].txt_records[j] = strdup(services[i].txt_records[j]);
        }
    }

    // Initialize hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to get hostname: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        strncpy(hostname, "unknown", sizeof(hostname));
    }
    char *dot = strchr(hostname, '.');
    if (dot) *dot = '\0';

    mdns_server_instance->hostname = malloc(strlen(hostname) + 7);
    if (!mdns_server_instance->hostname) {
        log_this(SR_MDNS_SERVER, "Out of memory", LOG_LEVEL_ERROR, 0);
        goto cleanup;
    }
    sprintf(mdns_server_instance->hostname, "%s.local", hostname);

    // Initialize service info
    mdns_server_instance->service_name = strdup(app_name);
    mdns_server_instance->device_id = strdup(id);
    mdns_server_instance->friendly_name = strdup(friendly_name);
    mdns_server_instance->model = strdup(model);
    mdns_server_instance->manufacturer = strdup(manufacturer);
    mdns_server_instance->sw_version = strdup(sw_version);
    mdns_server_instance->hw_version = strdup(hw_version);
    mdns_server_instance->hw_version = strdup(hw_version);
    mdns_server_instance->config_url = strdup(config_url);
    mdns_server_instance->secret_key = generate_secret_mdns_key();

    if (!mdns_server_instance->service_name || !mdns_server_instance->device_id || !mdns_server_instance->friendly_name ||
        !mdns_server_instance->model || !mdns_server_instance->manufacturer || !mdns_server_instance->sw_version ||
        !mdns_server_instance->hw_version || !mdns_server_instance->config_url || !mdns_server_instance->secret_key) {
        log_this(SR_MDNS_SERVER, "Out of memory", LOG_LEVEL_ERROR, 0);
        goto cleanup;
    }

    log_this(SR_MDNS_SERVER, "mDNS Server initialized with hostname: %s", LOG_LEVEL_STATE, 1, mdns_server_instance->hostname);
    free_network_info(net_info_instance);
    return mdns_server_instance;

cleanup:
    if (mdns_server_instance) {
        if (mdns_server_instance->interfaces) {
            for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
                mdns_server_interface_t *mdns_server_if = &mdns_server_instance->interfaces[i];
                if (mdns_server_if->if_name) free(mdns_server_if->if_name);
                if (mdns_server_if->ip_addresses) {
                    for (size_t j = 0; j < mdns_server_if->num_addresses; j++) {
                        if (mdns_server_if->ip_addresses[j]) free(mdns_server_if->ip_addresses[j]);
                    }
                    free(mdns_server_if->ip_addresses);
                }
                if (mdns_server_if->sockfd_v4 >= 0) close(mdns_server_if->sockfd_v4);
                if (mdns_server_if->sockfd_v6 >= 0) close(mdns_server_if->sockfd_v6);
            }
            free(mdns_server_instance->interfaces);
        }
        if (mdns_server_instance->services) {
            for (size_t i = 0; i < mdns_server_instance->num_services; i++) {
                if (mdns_server_instance->services[i].name) free(mdns_server_instance->services[i].name);
                if (mdns_server_instance->services[i].type) free(mdns_server_instance->services[i].type);
                if (mdns_server_instance->services[i].txt_records) {
                    for (size_t j = 0; j < mdns_server_instance->services[i].num_txt_records; j++) {
                        if (mdns_server_instance->services[i].txt_records[j]) free(mdns_server_instance->services[i].txt_records[j]);
                    }
                    free(mdns_server_instance->services[i].txt_records);
                }
            }
            free(mdns_server_instance->services);
        }
        if (mdns_server_instance->hostname) free(mdns_server_instance->hostname);
        if (mdns_server_instance->service_name) free(mdns_server_instance->service_name);
        if (mdns_server_instance->device_id) free(mdns_server_instance->device_id);
        if (mdns_server_instance->friendly_name) free(mdns_server_instance->friendly_name);
        if (mdns_server_instance->model) free(mdns_server_instance->model);
        if (mdns_server_instance->manufacturer) free(mdns_server_instance->manufacturer);
        if (mdns_server_instance->sw_version) free(mdns_server_instance->sw_version);
        if (mdns_server_instance->hw_version) free(mdns_server_instance->hw_version);
        if (mdns_server_instance->secret_key) free(mdns_server_instance->secret_key);
        free(mdns_server_instance);
    }
    if (net_info_instance) free_network_info(net_info_instance);
    return NULL;
}

/**
 * Close sockets and free interface resources
 * Made non-static for unit testing
 */
void close_mdns_server_interfaces(mdns_server_t *mdns_server_instance) {
    if (!mdns_server_instance || !mdns_server_instance->interfaces) return;
    
    for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
        mdns_server_interface_t *iface = &mdns_server_instance->interfaces[i];
        
        // Close sockets first
        if (iface->sockfd_v4 >= 0) {
            log_this(SR_MDNS_SERVER, "Closing IPv4 socket on interface %s", LOG_LEVEL_STATE, 1, iface->if_name);
            close(iface->sockfd_v4);
            iface->sockfd_v4 = -1;
        }
        
        if (iface->sockfd_v6 >= 0) {
            log_this(SR_MDNS_SERVER, "Closing IPv6 socket on interface %s", LOG_LEVEL_STATE, 1, iface->if_name);
            close(iface->sockfd_v6);
            iface->sockfd_v6 = -1;
        }
    }
}

void mdns_server_shutdown(mdns_server_t *mdns_server_instance) {
    if (!mdns_server_instance) return;

    log_this(SR_MDNS_SERVER, "Shutdown: Initiating mDNS Server shutdown", LOG_LEVEL_STATE, 0);
    
    // Wait for any active threads to notice the shutdown flag
    // This ensures they don't access mdns_server after we free it
    extern ServiceThreads mdns_server_threads;
    update_service_thread_metrics(&mdns_server_threads);
    
    // Check for active threads before proceeding
    if (mdns_server_threads.thread_count > 0) {
        log_this(SR_MDNS_SERVER, "Waiting for %d mDNS Server threads to exit", LOG_LEVEL_STATE, 1, mdns_server_threads.thread_count);
                
        // Wait with timeout for threads to exit
        for (int i = 0; i < 10 && mdns_server_threads.thread_count > 0; i++) {
            usleep(200000);  // 200ms between checks
            update_service_thread_metrics(&mdns_server_threads);
        }
        
        if (mdns_server_threads.thread_count > 0) {
            log_this(SR_MDNS_SERVER, "Warning: %d mDNS Server threads still active", LOG_LEVEL_ALERT, 1, mdns_server_threads.thread_count);
        }
    }
    
    // Send goodbye packets
    network_info_t *net_info_instance = get_network_info();
    if (net_info_instance && net_info_instance->primary_index != -1) {
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
        for (size_t iface_idx = 0; iface_idx < mdns_server_instance->num_interfaces; iface_idx++) {
            mdns_server_interface_t *iface = &mdns_server_instance->interfaces[iface_idx];
            uint8_t packet[MDNS_MAX_PACKET_SIZE];
            size_t packet_len;

            // Build goodbye announcement with primary network interface
            mdns_server_build_announcement(packet, &packet_len, mdns_server_instance->hostname, mdns_server_instance, 0, net_info_instance);
            
            // Send 3 goodbye packets per RFC 6762
            for (int i = 0; i < 3; i++) {
                if (iface->sockfd_v4 >= 0) {
                    if (sendto(iface->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof(addr_v4)) < 0) {
                        log_this(SR_MDNS_SERVER, "Failed to send IPv4 goodbye on %s: %s", LOG_LEVEL_ALERT, 2, iface->if_name, strerror(errno));
                    } else {
                        log_this(SR_MDNS_SERVER, "Sent IPv4 goodbye packet %d/3 on %s", LOG_LEVEL_STATE, 2, i+1, iface->if_name);
                    }
                }
                if (iface->sockfd_v6 >= 0) {
                    if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
                        log_this(SR_MDNS_SERVER, "Failed to send IPv6 goodbye on %s: %s", LOG_LEVEL_ALERT, 2, iface->if_name, strerror(errno));
                    } else {
                        log_this(SR_MDNS_SERVER, "Sent IPv6 goodbye packet %d/3 on %s", LOG_LEVEL_STATE, 2, i+1, iface->if_name);
                    }
                }
                usleep(250000); // 250ms per RFC 6762
            }
        }
    }
    
    if (net_info_instance) {
        free_network_info(net_info_instance);
    }
    
    // Close all sockets before freeing memory
    log_this(SR_MDNS_SERVER, "Closing mDNS Server sockets", LOG_LEVEL_STATE, 0);
    close_mdns_server_interfaces(mdns_server_instance);
    
    // Final check for active threads
    update_service_thread_metrics(&mdns_server_threads);
    if (mdns_server_threads.thread_count > 0) {
        log_this(SR_MDNS_SERVER, "Warning: Proceeding with cleanup with %d threads still active", LOG_LEVEL_ALERT, 1, mdns_server_threads.thread_count);
    }
    
    // Brief delay to ensure no threads are accessing resources
    usleep(200000);  // 200ms delay
    log_this(SR_MDNS_SERVER, "Freeing mDNS Server resources", LOG_LEVEL_STATE, 0);
    
    // Free interfaces
    if (mdns_server_instance->interfaces) {
        for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
            if (mdns_server_instance->interfaces[i].if_name) free(mdns_server_instance->interfaces[i].if_name);
            if (mdns_server_instance->interfaces[i].ip_addresses) {
                for (size_t j = 0; j < mdns_server_instance->interfaces[i].num_addresses; j++) {
                    if (mdns_server_instance->interfaces[i].ip_addresses[j]) {
                        free(mdns_server_instance->interfaces[i].ip_addresses[j]);
                    }
                }
                free(mdns_server_instance->interfaces[i].ip_addresses);
            }
        }
        free(mdns_server_instance->interfaces);
    }
    
    // Free server identity
    free(mdns_server_instance->hostname);
    free(mdns_server_instance->service_name);
    free(mdns_server_instance->device_id);
    free(mdns_server_instance->friendly_name);
    free(mdns_server_instance->model);
    free(mdns_server_instance->manufacturer);
    free(mdns_server_instance->sw_version);
    free(mdns_server_instance->hw_version);
    free(mdns_server_instance->config_url);
    free(mdns_server_instance->secret_key);

    // Free services
    for (size_t i = 0; i < mdns_server_instance->num_services; i++) {
        free(mdns_server_instance->services[i].name);
        free(mdns_server_instance->services[i].type);
        for (size_t j = 0; j < mdns_server_instance->services[i].num_txt_records; j++) {
            free(mdns_server_instance->services[i].txt_records[j]);
        }
        free(mdns_server_instance->services[i].txt_records);
    }
    free(mdns_server_instance->services);
    
    // Finally free the server structure
    free(mdns_server_instance);
    
    log_this(SR_MDNS_SERVER, "Shutdown: mDNS Server shutdown complete", LOG_LEVEL_STATE, 0);
}
