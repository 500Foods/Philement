/*
 * In-memory peer registry for instance presence.
 *
 * Presence publishes Starting, then Alive, on link up. Alive repeats
 * on the heartbeat. Link down publishes Stopping. Peers live in this
 * table. nats_registry_copy snapshots that table for Lua.
 * This file does not register Lua, serve HTTP, or take
 * nats_client_mu. The payload is not logged. Ids are not logged.
 */

#include <src/hydrogen.h>

#include <src/config/config_nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_subject.h>
#include <src/websocket/websocket_count.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATS_REGISTRY_ID_CAP 128

enum {
    NATS_SELF_NONE = 0,
    NATS_SELF_STARTING = 1,
    NATS_SELF_ALIVE = 2,
    NATS_SELF_STOPPING = 3
};

typedef struct NatsRegistryPeer {
    char id[NATS_REGISTRY_ID_CAP];
    int alive;
    time_t seen;
    time_t last_alive;
    int websocket_connections;
    bool used;
} NatsRegistryPeer;

_Static_assert(NATS_PRESENCE_SID == NATS_MAX_SUBSCRIPTIONS + 1,
               "presence sid stays outside the configured SUB table");
_Static_assert(NATS_REGISTRY_CAP == 64, "peer table holds 64 instances");
_Static_assert(NATS_REGISTRY_COPY_CAP == NATS_REGISTRY_CAP,
               "registry copy cap matches the peer table");
_Static_assert(sizeof(((NatsRegistryCopy *)0)->id) == NATS_REGISTRY_ID_CAP,
               "registry copy id fits the peer id");

static pthread_mutex_t nats_registry_mu = PTHREAD_MUTEX_INITIALIZER;
static NatsRegistryPeer nats_registry_peers[NATS_REGISTRY_CAP];
static int nats_registry_self = NATS_SELF_NONE;
static time_t nats_registry_alive_since;
static time_t nats_registry_last_beat;
static bool nats_registry_extra_sub;
static time_t (*nats_registry_clock_fn)(void);
static int (*nats_registry_count_fn)(void) = websocket_active_connection_count;

/* Caller holds nats_registry_mu. */
void nats_registry_peer_clear_at(size_t index) {
    NatsRegistryPeer *peer;

    if (index >= NATS_REGISTRY_CAP) {
        return;
    }
    peer = &nats_registry_peers[index];
    peer->used = false;
    peer->id[0] = '\0';
    peer->alive = 0;
    peer->seen = 0;
    peer->last_alive = 0;
    peer->websocket_connections = 0;
}

void nats_registry_set_clock(time_t (*clock_fn)(void)) {
    nats_registry_clock_fn = clock_fn;
}

void nats_registry_set_connection_count(int (*count_fn)(void)) {
    nats_registry_count_fn = count_fn ? count_fn : websocket_active_connection_count;
}

void nats_registry_reset(void) {
    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return;
    }
    {
        size_t i;

        for (i = 0; i < NATS_REGISTRY_CAP; i++) {
            nats_registry_peer_clear_at(i);
        }
    }
    nats_registry_self = NATS_SELF_NONE;
    nats_registry_alive_since = 0;
    nats_registry_last_beat = 0;
    nats_registry_extra_sub = false;
    pthread_mutex_unlock(&nats_registry_mu);
    nats_registry_clock_fn = NULL;
    nats_registry_count_fn = websocket_active_connection_count;
}

void nats_registry_bind(void) {
    nats_registry_set_clock(NULL);
    nats_registry_set_connection_count(NULL);
    (void)websocket_active_connection_count();
    (void)nats_registry_peer_count();
    (void)nats_registry_alive_count();
    (void)nats_registry_is_singleton();
    (void)nats_registry_self_state();
    (void)nats_registry_copy(NULL, 0);
}

