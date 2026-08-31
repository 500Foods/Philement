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
#include <time.h>

#include "mdns_keys.h"
#include "mdns_server.h"
#include <src/mdns/mdns_wire.h>

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

    log_this(SR_MDNS_SERVER, "mDNS Server announce loop started", LOG_LEVEL_DEBUG, 0);

    mdns_server_run_probe(mdns_server_instance);

    int initial_announcements = 3; // Initial burst
    unsigned int interval = 1; // Start with 1-second intervals

    while (!mdns_server_system_shutdown) {
        if (mdns_server_instance->probe_failed) {
            pthread_mutex_lock(&terminate_mutex);
            if (!mdns_server_system_shutdown) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += 1;
                pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
            }
            pthread_mutex_unlock(&terminate_mutex);
            continue;
        }
        if (initial_announcements > 0) {
            if (mdns_server_any_claimed(mdns_server_instance)) {
                mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
            }
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

            if (!mdns_server_system_shutdown && mdns_server_any_claimed(mdns_server_instance)) {
                mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);
            }
        }
    }

    log_this(SR_MDNS_SERVER, "Shutdown: mDNS Server announce loop exiting", LOG_LEVEL_DEBUG, 0);
    remove_service_thread(&mdns_server_threads, pthread_self());
    // Note: thread_arg is not freed here as it may be stack-allocated by the caller
    return NULL;
}

int mdns_server_run_probe(mdns_server_t *server)
{
    int attempt;
    int tries;
    int waited;

    if (!server) {
        return -1;
    }
    if (server->probe_tiebreak_lose) {
        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        log_this(SR_MDNS_SERVER, "MDNS_SERVER probe tiebreak lost, delaying 1s", LOG_LEVEL_DEBUG, 0);
        nanosleep(&ts, NULL);
        server->probe_tiebreak_lose = 0;
    }
    if (server->hostname_attempts < 1) {
        server->hostname_attempts = 1;
    }
    for (attempt = 0; attempt < MDNS_MAX_NAME_ATTEMPTS && !mdns_server_system_shutdown; attempt++) {
        if (mdns_server_all_claimed(server)) {
            return 0;
        }
        mdns_server_clear_probe_conflicts(server);
        for (tries = 0; tries < MDNS_PROBE_TRIES && !mdns_server_system_shutdown; tries++) {
            mdns_server_send_probe(server);
            waited = 0;
            while (waited < MDNS_PROBE_GAP_MS && !mdns_server_system_shutdown) {
                usleep(10000);
                waited += 10;
            }
            if (mdns_server_any_probe_conflict(server)) {
                break;
            }
        }
        if (mdns_server_system_shutdown) {
            return -1;
        }
        if (!mdns_server_any_probe_conflict(server)) {
            mdns_server_claim_unclaimed(server);
            return 0;
        }
        if (mdns_server_apply_probe_renames(server) < 0) {
            return -1;
        }
    }
    if (!mdns_server_all_claimed(server) && !mdns_server_system_shutdown) {
        mdns_server_probe_fail(server);
        return -1;
    }
    return 0;
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
    uint8_t buffer[MDNS_MSG_MAX];

    add_service_thread(&mdns_server_threads, pthread_self());
    log_this(SR_MDNS_SERVER, "mDNS Server responder loop started", LOG_LEVEL_DEBUG, 0);

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

                // Process the DNS query packet
                mdns_server_process_query_packet(mdns_server_instance, thread_arg->net_info, buffer, len,
                                                 fds[i].fd, &src_addr, (uint32_t)src_len);
            }
        }
    }

    log_this(SR_MDNS_SERVER, "Shutdown: mDNS Server responder loop exiting", LOG_LEVEL_DEBUG, 0);
    remove_service_thread(&mdns_server_threads, pthread_self());
    // Note: thread_arg is not freed here as it may be stack-allocated by the caller
    return NULL;
}