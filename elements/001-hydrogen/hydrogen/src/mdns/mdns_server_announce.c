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
#include <src/mdns/mdns_wire.h>

extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

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
        memset(packet, 0, sizeof(dns_header_t));
        *packet_len = sizeof(dns_header_t);
        return;
    }

    {
        mdns_buf b;
        size_t an_pos;
        uint16_t ancount = 0;
        uint32_t host_ttl = (ttl == 0) ? 0 : MDNS_TTL_HOST;
        uint32_t shared_ttl = (ttl == 0) ? 0 : MDNS_TTL_SHARED;
        size_t i;

        mdns_buf_init(&b, packet, MDNS_MAX_PACKET_SIZE);
        if (mdns_put_u16(&b, 0) < 0) {
            *packet_len = 0;
            return;
        }
        if (mdns_put_u16(&b, DNS_FLAG_RESPONSE) < 0) {
            *packet_len = 0;
            return;
        }
        if (mdns_put_u16(&b, 0) < 0) {
            *packet_len = 0;
            return;
        }
        an_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            *packet_len = 0;
            return;
        }
        if (mdns_put_u16(&b, 0) < 0) {
            *packet_len = 0;
            return;
        }
        if (mdns_put_u16(&b, 0) < 0) {
            *packet_len = 0;
            return;
        }

        for (i = 0; i < iface->num_addresses; i++) {
            struct in_addr addr;
            struct in6_addr addr6;
            size_t pos;

            if (ttl != 0 && mdns_server_instance && !mdns_server_instance->hostname_claimed) {
                break;
            }
            if (!iface->ip_addresses || !iface->ip_addresses[i]) {
                continue;
            }
            if (inet_pton(AF_INET, iface->ip_addresses[i], &addr) == 1) {
                if (mdns_rr_head(&b, hostname, MDNS_TYPE_A, host_ttl, 1, &pos) < 0) {
                    break;
                }
                if (mdns_put_bytes(&b, &addr.s_addr, 4) < 0) {
                    break;
                }
                if (mdns_rr_tail(&b, pos) < 0) {
                    break;
                }
                ancount++;
            } else if (inet_pton(AF_INET6, iface->ip_addresses[i], &addr6) == 1) {
                if (mdns_rr_head(&b, hostname, MDNS_TYPE_AAAA, host_ttl, 1, &pos) < 0) {
                    break;
                }
                if (mdns_put_bytes(&b, &addr6.s6_addr, 16) < 0) {
                    break;
                }
                if (mdns_rr_tail(&b, pos) < 0) {
                    break;
                }
                ancount++;
            }
        }

        if (mdns_server_instance) {
            for (i = 0; i < mdns_server_instance->num_services; i++) {
                const char *ptr_owner_name;
                if (ttl != 0 && !mdns_server_instance->services[i].claimed) {
                    continue;
                }
                ptr_owner_name = mdns_server_instance->services[i].type;
                const size_t max_name_len = 100;
                const size_t max_type_len = 100;
                size_t name_len = strlen(mdns_server_instance->services[i].name);
                size_t type_len = strlen(mdns_server_instance->services[i].type);
                size_t total_len = name_len + 1 + type_len + 6;
                char full_service_instance_name[256];
                size_t pos;
                size_t t;

                if (total_len >= 256) {
                    log_this(SR_MDNS_SERVER, "Service name too long: %s.%s truncated", LOG_LEVEL_ALERT, 2,
                             mdns_server_instance->services[i].name, mdns_server_instance->services[i].type);
                    name_len = max_name_len < name_len ? max_name_len : name_len;
                    type_len = max_type_len < type_len ? max_type_len : type_len;
                }

                snprintf(full_service_instance_name, sizeof(full_service_instance_name), "%.*s.%.*s",
                         (int)name_len, mdns_server_instance->services[i].name, (int)type_len, mdns_server_instance->services[i].type);

                if (mdns_rr_head(&b, ptr_owner_name, MDNS_TYPE_PTR, shared_ttl, 0, &pos) < 0) {
                    break;
                }
                if (mdns_put_name(&b, full_service_instance_name) < 0) {
                    break;
                }
                if (mdns_rr_tail(&b, pos) < 0) {
                    break;
                }
                ancount++;

                if (mdns_rr_head(&b, full_service_instance_name, MDNS_TYPE_SRV, host_ttl, 1, &pos) < 0) {
                    break;
                }
                if (mdns_put_u16(&b, 0) < 0) {
                    break;
                }
                if (mdns_put_u16(&b, 0) < 0) {
                    break;
                }
                if (mdns_put_u16(&b, (uint16_t)mdns_server_instance->services[i].port) < 0) {
                    break;
                }
                if (mdns_put_name(&b, hostname) < 0) {
                    break;
                }
                if (mdns_rr_tail(&b, pos) < 0) {
                    break;
                }
                ancount++;

                if (mdns_rr_head(&b, full_service_instance_name, MDNS_TYPE_TXT, shared_ttl, 1, &pos) < 0) {
                    break;
                }
                for (t = 0; t < mdns_server_instance->services[i].num_txt_records; t++) {
                    const char *txt = mdns_server_instance->services[i].txt_records[t];
                    size_t txt_len;

                    if (!txt) {
                        continue;
                    }
                    txt_len = strlen(txt);
                    if (txt_len > 255) {
                        txt_len = 255;
                    }
                    if (mdns_put_u8(&b, (uint8_t)txt_len) < 0) {
                        break;
                    }
                    if (mdns_put_bytes(&b, txt, txt_len) < 0) {
                        break;
                    }
                }
                if (mdns_rr_tail(&b, pos) < 0) {
                    break;
                }
                ancount++;
            }
        }

        if (b.overflow) {
            log_this(SR_MDNS_SERVER, "Announcement packet overflow, skipping send", LOG_LEVEL_ALERT, 0);
            *packet_len = 0;
            return;
        }
        b.buf[an_pos] = (uint8_t)(ancount >> 8);
        b.buf[an_pos + 1] = (uint8_t)(ancount & 0xffu);
        *packet_len = b.len;
        if (*packet_len > 1500) {
            log_this(SR_MDNS_SERVER, "Warning: Packet size %zu exceeds typical MTU (1500)", LOG_LEVEL_ALERT, 1, *packet_len);
        }
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

    if (!mdns_server_instance || mdns_server_instance->probe_failed || !mdns_server_any_claimed(mdns_server_instance)) {
        return;
    }

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
        mdns_server_build_announcement(packet, &packet_len, mdns_server_instance->hostname, mdns_server_instance, MDNS_TTL_HOST, iface_net_info);
        free_single_interface_net_info(iface_net_info);

        if (packet_len == 0) {
            log_this(SR_MDNS_SERVER, "Skipping send on %s: empty or overflow announcement", LOG_LEVEL_ALERT, 1, iface->if_name);
            continue;
        }

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
            log_this(SR_MDNS_SERVER, "DEBUG: IPv4 announcement to mDNS group on %s, packet size %zu bytes", LOG_LEVEL_DEBUG, 2, iface->if_name, packet_len);
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
                log_this(SR_MDNS_SERVER, "Sent IPv4 announcement on %s", LOG_LEVEL_TRACE, 1, iface->if_name);
                v4_success = 1;

                // Reset IPv4 failure count on success
                iface->v4_consecutive_failures = 0;

                // Reactivate IPv4 if it was disabled and is now working
                if (iface->v4_disabled == 1) {
                    iface->v4_disabled = 0;
                    log_this(SR_MDNS_SERVER, "IPv4 on %s recovered from failures, re-enabled", LOG_LEVEL_DEBUG, 1, iface->if_name);
                }
            }
        } else if (iface->v4_disabled == 1) {
            // log_this(SR_MDNS_SERVER, "Skipping disabled IPv4 on interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
        }

        // Send IPv6 announcement (if not protocol-specifically disabled)
        if (iface->sockfd_v6 >= 0 && iface->v6_disabled == 0) {
            if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send IPv6 announcement on %s: %s", LOG_LEVEL_TRACE, 2, iface->if_name, strerror(errno));

                // Increment IPv6-specific failure count
                iface->v6_consecutive_failures++;
                log_this(SR_MDNS_SERVER, "IPv6 on %s has %d consecutive failures (limit: %d)", LOG_LEVEL_TRACE, 2, iface->if_name, iface->v6_consecutive_failures, retry_count);

                // Automatically disable IPv6 after configured consecutive failures
                if (iface->v6_consecutive_failures >= retry_count && iface->v6_disabled == 0) {
                    iface->v6_disabled = 1;
                    log_this(SR_MDNS_SERVER, "Automatically disabling IPv6 on %s after %d consecutive failures", LOG_LEVEL_ALERT, 2, iface->if_name, iface->v6_consecutive_failures);
                }
            } else {
                log_this(SR_MDNS_SERVER, "Sent IPv6 announcement on %s", LOG_LEVEL_TRACE, 1, iface->if_name);
                v6_success = 1;

                // Reset IPv6 failure count on success
                iface->v6_consecutive_failures = 0;

                // Reactivate IPv6 if it was disabled and is now working
                if (iface->v6_disabled == 1) {
                    iface->v6_disabled = 0;
                    log_this(SR_MDNS_SERVER, "IPv6 on %s recovered from failures, re-enabled", LOG_LEVEL_DEBUG, 1, iface->if_name);
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
                log_this(SR_MDNS_SERVER, "Automatically disabling interface %s after %d consecutive failures", LOG_LEVEL_DEBUG, 2, iface->if_name, iface->consecutive_failures);
            }
        } else if (iface->disabled == 0) {
            // Reset interface-level failure count if either protocol succeeded
            iface->consecutive_failures = 0;
        }
    }
}
