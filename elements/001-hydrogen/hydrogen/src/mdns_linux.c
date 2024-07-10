#include "keys.h"
#include "mdns.h"
#include "utils.h"
#include "logging.h"
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

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
        if ((*ptr & 0xC0) == 0xC0) {  // Compressed name pointer
            uint16_t offset = ((*ptr & 0x3F) << 8) | *(ptr + 1);
            read_dns_name((uint8_t*)packet + offset, packet, name + i, name_len - i);
            return ptr + 2;
        }

        size_t len = *ptr++;
        if (i + len + 1 >= name_len) return NULL;  // Name too long
        memcpy(name + i, ptr, len);
        i += len;
        name[i++] = '.';
        ptr += len;
    }
    if (i > 0) name[i - 1] = '\0';  // Remove trailing dot
    else name[0] = '\0';
    return ptr + 1;
}

static int create_multicast_socket(int family, const char *group) {
    int sockfd = socket(family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_this("mDNS", "Failed to create socket: %s", 3, true, true, true, strerror(errno));
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
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(group);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            log_this("mDNS", "Failed to join IPv4 multicast group: %s", 3, true, true, true, strerror(errno));
            close(sockfd);
            return -1;
        }
    } else {
        struct ipv6_mreq mreq;
        inet_pton(AF_INET6, group, &mreq.ipv6mr_multiaddr);
        mreq.ipv6mr_interface = 0;  // Any interface
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
            log_this("mDNS", "Failed to join IPv6 multicast group: %s", 3, true, true, true, strerror(errno));
            close(sockfd);
            return -1;
        }
    }

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
    *ptr++ = 0;  // Null terminator
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
    uint16_t data_len = strlen(ptr_data) + 2;  // Length of PTR data
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    ptr = write_dns_name(ptr, ptr_data);
    return ptr;
}

static uint8_t *write_dns_srv_record(uint8_t *ptr, const char *name, uint16_t priority, uint16_t weight, uint16_t port, const char *target, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_SRV); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t data_len = 6 + strlen(target) + 2;  // Priority + Weight + Port + Target
    *((uint16_t*)ptr) = htons(data_len); ptr += 2;
    *((uint16_t*)ptr) = htons(priority); ptr += 2;
    *((uint16_t*)ptr) = htons(weight); ptr += 2;
    *((uint16_t*)ptr) = htons(port); ptr += 2;
    ptr = write_dns_name(ptr, target);
    return ptr;
}

static uint8_t *write_dns_txt_record(uint8_t *ptr, const char *name, const char *txt_data, uint32_t ttl) {
    ptr = write_dns_name(ptr, name);
    *((uint16_t*)ptr) = htons(MDNS_TYPE_TXT); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    *((uint32_t*)ptr) = htonl(ttl); ptr += 4;
    uint16_t txt_len = strlen(txt_data) + 1;  // Length byte + TXT data
    *((uint16_t*)ptr) = htons(txt_len); ptr += 2;
    *ptr++ = strlen(txt_data);
    memcpy(ptr, txt_data, strlen(txt_data));
    ptr += strlen(txt_data);
    return ptr;
}

