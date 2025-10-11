/*
 * mDNS Server Announcement Implementation for the Hydrogen printer.
 *
 * Contains announcement-specific functionality split from the main mDNS server
 * implementation. This includes packet construction for service advertisements
 * and announcement sending logic.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "mdns_keys.h"
#include "mdns_server.h"

extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// DNS header structure is now defined in mdns_server.h

uint8_t *read_dns_name(uint8_t *ptr, const uint8_t *packet, char *name, size_t name_len);

void _mdns_server_build_interface_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                                             const mdns_server_t *mdns_server_instance, uint32_t ttl, const mdns_server_interface_t *iface);
network_info_t *create_single_interface_net_info(const mdns_server_interface_t *iface);
void free_single_interface_net_info(network_info_t *net_info_instance);

void mdns_server_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                           const mdns_server_t *mdns_server_instance, uint32_t ttl, const network_info_t *net_info_instance);

/**
 * Builds the actual mDNS announcement packet for a specific interface.
 * This is the core announcement packet construction logic that creates PTR, SRV, and TXT records.
 */
void _mdns_server_build_interface_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                                             const mdns_server_t *mdns_server_instance, uint32_t ttl, const mdns_server_interface_t *iface) {
    // Check for NULL packet buffer or length pointer
    if (!packet || !packet_len) {
        log_this(SR_MDNS_SERVER, "Warning: NULL packet buffer or length pointer passed to announcement builder", LOG_LEVEL_ALERT, 0);
        if (packet_len) *packet_len = 0;
        return;
    }

    if (!iface) {
        log_this(SR_MDNS_SERVER, "Warning: NULL interface passed to announcement builder", LOG_LEVEL_ALERT, 0);
        // Initialize header with zeros and return minimum packet
        dns_header_t *header = (dns_header_t *)packet;
        memset(header, 0, sizeof(dns_header_t));
        *packet_len = sizeof(dns_header_t);
        return;
    }

    // Initialize DNS header for response
    dns_header_t *header = (dns_header_t *)packet;
    header->id = 0;
    header->flags = htons(MDNS_FLAG_RESPONSE | MDNS_FLAG_AUTHORITATIVE);
    header->qdcount = 0;
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    uint8_t *ptr = packet + sizeof(dns_header_t);

    // Add IP address records for the hostname
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

    // Add service records (PTR, SRV, TXT)
    for (size_t i = 0; i < mdns_server_instance->num_services; i++) {
        // Use the full service type including .local as PTR owner name
        const char *ptr_owner_name = mdns_server_instance->services[i].type;
    
        // Create full service instance name for SRV and TXT records
        const size_t max_name_len = 100;
        const size_t max_type_len = 100;
        size_t name_len = strlen(mdns_server_instance->services[i].name);
        size_t type_len = strlen(mdns_server_instance->services[i].type);
    
        size_t total_len = name_len + 1 + type_len + 6;  // +6 for extra formatting
        if (total_len >= 256) {
            log_this(SR_MDNS_SERVER, "Service name too long: %s.%s truncated", LOG_LEVEL_ALERT, 2,
                     mdns_server_instance->services[i].name, mdns_server_instance->services[i].type);
            name_len = max_name_len < name_len ? max_name_len : name_len;
            type_len = max_type_len < type_len ? max_type_len : type_len;
        }
    
        // Full service instance name with .local suffix
        char full_service_instance_name[256];
        snprintf(full_service_instance_name, sizeof(full_service_instance_name), "%.*s.%.*s",
                 (int)name_len, mdns_server_instance->services[i].name, (int)type_len, mdns_server_instance->services[i].type);

        // PTR record - owner name MUST include .local as per RFC 6763
        ptr = write_dns_ptr_record(ptr, ptr_owner_name, full_service_instance_name, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);

        // SRV record for service location
        ptr = write_dns_srv_record(ptr, full_service_instance_name, 0, 0, (uint16_t)mdns_server_instance->services[i].port, hostname, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);

        // TXT record with service metadata
        ptr = write_dns_txt_record(ptr, full_service_instance_name, mdns_server_instance->services[i].txt_records, mdns_server_instance->services[i].num_txt_records, ttl);
        header->ancount = htons(ntohs(header->ancount) + 1);
    }

    *packet_len = (size_t)(ptr - packet);
    if (*packet_len > 1500) {
        log_this(SR_MDNS_SERVER, "Warning: Packet size %zu exceeds typical MTU (1500)", LOG_LEVEL_ALERT, 1, *packet_len);
    }
}

/**
 * Public interface for building mDNS announcement packets.
 * Finds the appropriate interface and delegates to the interface-specific builder.
 */