int nats_registry_heartbeat_seconds(void) {
    int interval = 60;

    if (app_config) {
        interval = app_config->nats.Presence.HeartbeatIntervalSeconds;
    }
    if (interval < 1) {
        interval = 1;
    }
    return interval;
}

int nats_registry_stale_seconds(void) {
    long stale = 0;

    if (app_config) {
        stale = app_config->nats.Presence.StaleAfterSeconds;
    }
    if (stale <= 0) {
        stale = (long)nats_registry_heartbeat_seconds() * 3;
    }
    if (stale > INT_MAX) {
        return INT_MAX;
    }
    return (int)stale;
}

time_t nats_registry_now(void) {
    if (nats_registry_clock_fn) {
        return nats_registry_clock_fn();
    }
    return time(NULL);
}

int nats_registry_alive_count(void) {
    int count = 0;

    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return 0;
    }
    {
        size_t i;

        for (i = 0; i < NATS_REGISTRY_CAP; i++) {
            if (nats_registry_peers[i].used && nats_registry_peers[i].alive) {
                count++;
            }
        }
    }
    pthread_mutex_unlock(&nats_registry_mu);
    return count;
}

int nats_registry_peer_count(void) {
    int count = 0;

    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return 0;
    }
    {
        size_t i;

        for (i = 0; i < NATS_REGISTRY_CAP; i++) {
            if (nats_registry_peers[i].used) {
                count++;
            }
        }
    }
    pthread_mutex_unlock(&nats_registry_mu);
    return count;
}

const char *nats_registry_self_state(void) {
    int state = NATS_SELF_NONE;

    if (pthread_mutex_lock(&nats_registry_mu) == 0) {
        state = nats_registry_self;
        pthread_mutex_unlock(&nats_registry_mu);
    }
    if (state == NATS_SELF_STARTING) {
        return "Starting";
    }
    if (state == NATS_SELF_ALIVE) {
        return "Alive";
    }
    if (state == NATS_SELF_STOPPING) {
        return "Stopping";
    }
    return "none";
}

int nats_registry_copy(NatsRegistryCopy *out, size_t cap) {
    size_t n = 0;

    if (!out || cap == 0) {
        return 0;
    }
    if (cap > NATS_REGISTRY_CAP) {
        cap = NATS_REGISTRY_CAP;
    }
    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return 0;
    }
    {
        size_t i;

        for (i = 0; i < NATS_REGISTRY_CAP && n < cap; i++) {
            const NatsRegistryPeer *peer = &nats_registry_peers[i];

            if (!peer->used) {
                continue;
            }
            memcpy(out[n].id, peer->id, sizeof(out[n].id));
            snprintf(out[n].state, sizeof(out[n].state), "%s",
                     peer->alive ? "Alive" : "Starting");
            out[n].websocket_connections = peer->websocket_connections;
            n++;
        }
    }
    pthread_mutex_unlock(&nats_registry_mu);
    return (int)n;
}

bool nats_registry_is_singleton(void) {
    bool single = false;

    if (!app_config || !app_config->nats.Presence.Enabled) {
        return false;
    }
    if (nats_link_get() != NATS_LINK_UP) {
        return false;
    }
    {
        time_t now = nats_registry_now();
        int stale = nats_registry_stale_seconds();

        if (pthread_mutex_lock(&nats_registry_mu) != 0) {
            return false;
        }
        if (nats_registry_self == NATS_SELF_ALIVE &&
            nats_registry_alive_since != 0 &&
            now >= nats_registry_alive_since &&
            now - nats_registry_alive_since >= (time_t)stale) {
            size_t i;
            int alive = 0;

            for (i = 0; i < NATS_REGISTRY_CAP; i++) {
                if (nats_registry_peers[i].used && nats_registry_peers[i].alive) {
                    alive++;
                }
            }
            single = alive == 0;
        }
        pthread_mutex_unlock(&nats_registry_mu);
    }
    return single;
}

