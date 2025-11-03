/*
 * mDNS Server Socket Management for the Hydrogen printer.
 *
 * Contains socket creation and management functionality split from the main
 * mDNS server implementation. This includes multicast socket setup for IPv4/IPv6.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <poll.h>

#include "mdns_server.h"

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
        log_this(SR_MDNS_SERVER, "Failed to set SO_REUSEADDR: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        close(sockfd);
        return -1;
    }

    // Add SO_REUSEPORT for better multicast socket sharing
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
        log_this(SR_MDNS_SERVER, "Failed to set SO_REUSEPORT: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
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

    log_this(SR_MDNS_SERVER, "Created multicast socket on interface %s", LOG_LEVEL_DEBUG, 1, if_name);
    return sockfd;
}