void mdns_server_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                           const mdns_server_t *mdns_server_instance, uint32_t ttl, const network_info_t *net_info_instance) {
    // Defensive check for NULL server instance
    if (!mdns_server_instance) {
        log_this(SR_MDNS_SERVER, "Warning: NULL mDNS server instance passed to build_announcement", LOG_LEVEL_ALERT, 0);
        if (packet_len) *packet_len = 0;
        return;
    }

    // Find the matching interface from net_info
    const mdns_server_interface_t *matching_iface = NULL;
    if (net_info_instance && net_info_instance->primary_index >= 0 && net_info_instance->primary_index < net_info_instance->count) {
        const interface_t *primary = &net_info_instance->interfaces[net_info_instance->primary_index];
        for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
            if (strcmp(mdns_server_instance->interfaces[i].if_name, primary->name) == 0) {
                matching_iface = &mdns_server_instance->interfaces[i];
                break;
            }
        }
    }

    if (!matching_iface && mdns_server_instance->num_interfaces > 0) {
        // Fall back to the first available interface if no match found
        log_this(SR_MDNS_SERVER, "No matching interface found, using first available", LOG_LEVEL_ALERT, 0);
        matching_iface = &mdns_server_instance->interfaces[0];
    }

    // Use the interface-specific announcement builder
    _mdns_server_build_interface_announcement(packet, packet_len, hostname, mdns_server_instance, ttl, matching_iface);
}

/**
 * Creates a minimal network_info_t containing only the specified interface.
 * Used internally for interface-specific announcements.
 */
network_info_t *create_single_interface_net_info(const mdns_server_interface_t *iface) {
    network_info_t *net_info_instance = calloc(1, sizeof(network_info_t));
    if (!net_info_instance) return NULL;

    net_info_instance->count = 1;
    net_info_instance->primary_index = 0;

    // Copy interface name
    strncpy(net_info_instance->interfaces[0].name, iface->if_name, IF_NAMESIZE - 1);
    net_info_instance->interfaces[0].name[IF_NAMESIZE - 1] = '\0';

    // Copy IP addresses
    size_t num_ips = iface->num_addresses;
    if (num_ips > MAX_IPS) {
        num_ips = MAX_IPS;
    }
    // Safely convert size_t to int, clamping to INT_MAX if necessary
    if (num_ips <= (size_t)INT_MAX) {
        net_info_instance->interfaces[0].ip_count = (int)num_ips;
    } else {
        net_info_instance->interfaces[0].ip_count = INT_MAX;
    }

    for (size_t i = 0; i < num_ips; i++) {
        strncpy(net_info_instance->interfaces[0].ips[i], iface->ip_addresses[i], INET6_ADDRSTRLEN - 1);
        net_info_instance->interfaces[0].ips[i][INET6_ADDRSTRLEN - 1] = '\0';
    }

    return net_info_instance;
}

/**
 * Frees the network_info_t created by create_single_interface_net_info.
 */
void free_single_interface_net_info(network_info_t *net_info_instance) {
    free(net_info_instance);
}

/**
 * Sends mDNS service announcements on all configured interfaces.
 * Handles IPv4 and IPv6 multicast sending with failure tracking and automatic retry logic.
 */
