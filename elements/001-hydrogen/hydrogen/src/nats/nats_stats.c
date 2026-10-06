/*
 * NATS counters and the status snapshot.
 *
 * Storage stays here. nats_client.c only calls the incrementers.
 * A queued publish does not count. A handshake that never reached
 * up does not count as a reconnect. Shutdown does not count.
 */

#include <src/hydrogen.h>

#include <string.h>

#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>

static unsigned long long nats_stat_published = 0;
static unsigned long long nats_stat_received = 0;
static unsigned long long nats_stat_reconnects = 0;

void nats_stats_reset(void) {
    __atomic_store_n(&nats_stat_published, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&nats_stat_received, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&nats_stat_reconnects, 0, __ATOMIC_RELAXED);
}

void nats_stats_inc_published(void) {
    __atomic_fetch_add(&nats_stat_published, 1, __ATOMIC_RELAXED);
}

void nats_stats_inc_received(void) {
    __atomic_fetch_add(&nats_stat_received, 1, __ATOMIC_RELAXED);
}

void nats_stats_inc_reconnects(void) {
    __atomic_fetch_add(&nats_stat_reconnects, 1, __ATOMIC_RELAXED);
}

const char *nats_status_state(bool enabled, int link) {
    if (!enabled) {
        return "down";
    }
    if (link == NATS_LINK_UP) {
        return "up";
    }
    return "degraded";
}

void nats_stats_collect(NatsMetrics *metrics) {
    const char *self;

    if (!metrics) {
        return;
    }
    memset(metrics, 0, sizeof(*metrics));
    metrics->enabled = app_config && app_config->nats.Enabled;
    metrics->presence = app_config && app_config->nats.Presence.Enabled;
    metrics->link = nats_link_get();
    metrics->published = __atomic_load_n(&nats_stat_published, __ATOMIC_RELAXED);
    metrics->received = __atomic_load_n(&nats_stat_received, __ATOMIC_RELAXED);
    metrics->reconnects = __atomic_load_n(&nats_stat_reconnects, __ATOMIC_RELAXED);
    metrics->peers = nats_registry_peer_count();
    metrics->alive = nats_registry_alive_count();
    metrics->singleton = nats_registry_is_singleton();
    self = nats_registry_self_state();
    metrics->self = self ? self : "none";
}
