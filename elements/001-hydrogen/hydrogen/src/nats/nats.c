/*
 * NATS subsystem lifecycle.
 *
 * Phase 2 starts one reconnect thread. The first connect may fail.
 * Launch still succeeds, and the link stays degraded until CONNECT.
 */

#include <src/hydrogen.h>

#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>
#include <src/threads/threads.h>

static pthread_mutex_t nats_life_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_t nats_thread;
static bool nats_started = false;
static volatile int nats_link_state = NATS_LINK_DOWN;
static int (*nats_publish_fn)(const char *, const void *, size_t) = NULL;
static int (*nats_broadcast_fn)(const char *, const json_t *) = NULL;
static void (*nats_dispatch_fn)(const char *, const void *, size_t) = NULL;

void nats_link_set(int state) {
    nats_link_state = state;
}

int nats_link_get(void) {
    return nats_link_state;
}

const char *nats_link_name(int state) {
    if (state == NATS_LINK_UP) {
        return "up";
    }
    if (state == NATS_LINK_DEGRADED) {
        return "degraded";
    }
    return "down";
}

const char *nats_link_state_name(void) {
    return nats_link_name(nats_link_state);
}

void nats_on_msg(const char *subject, const char *sid, const char *reply,
                 const void *data, size_t len) {
    (void)sid;
    (void)reply;
    nats_stats_inc_received();
    log_this(SR_NATS, "NATS message on %s (%zu bytes)", LOG_LEVEL_TRACE, 2,
             subject ? subject : "", len);
    nats_dispatch_message(subject, data, len);
}

int nats_start(void) {
    pthread_t created;
    int rc;

    if (pthread_mutex_lock(&nats_life_mu) != 0) {
        return 0;
    }
    if (nats_started) {
        pthread_mutex_unlock(&nats_life_mu);
        return 1;
    }
    if (!app_config) {
        pthread_mutex_unlock(&nats_life_mu);
        return 0;
    }
    nats_system_shutdown = 0;
    nats_link_set(NATS_LINK_DEGRADED);
    nats_client_reset();
    nats_msg_set_handler(nats_on_msg);
    /* Addresses taken from live code so --gc-sections keeps them. */
    nats_publish_fn = nats_client_publish;
    nats_broadcast_fn = nats_broadcast;
    nats_dispatch_fn = nats_dispatch_message;
    nats_dispatch_set_invalidate(nats_dispatch_invalidate);
    nats_registry_bind();
    if (nats_publish_fn == NULL || nats_broadcast_fn == NULL ||
        nats_dispatch_fn == NULL) {
        nats_link_set(NATS_LINK_DOWN);
        pthread_mutex_unlock(&nats_life_mu);
        return 0;
    }
    nats_io_use_config();
    nats_stats_reset();
    rc = pthread_create(&created, NULL, nats_reconnect_thread, NULL);
    if (rc != 0) {
        nats_link_set(NATS_LINK_DOWN);
        pthread_mutex_unlock(&nats_life_mu);
        return 0;
    }
    nats_thread = created;
    nats_started = true;
    add_service_thread(&nats_threads, created);
    pthread_mutex_unlock(&nats_life_mu);
    log_this(SR_NATS, "NATS link %s", LOG_LEVEL_STATE, 1, nats_link_state_name());
    return 1;
}

void nats_shutdown(void) {
    pthread_t to_join;
    bool join = false;

    nats_reconnect_wake();
    if (pthread_mutex_lock(&nats_life_mu) != 0) {
        return;
    }
    if (nats_started) {
        to_join = nats_thread;
        nats_started = false;
        join = true;
    }
    if (join) {
        pthread_mutex_unlock(&nats_life_mu);
        pthread_join(to_join, NULL);
        if (pthread_mutex_lock(&nats_life_mu) != 0) {
            return;
        }
    }
    nats_io_close();
    nats_client_reset();
    nats_link_set(NATS_LINK_DOWN);
    init_service_threads(&nats_threads, SR_NATS);
    pthread_mutex_unlock(&nats_life_mu);
}