void mdns_server_send_announcement(mdns_server_t *mdns_server_instance, const network_info_t *net_info_instance __attribute__((unused))) {
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
    for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
        mdns_server_interface_t *iface = &mdns_server_instance->interfaces[i];
        uint8_t packet[MDNS_MAX_PACKET_SIZE];
        size_t packet_len;

        // Create temporary network_info for this interface
        network_info_t *iface_net_info = create_single_interface_net_info(iface);
        if (!iface_net_info) {
            log_this(SR_MDNS_SERVER, "Failed to create network info for interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
            continue;
        }

        // Build announcement with interface-specific IPs
        mdns_server_build_announcement(packet, &packet_len, mdns_server_instance->hostname, mdns_server_instance, MDNS_TTL, iface_net_info);
        free_single_interface_net_info(iface_net_info);

        // Track send attempt outcomes
        int v4_success = 0;
        int v6_success = 0;

        // Check if entire interface is manually disabled (global disable)
        if (iface->disabled) {
            // log_this(SR_MDNS_SERVER, "Skipping disabled interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
            continue;
        }

        // Get retry count from configuration
        int retry_count = get_mdns_server_retry_count(app_config);

        // Send IPv4 announcement (if not protocol-specifically disabled)
        if (iface->sockfd_v4 >= 0 && iface->v4_disabled == 0) {
            // Add debugging information for packet content
            log_this(SR_MDNS_SERVER, "DEBUG: IPv4 announcement to mDNS group on %s, packet size %zu bytes", LOG_LEVEL_STATE, 2, iface->if_name, packet_len);
            if (sendto(iface->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof(addr_v4)) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send IPv4 announcement on %s: %s", LOG_LEVEL_DEBUG, 2, iface->if_name, strerror(errno));

                // Increment IPv4-specific failure count
                iface->v4_consecutive_failures++;
                log_this(SR_MDNS_SERVER, "IPv4 on %s has %d consecutive failures (limit: %d)", LOG_LEVEL_ALERT, 2, iface->if_name, iface->v4_consecutive_failures, retry_count);

                // Automatically disable IPv4 after configured consecutive failures
                if (iface->v4_consecutive_failures >= retry_count && iface->v4_disabled == 0) {
                    iface->v4_disabled = 1;
                    log_this(SR_MDNS_SERVER, "Automatically disabling IPv4 on %s after %d consecutive failures", LOG_LEVEL_ALERT, 2, iface->if_name, iface->v4_consecutive_failures);
                }
            } else {
                log_this(SR_MDNS_SERVER, "Sent IPv4 announcement on %s", LOG_LEVEL_STATE, 1, iface->if_name);
                v4_success = 1;

                // Reset IPv4 failure count on success
                iface->v4_consecutive_failures = 0;

                // Reactivate IPv4 if it was disabled and is now working
                if (iface->v4_disabled == 1) {
                    iface->v4_disabled = 0;
                    log_this(SR_MDNS_SERVER, "IPv4 on %s recovered from failures, re-enabled", LOG_LEVEL_STATE, 1, iface->if_name);
                }
            }
        } else if (iface->v4_disabled == 1) {
            // log_this(SR_MDNS_SERVER, "Skipping disabled IPv4 on interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
        }

        // Send IPv6 announcement (if not protocol-specifically disabled)
        if (iface->sockfd_v6 >= 0 && iface->v6_disabled == 0) {
            if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send IPv6 announcement on %s: %s", LOG_LEVEL_ALERT, 2, iface->if_name, strerror(errno));

                // Increment IPv6-specific failure count
                iface->v6_consecutive_failures++;
                log_this(SR_MDNS_SERVER, "IPv6 on %s has %d consecutive failures (limit: %d)", LOG_LEVEL_ALERT, 2, iface->if_name, iface->v6_consecutive_failures, retry_count);

                // Automatically disable IPv6 after configured consecutive failures
                if (iface->v6_consecutive_failures >= retry_count && iface->v6_disabled == 0) {
                    iface->v6_disabled = 1;
                    log_this(SR_MDNS_SERVER, "Automatically disabling IPv6 on %s after %d consecutive failures", LOG_LEVEL_ALERT, 2, iface->if_name, iface->v6_consecutive_failures);
                }
            } else {
                log_this(SR_MDNS_SERVER, "Sent IPv6 announcement on %s", LOG_LEVEL_STATE, 1, iface->if_name);
                v6_success = 1;

                // Reset IPv6 failure count on success
                iface->v6_consecutive_failures = 0;

                // Reactivate IPv6 if it was disabled and is now working
                if (iface->v6_disabled == 1) {
                    iface->v6_disabled = 0;
                    log_this(SR_MDNS_SERVER, "IPv6 on %s recovered from failures, re-enabled", LOG_LEVEL_STATE, 1, iface->if_name);
                }
            }
        } else if (iface->v6_disabled == 1) {
            // log_this(SR_MDNS_SERVER, "Skipping disabled IPv6 on interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
        }

        // Legacy interface-level failure tracking (for backward compatibility)
        // This maintains the existing logic but with protocol-level granularity
        if (v4_success == 0 && v6_success == 0) {
            // No announcements succeeded on this interface
            iface->consecutive_failures++;

            log_this(SR_MDNS_SERVER, "Interface %s has %d consecutive announcement failures (limit: %d)", LOG_LEVEL_ALERT, 3, iface->if_name, iface->consecutive_failures, retry_count);

            // Automatically disable interface after configured consecutive failures
            if (iface->consecutive_failures >= retry_count && iface->disabled == 0) {
                iface->disabled = 1;
                log_this(SR_MDNS_SERVER, "Automatically disabling interface %s after %d consecutive failures", LOG_LEVEL_ALERT, 2, iface->if_name, iface->consecutive_failures);
            }
        } else if (iface->disabled == 0) {
            // Reset interface-level failure count if either protocol succeeded
            iface->consecutive_failures = 0;
        }
    }
}
