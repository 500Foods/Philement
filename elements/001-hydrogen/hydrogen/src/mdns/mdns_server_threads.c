/*
 * mDNS Server Thread Management for the Hydrogen printer.
 *
 * Contains thread management functionality split from the main mDNS server
 * implementation. This includes the announce and responder thread loops.
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "mdns_keys.h"
#include "mdns_server.h"
#include "mdns_dns_utils.h"

// Include mock headers for Unity testing
#ifdef USE_MOCK_THREADS
#include <unity/mocks/mock_threads.h>
#endif

extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

void mdns_server_send_announcement(mdns_server_t *mdns_server_instance, const network_info_t *net_info_instance);

void *mdns_server_announce_loop(void *arg) {
    // Handle NULL argument gracefully
    // cppcheck-suppress knownConditionTrueFalse
    if (!arg) {
        log_this(SR_MDNS_SERVER, "mDNS Server announce loop called with NULL argument", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    mdns_server_thread_arg_t *thread_arg = (mdns_server_thread_arg_t *)arg;

    // Handle NULL thread_arg or mdns_server gracefully
    // cppcheck-suppress knownConditionTrueFalse
    if (!thread_arg || !thread_arg->mdns_server) {
        log_this(SR_MDNS_SERVER, "mDNS Server announce loop called with NULL thread_arg or mdns_server", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

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
    // Note: thread_arg is not freed here as it may be stack-allocated by the caller
    return NULL;
}

void *mdns_server_responder_loop(void *arg) {
    // Handle NULL argument gracefully
    // cppcheck-suppress knownConditionTrueFalse
    if (!arg) {
        log_this(SR_MDNS_SERVER, "mDNS Server responder loop called with NULL argument", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    mdns_server_thread_arg_t *thread_arg = (mdns_server_thread_arg_t *)arg;

    // Handle NULL thread_arg or mdns_server gracefully
    // cppcheck-suppress knownConditionTrueFalse
    if (!thread_arg || !thread_arg->mdns_server) {
        log_this(SR_MDNS_SERVER, "mDNS Server responder loop called with NULL thread_arg or mdns_server", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

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
        // Note: thread_arg is not freed here as it may be stack-allocated by the caller
        return NULL;
    }

    nfds_t nfds = 0;  // Use nfds_t type to avoid sign conversion

    // Check if interfaces array is properly initialized
    if (!mdns_server_instance->interfaces) {
        log_this(SR_MDNS_SERVER, "mDNS Server interfaces array is NULL", LOG_LEVEL_DEBUG, 0);
        free(fds);
        remove_service_thread(&mdns_server_threads, pthread_self());
        return NULL;
    }

    for (size_t i = 0; i < mdns_server_instance->num_interfaces; i++) {
        const mdns_server_interface_t *iface = &mdns_server_instance->interfaces[i];
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
        // Note: thread_arg is not freed here as it may be stack-allocated by the caller
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
    // Note: thread_arg is not freed here as it may be stack-allocated by the caller
    return NULL;
}