void nats_registry_sweep(void) {
    if (!app_config || !app_config->nats.Presence.Enabled) {
        return;
    }
    {
        time_t now = nats_registry_now();
        int stale = nats_registry_stale_seconds();

        if (pthread_mutex_lock(&nats_registry_mu) != 0) {
            return;
        }
        {
            size_t i;

            for (i = 0; i < NATS_REGISTRY_CAP; i++) {
                NatsRegistryPeer *peer = &nats_registry_peers[i];
                time_t mark;

                if (!peer->used) {
                    continue;
                }
                mark = peer->alive ? peer->last_alive : peer->seen;
                if (now >= mark && now - mark >= (time_t)stale) {
                    nats_registry_peer_clear_at(i);
                }
            }
        }
        pthread_mutex_unlock(&nats_registry_mu);
    }
}

bool nats_registry_envelope_ok(const json_t *root) {
    if (!json_is_object(root)) {
        return false;
    }
    {
        const json_t *event = json_object_get(root, "event");
        const json_t *timestamp = json_object_get(root, "timestamp");
        const json_t *source = json_object_get(root, "source");
        const json_t *instance = json_object_get(root, "instance_id");
        const json_t *subject = json_object_get(root, "subject");
        const json_t *data = json_object_get(root, "data");
        const json_t *state_json;
        const json_t *connections;
        const char *event_name;
        const char *stamp;
        const char *instance_id;
        const char *state_name;

        if (!json_is_string(event) || !json_is_string(timestamp) ||
            !json_is_string(source) || !json_is_string(instance) ||
            !json_is_string(subject) || !json_is_object(data)) {
            return false;
        }
        event_name = json_string_value(event);
        stamp = json_string_value(timestamp);
        instance_id = json_string_value(instance);
        state_json = json_object_get(data, "state");
        if (!event_name || strcmp(event_name, "app_state") != 0 ||
            !stamp || stamp[0] == '\0' || !instance_id ||
            strlen(instance_id) >= NATS_REGISTRY_ID_CAP ||
            !json_is_string(state_json)) {
            return false;
        }
        state_name = json_string_value(state_json);
        if (!state_name ||
            (strcmp(state_name, "Starting") != 0 &&
             strcmp(state_name, "Alive") != 0 &&
             strcmp(state_name, "Stopping") != 0)) {
            return false;
        }
        connections = json_object_get(data, "websocket_connections");
        if (connections) {
            json_int_t value;

            if (!json_is_integer(connections)) {
                return false;
            }
            value = json_integer_value(connections);
            if (value < 0 || value > (json_int_t)INT_MAX) {
                return false;
            }
        }
        return true;
    }
}

bool nats_registry_subject_ok(const char *wire_subject, const json_t *root) {
    const char *claimed;
    char *expected;
    bool match;

    if (!app_config || !wire_subject || !json_is_object(root)) {
        return false;
    }
    claimed = json_string_value(json_object_get(root, "subject"));
    if (!claimed || strcmp(claimed, wire_subject) != 0) {
        return false;
    }
    expected = nats_subject_build(app_config->nats.ClusterId, NATS_PRESENCE_SUFFIX);
    if (!expected) {
        return false;
    }
    match = strcmp(wire_subject, expected) == 0;
    free(expected);
    return match;
}