void mdns_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname, const mdns_t *mdns, uint32_t ttl) {
    dns_header_t *header = (dns_header_t *)packet;
    header->id = 0;  // mDNS uses 0
    header->flags = htons(MDNS_FLAG_RESPONSE);
    header->qdcount = 0;  // No questions
    header->ancount = htons(1);  // Start with 1 for hostname
    header->nscount = 0;
    header->arcount = 0;

    uint8_t *ptr = packet + sizeof(dns_header_t);

    // A and AAAA records
    network_info_t *net_info = get_network_info();
    if (net_info && net_info->primary_index >= 0 && net_info->primary_index < net_info->count) {
        const interface_t *primary = &net_info->interfaces[net_info->primary_index];
        for (int i = 0; i < primary->ip_count; i++) {
            struct in_addr addr;
            struct in6_addr addr6;
            if (inet_pton(AF_INET, primary->ips[i], &addr) == 1) {
                ptr = write_dns_record(ptr, hostname, MDNS_TYPE_A, MDNS_CLASS_IN, ttl, &addr.s_addr, 4);
                header->ancount = htons(ntohs(header->ancount) + 1);
            } else if (inet_pton(AF_INET6, primary->ips[i], &addr6) == 1) {
                ptr = write_dns_record(ptr, hostname, MDNS_TYPE_AAAA, MDNS_CLASS_IN, ttl, &addr6.s6_addr, 16);
                header->ancount = htons(ntohs(header->ancount) + 1);
            }
        }
    }
    free_network_info(net_info);

    // Service records
    for (int i = 0; i < mdns->num_services; i++) {
        char full_service_name[256];
        snprintf(full_service_name, sizeof(full_service_name), "%s.%s.local", mdns->device_id, mdns->services[i].service_type);

        // PTR record
        ptr = write_dns_ptr_record(ptr, mdns->services[i].service_type, full_service_name, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);

        // SRV record
        ptr = write_dns_srv_record(ptr, full_service_name, 0, 0, mdns->services[i].port, hostname, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);

        // TXT record
        ptr = write_dns_txt_record(ptr, full_service_name, mdns->services[i].txt_records, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);
    }

    *packet_len = ptr - packet;
}

