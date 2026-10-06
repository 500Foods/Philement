/*
 * NATS Subsystem Landing
 *
 * Lands before Print, while WebSocket and Database are still up.
 * Shutdown wakes the retry thread, joins it, and closes the socket.
 */

#include <src/hydrogen.h>

#include <src/nats/nats.h>
#include <src/registry/registry.h>

#include "landing.h"

LaunchReadiness check_nats_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    bool is_running;

    readiness.subsystem = SR_NATS;
    readiness.messages = malloc(5 * sizeof(char *));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }

    readiness.messages[0] = strdup(SR_NATS);
    is_running = is_subsystem_running_by_name(SR_NATS);
    readiness.ready = is_running;

    if (is_running) {
        readiness.messages[1] = strdup("  Go:      NATS subsystem is running");
        readiness.messages[2] = strdup("  Decide:  Go For Landing of NATS");
        readiness.messages[3] = NULL;
    } else {
        readiness.messages[1] = strdup("  No-Go:   NATS not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of NATS");
        readiness.messages[3] = NULL;
    }

    return readiness;
}

int land_nats_subsystem(void) {
    if (!is_subsystem_running_by_name(SR_NATS)) {
        log_this(SR_NATS, "NATS subsystem is not running, skipping landing", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    log_this(SR_NATS, "Landing NATS subsystem", LOG_LEVEL_STATE, 0);
    nats_shutdown();
    update_subsystem_after_shutdown(SR_NATS);
    log_this(SR_NATS, "NATS subsystem landed", LOG_LEVEL_STATE, 0);
    return 1;
}