void nats_registry_apply(const json_t *root) {
    const char *id;
    const char *state;
    const json_t *data;
    const json_t *conn;
    int connections = 0;
    bool full = false;
    time_t now;

    if (!app_config || !app_config->nats.Presence.Enabled ||
        !nats_registry_envelope_ok(root)) {
        return;
    }
    id = json_string_value(json_object_get(root, "instance_id"));
    data = json_object_get(root, "data");
    state = json_string_value(json_object_get(data, "state"));
    conn = json_object_get(data, "websocket_connections");
    if (json_is_integer(conn)) {
        connections = (int)json_integer_value(conn);
    }
    if (!id || !state) {
        return;
    }
    now = nats_registry_now();
    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return;
    }
    {
        size_t i;
        int found = -1;
        int free_slot = -1;

        for (i = 0; i < NATS_REGISTRY_CAP; i++) {
            if (!nats_registry_peers[i].used) {
                if (free_slot < 0) {
                    free_slot = (int)i;
                }
                continue;
            }
            if (strcmp(nats_registry_peers[i].id, id) == 0) {
                found = (int)i;
                break;
            }
        }
        if (strcmp(state, "Stopping") == 0) {
            if (found >= 0) {
                nats_registry_peer_clear_at((size_t)found);
            }
        } else if (found < 0 && free_slot < 0) {
            full = true;
        } else {
            NatsRegistryPeer *peer = &nats_registry_peers[found >= 0 ? found : free_slot];

            if (found < 0) {
                snprintf(peer->id, sizeof(peer->id), "%s", id);
                peer->used = true;
            }
            peer->seen = now;
            if (strcmp(state, "Alive") == 0) {
                peer->alive = 1;
                peer->last_alive = now;
                peer->websocket_connections = connections;
            } else {
                peer->alive = 0;
                peer->websocket_connections = 0;
            }
        }
    }
    pthread_mutex_unlock(&nats_registry_mu);
    if (full) {
        log_this(SR_NATS, "NATS registry full", LOG_LEVEL_TRACE, 0);
    }
}

int nats_registry_publish(const char *state) {
    const NATSConfig *cfg;
    const char *instance;
    char *subject;
    json_t *envelope;
    json_t *body;
    char *payload;
    char timestamp[32];
    int connections = 0;
    bool report;
    time_t now;
    int rc;

    if (!state || !app_config) {
        return -1;
    }
    if (!app_config->nats.Presence.Enabled) {
        return 0;
    }
    if (strcmp(state, "Starting") != 0 && strcmp(state, "Alive") != 0 &&
        strcmp(state, "Stopping") != 0) {
        return -1;
    }
    cfg = &app_config->nats;
    instance = cfg->InstanceId ? cfg->InstanceId : "";
    report = strcmp(state, "Alive") == 0 && cfg->Presence.ReportConnections;
    if (report && nats_registry_count_fn) {
        connections = nats_registry_count_fn();
        if (connections < 0) {
            connections = 0;
        }
    }
    subject = nats_subject_build(cfg->ClusterId, NATS_PRESENCE_SUFFIX);
    if (!subject) {
        return -1;
    }
    if (nats_broadcast_timestamp(timestamp, sizeof(timestamp)) != 0) {
        free(subject);
        return -1;
    }
    envelope = json_object();
    body = json_object();
    if (!envelope || !body) {
        json_decref(envelope);
        json_decref(body);
        free(subject);
        return -1;
    }
    json_object_set_new(body, "state", json_string(state));
    if (report) {
        json_object_set_new(body, "websocket_connections",
                            json_integer((json_int_t)connections));
    }
    json_object_set_new(envelope, "event", json_string("app_state"));
    json_object_set_new(envelope, "subject", json_string(subject));
    json_object_set_new(envelope, "timestamp", json_string(timestamp));
    json_object_set_new(envelope, "source", json_string(instance));
    json_object_set_new(envelope, "instance_id", json_string(instance));
    json_object_set_new(envelope, "data", body);
    payload = json_dumps(envelope, JSON_COMPACT);
    json_decref(envelope);
    if (!payload) {
        free(subject);
        return -1;
    }
    now = nats_registry_now();
    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        free(payload);
        free(subject);
        return -1;
    }
    if (strcmp(state, "Starting") == 0) {
        nats_registry_self = NATS_SELF_STARTING;
        nats_registry_alive_since = 0;
    } else if (strcmp(state, "Alive") == 0) {
        nats_registry_self = NATS_SELF_ALIVE;
        if (nats_registry_alive_since == 0) {
            nats_registry_alive_since = now;
        }
        nats_registry_last_beat = now;
    } else {
        nats_registry_self = NATS_SELF_STOPPING;
        nats_registry_alive_since = 0;
        nats_registry_last_beat = 0;
    }
    pthread_mutex_unlock(&nats_registry_mu);
    rc = nats_client_publish(subject, payload, strlen(payload));
    free(payload);
    free(subject);
    if (rc == 0) {
        /* Event name only. Do not log the payload. */
        log_this(SR_NATS, "NATS app_state", LOG_LEVEL_TRACE, 0);
    }
    return rc;
}

