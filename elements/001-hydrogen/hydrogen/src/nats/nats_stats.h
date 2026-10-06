/*
 * NATS counters and the status snapshot.
 *
 * published, received, and reconnects are process-lifetime atomics.
 * nats_start zeroes them. Shutdown does not. The HTTP status route
 * and SystemMetrics both read nats_stats_collect.
 */

#ifndef NATS_STATS_H
#define NATS_STATS_H

#include <stdbool.h>

typedef struct NatsMetrics {
    bool enabled;
    bool presence;
    bool singleton;
    int link;
    int peers;
    int alive;
    unsigned long long published;
    unsigned long long received;
    unsigned long long reconnects;
    const char *self;
} NatsMetrics;

void nats_stats_reset(void);
void nats_stats_inc_published(void);
void nats_stats_inc_received(void);
void nats_stats_inc_reconnects(void);
void nats_stats_collect(NatsMetrics *metrics);

/* down when disabled. up when the link is up. degraded otherwise. */
const char *nats_status_state(bool enabled, int link);

#endif /* NATS_STATS_H */
