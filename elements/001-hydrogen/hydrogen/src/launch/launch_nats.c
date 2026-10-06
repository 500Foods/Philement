/*
 * NATS Subsystem Launch
 *
 * Disabled and missing config are a clean skip. Enabled and valid
 * starts the retry thread and returns success even if the first
 * connect fails. Enabled and invalid is ready = false.
 */

#include <src/hydrogen.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/registry/registry_integration.h>
#include <src/threads/threads.h>

#include "launch.h"

extern ServiceThreads nats_threads;
extern volatile sig_atomic_t nats_system_shutdown;

int nats_subsystem_id = -1;

LaunchReadiness check_nats_launch_readiness(void) {
    const char **messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;
    const NATSConfig *nats;

    add_launch_message(&messages, &count, &capacity, strdup(SR_NATS));

    if (nats_subsystem_id < 0) {
        nats_subsystem_id = register_subsystem_from_launch(SR_NATS, &nats_threads, NULL,
                                                           &nats_system_shutdown,
                                                           launch_nats_subsystem, nats_shutdown);
    }

    if (!app_config) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NATS, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Configuration loaded"));

    nats = &app_config->nats;
    if (!nats->Enabled) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      NATS disabled, skipping validation"));
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Decide:  Go For Launch of NATS Subsystem"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NATS, .ready = true, .messages = messages };
    }

    if (nats_subsystem_id >= 0) {
        if (!add_dependency_from_launch(nats_subsystem_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  No-Go:   Failed to register Network dependency"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  Go:      Network dependency registered"));
        }
        if (!add_dependency_from_launch(nats_subsystem_id, SR_DATABASE)) {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  No-Go:   Failed to register Database dependency"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  Go:      Database dependency registered"));
        }
        if (nats->WebSocketRelay.Enabled) {
            if (!add_dependency_from_launch(nats_subsystem_id, SR_WEBSOCKET)) {
                add_launch_message(&messages, &count, &capacity,
                                   strdup("  No-Go:   Failed to register WebSocket dependency"));
                ready = false;
            } else {
                add_launch_message(&messages, &count, &capacity,
                                   strdup("  Go:      WebSocket dependency registered"));
            }
        }
    }

    if (!nats->LoadOk) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   NATS configuration is malformed"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      NATS configuration parsed"));
    }

    if (nats->TlsEnabled) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   NATS.TlsEnabled must be false in v1"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      NATS.TlsEnabled is false"));
    }

    if (nats->ServerCount == 0 || nats->ServerCount > NATS_MAX_SERVERS) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   NATS.Servers is required when enabled"));
        ready = false;
    } else {
        size_t i;
        bool urls_ok = true;
        for (i = 0; i < nats->ServerCount; i++) {
            if (!nats_server_url_ok(nats->Servers[i])) {
                urls_ok = false;
            }
        }
        if (!urls_ok) {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  No-Go:   NATS.Servers URL is invalid"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  Go:      NATS.Servers URLs are valid"));
        }
    }

    if (!nats_reconnect_delays_ok(nats)) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   NATS.Reconnect.Delays is invalid"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      NATS.Reconnect.Delays are valid"));
    }

    if (!nats_subscription_subjects_ok(nats)) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   NATS subject is invalid"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      NATS subjects are valid"));
    }

    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of NATS Subsystem" :
        "  Decide:  No-Go For Launch of NATS Subsystem"));
    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){ .subsystem = SR_NATS, .ready = ready, .messages = messages };
}

int launch_nats_subsystem(void) {
    log_this(SR_NATS, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_NATS, "LAUNCH: " SR_NATS, LOG_LEVEL_STATE, 0);

    if (!app_config) {
        log_this(SR_NATS, "Cannot launch: no application config", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    if (!app_config->nats.Enabled) {
        log_this(SR_NATS, "NATS subsystem is disabled, skipping launch", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    if (!nats_config_is_usable(&app_config->nats)) {
        log_this(SR_NATS, "Cannot launch: NATS configuration is invalid", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    if (nats_start() != 1) {
        log_this(SR_NATS, "Cannot launch: NATS retry thread did not start", LOG_LEVEL_ERROR, 0);
        return 0;
    }
    log_this(SR_NATS, "NATS subsystem launch accepted", LOG_LEVEL_STATE, 0);
    return 1;
}