int nats_registry_subscribe(void) {
    const NATSConfig *cfg;
    bool listed = false;

    if (!app_config || !app_config->nats.Presence.Enabled) {
        return 0;
    }
    cfg = &app_config->nats;
    {
        size_t i;

        for (i = 0; i < cfg->SubscriptionCount && i < NATS_MAX_SUBSCRIPTIONS; i++) {
            if (cfg->Subscriptions[i].Subject &&
                strcmp(cfg->Subscriptions[i].Subject, NATS_PRESENCE_SUFFIX) == 0) {
                listed = true;
                break;
            }
        }
    }
    if (listed) {
        if (pthread_mutex_lock(&nats_registry_mu) == 0) {
            nats_registry_extra_sub = false;
            pthread_mutex_unlock(&nats_registry_mu);
        }
        return 0;
    }
    {
        char *subject = nats_subject_build(cfg->ClusterId, NATS_PRESENCE_SUFFIX);
        char line[NATS_CTRL_CAP];
        int written;

        if (!subject) {
            return -1;
        }
        written = nats_frame_sub(line, sizeof(line), subject, NULL, NATS_PRESENCE_SID);
        free(subject);
        if (written < 0 || nats_io_write(line, (size_t)written) != 0) {
            return -1;
        }
    }
    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return -1;
    }
    nats_registry_extra_sub = true;
    pthread_mutex_unlock(&nats_registry_mu);
    return 0;
}

int nats_registry_announce_up(void) {
    int rc;

    if (!app_config || !app_config->nats.Presence.Enabled) {
        return 0;
    }
    nats_registry_sweep();
    rc = nats_registry_publish("Starting");
    if (rc != 0) {
        return rc;
    }
    return nats_registry_publish("Alive");
}

void nats_registry_on_link_down(void) {
    bool extra = false;

    if (!app_config || !app_config->nats.Presence.Enabled) {
        return;
    }
    (void)nats_registry_publish("Stopping");
    (void)nats_client_flush_outbound();
    if (pthread_mutex_lock(&nats_registry_mu) == 0) {
        extra = nats_registry_extra_sub;
        nats_registry_extra_sub = false;
        pthread_mutex_unlock(&nats_registry_mu);
    }
    if (extra) {
        char line[64];
        int written = nats_frame_unsub(line, sizeof(line), NATS_PRESENCE_SID);

        if (written > 0) {
            (void)nats_io_write(line, (size_t)written);
        }
    }
}

int nats_registry_tick(void) {
    time_t now;
    int interval;
    bool due = false;

    if (!app_config || !app_config->nats.Presence.Enabled) {
        return 0;
    }
    if (nats_link_get() != NATS_LINK_UP) {
        return 0;
    }
    interval = nats_registry_heartbeat_seconds();
    now = nats_registry_now();
    if (pthread_mutex_lock(&nats_registry_mu) != 0) {
        return -1;
    }
    if (nats_registry_last_beat == 0 || now < nats_registry_last_beat ||
        now - nats_registry_last_beat >= (time_t)interval) {
        due = true;
    }
    pthread_mutex_unlock(&nats_registry_mu);
    if (!due) {
        return 0;
    }
    nats_registry_sweep();
    return nats_registry_publish("Alive");
}
