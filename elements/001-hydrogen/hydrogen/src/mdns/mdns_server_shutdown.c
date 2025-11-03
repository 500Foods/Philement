/*
 * mDNS Server Shutdown for the Hydrogen printer.
 *
 * Contains shutdown functionality split from the main mDNS server
 * implementation. This includes cleanup and goodbye packet sending.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mdns_server.h"

extern volatile sig_atomic_t mdns_server_system_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

void mdns_server_build_announcement(uint8_t *packet, size_t *packet_len, const char *hostname,
                           const mdns_server_t *mdns_server_instance, uint32_t ttl, const network_info_t *net_info_instance);

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
            log_this(SR_MDNS_SERVER, "Closing IPv4 socket on interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
            close(iface->sockfd_v4);
            iface->sockfd_v4 = -1;
        }

        if (iface->sockfd_v6 >= 0) {
            log_this(SR_MDNS_SERVER, "Closing IPv6 socket on interface %s", LOG_LEVEL_DEBUG, 1, iface->if_name);
            close(iface->sockfd_v6);
            iface->sockfd_v6 = -1;
        }
    }
}

void mdns_server_shutdown(mdns_server_t *mdns_server_instance) {
    if (!mdns_server_instance) return;

    log_this(SR_MDNS_SERVER, "Shutdown: Initiating mDNS Server shutdown", LOG_LEVEL_DEBUG, 0);

    // Wait for any active threads to notice the shutdown flag
    // This ensures they don't access mdns_server after we free it
    update_service_thread_metrics(&mdns_server_threads);

    // Check for active threads before proceeding
    if (mdns_server_threads.thread_count > 0) {
        log_this(SR_MDNS_SERVER, "Waiting for %d mDNS Server threads to exit", LOG_LEVEL_DEBUG, 1, mdns_server_threads.thread_count);

        // Wait with timeout for threads to exit
        for (int i = 0; i < 10 && mdns_server_threads.thread_count > 0; i++) {
            usleep(10000);  // 200ms between checks
            update_service_thread_metrics(&mdns_server_threads);
        }

        if (mdns_server_threads.thread_count > 0) {
            log_this(SR_MDNS_SERVER, "Warning: %d mDNS Server threads still active", LOG_LEVEL_DEBUG, 1, mdns_server_threads.thread_count);
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
                        log_this(SR_MDNS_SERVER, "Sent IPv4 goodbye packet %d/3 on %s", LOG_LEVEL_DEBUG, 2, i+1, iface->if_name);
                    }
                }
                if (iface->sockfd_v6 >= 0) {
                    if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof(addr_v6)) < 0) {
                        log_this(SR_MDNS_SERVER, "Failed to send IPv6 goodbye on %s: %s", LOG_LEVEL_ALERT, 2, iface->if_name, strerror(errno));
                    } else {
                        log_this(SR_MDNS_SERVER, "Sent IPv6 goodbye packet %d/3 on %s", LOG_LEVEL_DEBUG, 2, i+1, iface->if_name);
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
    log_this(SR_MDNS_SERVER, "Closing mDNS Server sockets", LOG_LEVEL_DEBUG, 0);
    close_mdns_server_interfaces(mdns_server_instance);

    // Final check for active threads
    update_service_thread_metrics(&mdns_server_threads);
    if (mdns_server_threads.thread_count > 0) {
        log_this(SR_MDNS_SERVER, "Warning: Proceeding with cleanup with %d threads still active", LOG_LEVEL_ALERT, 1, mdns_server_threads.thread_count);
    }

    // Brief delay to ensure no threads are accessing resources
    usleep(100000);  // 200ms delay
    log_this(SR_MDNS_SERVER, "Freeing mDNS Server resources", LOG_LEVEL_DEBUG, 0);

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

    log_this(SR_MDNS_SERVER, "Shutdown: mDNS Server shutdown complete", LOG_LEVEL_DEBUG, 0);
}