void mdns_send_announcement(mdns_t *mdns, int port, const network_info_t *net_info) {
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len;

    mdns_build_announcement(packet, &packet_len, mdns->hostname, mdns, MDNS_TTL);

    struct sockaddr_in addr_v4;
    memset(&addr_v4, 0, sizeof(addr_v4));
    addr_v4.sin_family = AF_INET;
    addr_v4.sin_port = htons(MDNS_PORT);
    inet_pton(AF_INET, MDNS_GROUP_V4, &addr_v4.sin_addr);

    if (sendto(mdns->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof(addr_v4)) < 0) {
        log_this("mDNS", "Failed to send IPv4 mDNS announcement: %s", 3, true, true, true, strerror(errno));
    } else {
        log_this("mDNS", "Sent IPv4 mDNS announcement to %s:%d", 0, true, true, true, MDNS_GROUP_V4, MDNS_PORT);
    }

    if (mdns->sockfd_v6 >= 0) {  // Only try IPv6 if we have a socket
        struct sockaddr_in6 addr_v6;
        memset(&addr_v6, 0, sizeof(addr_v6));
        addr_v6.sin6_family = AF_INET6;
        addr_v6.sin6_port = htons(MDNS_PORT);
        inet_pton(AF_INET6, MDNS_GROUP_V6, &addr_v6.sin6_addr);

        if (sendto(mdns->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
            log_this("mDNS", "Failed to send IPv6 mDNS announcement: %s", 2, true, true, true, strerror(errno));
        } else {
            log_this("mDNS", "Sent IPv6 mDNS announcement", 0, true, true, true);
        }
    }

    log_this("mDNS", "Announced %s on port %d",0, true, true, true, mdns->service_name, port);
}
void *mdns_announce_loop(void *arg) {
    mdns_thread_arg_t *thread_arg = (mdns_thread_arg_t *)arg;

    while (*thread_arg->running) {

        mdns_send_announcement(thread_arg->mdns, thread_arg->port, thread_arg->net_info);

        for (int i = 0; i < 60 && *thread_arg->running; i++) {
            sleep(1);  // Sleep for 1 second at a time
        }
        log_this("mDNS", "mDNS announce loop: Running = %d", 0, true, true, true,  *thread_arg->running);
    }

    log_this("mDNS", "DEBUG: mDNS announce loop exiting.", 0, true, true, true);
    return NULL;
}

void *mdns_responder_loop(void *arg) {
    mdns_thread_arg_t *thread_arg = (mdns_thread_arg_t *)arg;
    uint8_t buffer[MDNS_MAX_PACKET_SIZE];
    char name[256];

       struct timeval tv;
       tv.tv_sec = 1;  // 1 second timeout
       tv.tv_usec = 0;
       setsockopt(thread_arg->mdns->sockfd_v4, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (*thread_arg->running) {
        struct sockaddr_storage src_addr;
        socklen_t src_len = sizeof(src_addr);

        ssize_t len = recvfrom(thread_arg->mdns->sockfd_v4, buffer, sizeof(buffer), 0,
                               (struct sockaddr *)&src_addr, &src_len);
        if (len < 0) {
            log_this("mDNS", "Failed to receive mDNS packet: %s", 3, true, true, true, strerror(errno));
            continue;
        }

        dns_header_t *header = (dns_header_t *)buffer;
        uint8_t *ptr = buffer + sizeof(dns_header_t);

        for (int i = 0; i < ntohs(header->qdcount); i++) {
            ptr = read_dns_name(ptr, buffer, name, sizeof(name));
            if (!ptr) break;

            uint16_t qtype = ntohs(*((uint16_t*)ptr)); ptr += 2;
            uint16_t qclass = ntohs(*((uint16_t*)ptr)); ptr += 2;

            if (qclass == MDNS_CLASS_IN) {
                if ((qtype == MDNS_TYPE_PTR && strcmp(name, "_http._tcp.local") == 0) ||
                    (qtype == MDNS_TYPE_ANY) ||
                    (qtype == MDNS_TYPE_SRV && strcmp(name, thread_arg->mdns->service_name) == 0) ||
                    (qtype == MDNS_TYPE_TXT && strcmp(name, thread_arg->mdns->service_name) == 0) ||
                    (qtype == MDNS_TYPE_A && strcmp(name, thread_arg->mdns->hostname) == 0)) {
                    mdns_send_announcement(thread_arg->mdns, thread_arg->port, thread_arg->net_info);
                    break;
                }
            }
        }
    }

    log_this("mDNS", "mDNS responder loop exiting.", 0, true, true, true);
    return NULL;
}

mdns_t *mdns_init(const char *app_name, 
		  const char *id, 
		  const char *friendly_name, 
		  const char *model, 
		  const char *manufacturer, 
		  const char *sw_version, 
		  const char *hw_version, 
		  const char *config_url,
		  mdns_service_t *services,
		  int num_services) {

    mdns_t *mdns = malloc(sizeof(mdns_t));
    if (!mdns) {
        log_this("mDNS", "Out of memory", 4, true, true, true);
        return NULL;
    }

    network_info_t *net_info = get_network_info();
    if (!net_info || net_info->primary_index == -1) {
        log_this("mDNS", "Failed to get network info", 3, true, true, true);
        free(mdns);
        return NULL;
    }

    mdns->sockfd_v4 = create_multicast_socket(AF_INET, MDNS_GROUP_V4);
    mdns->sockfd_v6 = create_multicast_socket(AF_INET6, MDNS_GROUP_V6);

    if (mdns->sockfd_v4 < 0 && mdns->sockfd_v6 < 0) {
        log_this("mDNS", "Failed to create any multicast sockets", 3, true, true, true);
        free(mdns);
        free_network_info(net_info);
        return NULL;
    }

    mdns->num_services = num_services;
    mdns->services = malloc(sizeof(mdns_service_t) * num_services);
    if (!mdns->services) {
        log_this("mDNS", "Out of memory for services", 3, true, true, true);
        free(mdns);
        return NULL;
    }

    for (int i = 0; i < num_services; i++) {
        mdns->services[i].service_type = strdup(services[i].service_type);
        mdns->services[i].port = services[i].port;
        mdns->services[i].txt_records = strdup(services[i].txt_records);
        if (!mdns->services[i].service_type || !mdns->services[i].txt_records) {
            log_this("mDNS", "Out of memory for service info", 3, true, true, true);
            // Clean up allocated memory
            for (int j = 0; j < i; j++) {
                free(mdns->services[j].service_type);
                free(mdns->services[j].txt_records);
            }
            free(mdns->services);
            free(mdns);
            return NULL;
        }
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        log_this("mDNS", "Failed to get hostname: %s", 3, true, true, true, strerror(errno));
        strncpy(hostname, "unknown", sizeof(hostname));
    }
    char *dot = strchr(hostname, '.');
    if (dot) *dot = '\0';  // Truncate at first dot

    mdns->hostname = malloc(strlen(hostname) + 7);  // + 7 for ".local" and null
    if (!mdns->hostname) {
        log_this("mDNS", "Out of memory", 4, true, true, true);
        mdns_shutdown(mdns);
        free_network_info(net_info);
        return NULL;
    }
    sprintf(mdns->hostname, "%s.local", hostname);

    mdns->service_name = strdup(app_name);
    mdns->device_id = strdup(id);
    mdns->friendly_name = strdup(friendly_name);
    mdns->model = strdup(model);
    mdns->manufacturer = strdup(manufacturer);
    mdns->sw_version = strdup(sw_version);
    mdns->hw_version = strdup(hw_version);
    mdns->config_url = strdup(config_url);
    mdns->secret_key = generate_secret_key();  // Implement this function to generate a secure random key

    if (!mdns->service_name || !mdns->device_id || !mdns->friendly_name ||
        !mdns->model || !mdns->manufacturer || !mdns->sw_version ||
        !mdns->hw_version || !mdns->config_url || !mdns->secret_key) {
        log_this("mDNS", "Out of memory", 4, true, true, true);
        mdns_shutdown(mdns);
        free_network_info(net_info);
        return NULL;
    }

    log_this("mDNS", "mDNS initialized with hostname: %s", 0, true, true, true,  mdns->hostname);
    free_network_info(net_info);
    return mdns;
}

void mdns_shutdown(mdns_t *mdns) {
    if (mdns) {
        // Send "Goodbye" record
        network_info_t *net_info = get_network_info();
        if (net_info && net_info->primary_index != -1) {
            uint8_t packet[MDNS_MAX_PACKET_SIZE];
            size_t packet_len;
	    mdns_build_announcement(packet, &packet_len, mdns->hostname, mdns, MDNS_TTL);

            struct sockaddr_in addr_v4;
            memset(&addr_v4, 0, sizeof(addr_v4));
            addr_v4.sin_family = AF_INET;
            addr_v4.sin_port = htons(MDNS_PORT);
            inet_pton(AF_INET, MDNS_GROUP_V4, &addr_v4.sin_addr);

            struct sockaddr_in6 addr_v6;
            memset(&addr_v6, 0, sizeof(addr_v6));
            addr_v6.sin6_family = AF_INET6;
            addr_v6.sin6_port = htons(MDNS_PORT);
            inet_pton(AF_INET6, MDNS_GROUP_V6, &addr_v6.sin6_addr);

            for (int i = 0; i < 3; i++) {  // Send goodbye 3 times
                ssize_t sent;
                do {
                    sent = sendto(mdns->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof(addr_v4));
                } while (sent < 0 && errno == EINTR);

                if (sent < 0) {
                    log_this("mDNS", "Failed to send IPv4 mDNS goodbye: %s", 3, true, true, true, strerror(errno));
                } else {
                    log_this("mDNS", "Sent IPv4 mDNS goodbye", 0, true, true, true);
                }

                if (mdns->sockfd_v6 >= 0) {
                    do {
                        sent = sendto(mdns->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6));
                    } while (sent < 0 && errno == EINTR);

                    if (sent < 0) {
                        log_this("mDNS", "Failed to send IPv6 mDNS goodbye: %s", 2, true, true, true, strerror(errno));
                    } else {
                        log_this("mDNS", "Sent IPv6 mDNS goodbye", 0, true, true, true);
                    }
                }

                usleep(20000);  // Wait 20ms between sends
            }

            usleep(100000);  // Wait 100ms to ensure packets are sent before closing sockets
        }
        free_network_info(net_info);

        if (mdns->sockfd_v4 >= 0) close(mdns->sockfd_v4);
        if (mdns->sockfd_v6 >= 0) close(mdns->sockfd_v6);
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
           free(mdns);
        log_this("mDNS", "mDNS shutdown", 0, true, true, true);
    }
}
