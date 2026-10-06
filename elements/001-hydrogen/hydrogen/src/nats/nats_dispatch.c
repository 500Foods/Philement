/*
 * Parse one incoming envelope and dispatch.
 *
 * A peer cache.invalidate_by_ref drops result-cache rows for that SQL
 * template. An allowlisted event is offered to WebSocket sessions.
 * A peer app_state updates the presence registry when presence is on.
 * That envelope is not offered to the relay. Pointers passed to the
 * invalidate hook are valid only for that call. This file does not
 * register Lua or take nats_client_mu.
 */

#include <src/hydrogen.h>

#include <src/database/dbqueue/dbqueue.h>
#include <src/database/dbqueue/query_result_cache.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_subject.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

void nats_dispatch_invalidate(const char *database, json_int_t query_ref,
                              const char *reason);

static void (*nats_invalidate_hook)(const char *, json_int_t, const char *) =
    nats_dispatch_invalidate;

void nats_dispatch_set_invalidate(void (*hook)(const char *, json_int_t,
                                               const char *)) {
    nats_invalidate_hook = hook ? hook : nats_dispatch_invalidate;
}

void nats_invalidate_query_ref(const char *database, json_int_t query_ref) {
    if (!database || database[0] == '\0') {
        return;
    }
    if (query_ref < (json_int_t)INT_MIN || query_ref > (json_int_t)INT_MAX) {
        return;
    }
    if (!global_queue_manager) {
        return;
    }
    {
        DatabaseQueue *db_queue = database_queue_manager_get_database(
            global_queue_manager, database);

        if (!db_queue || !db_queue->query_cache) {
            return;
        }
        {
            QueryCacheEntry *entry = query_cache_lookup(db_queue->query_cache,
                                                        (int)query_ref, SR_NATS);

            if (!entry || !entry->sql_template) {
                return;
            }
            {
                char *sql = strdup(entry->sql_template);

                if (!sql) {
                    return;
                }
                {
                    QueryResultCache *results = query_result_cache_get_global();

                    if (results) {
                        query_result_cache_invalidate_template(results, database, sql);
                    }
                }
                free(sql);
            }
        }
    }
}

void nats_dispatch_invalidate(const char *database, json_int_t query_ref,
                              const char *reason) {
    (void)reason;
    /* Do not log the payload, the database name, query_ref, or reason. */
    nats_invalidate_query_ref(database, query_ref);
    log_this(SR_NATS, "NATS dispatch cache.invalidate_by_ref", LOG_LEVEL_TRACE, 0);
}

bool nats_dispatch_fields_ok(const json_t *root) {
    if (!json_is_object(root)) {
        return false;
    }
    {
        const json_t *event = json_object_get(root, "event");
        const json_t *timestamp = json_object_get(root, "timestamp");
        const json_t *source = json_object_get(root, "source");
        const json_t *instance = json_object_get(root, "instance_id");
        const json_t *subject = json_object_get(root, "subject");
        const char *event_name;
        const char *stamp;

        if (!json_is_string(event) || !json_is_string(timestamp) ||
            !json_is_string(source) || !json_is_string(instance) ||
            !json_is_string(subject)) {
            return false;
        }
        event_name = json_string_value(event);
        stamp = json_string_value(timestamp);
        if (!event_name || strcmp(event_name, "cache.invalidate_by_ref") != 0) {
            return false;
        }
        if (!stamp || stamp[0] == '\0') {
            return false;
        }
        return nats_broadcast_data_ok(json_object_get(root, "data"));
    }
}

bool nats_dispatch_subject_ok(const char *wire_subject, const json_t *root) {
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
    expected = nats_subject_build(app_config->nats.ClusterId, "cache.invalidate");
    if (!expected) {
        return false;
    }
    match = strcmp(wire_subject, expected) == 0;
    free(expected);
    return match;
}

bool nats_dispatch_is_self(const json_t *root) {
    const char *theirs;
    const char *ours;

    if (!app_config || !json_is_object(root)) {
        return false;
    }
    theirs = json_string_value(json_object_get(root, "instance_id"));
    if (!theirs) {
        return false;
    }
    ours = app_config->nats.InstanceId ? app_config->nats.InstanceId : "";
    return strcmp(ours, theirs) == 0;
}

void nats_dispatch_message(const char *wire_subject, const void *data, size_t len) {
    if (!app_config || !wire_subject || !data || len == 0) {
        log_this(SR_NATS, "NATS envelope dropped", LOG_LEVEL_TRACE, 0);
        return;
    }
    {
        json_t *root = json_loadb((const char *)data, len, 0, NULL);

        if (nats_dispatch_fields_ok(root) &&
            nats_dispatch_subject_ok(wire_subject, root)) {
            if (nats_dispatch_is_self(root)) {
                json_decref(root);
                log_this(SR_NATS, "NATS skip self", LOG_LEVEL_TRACE, 0);
                return;
            }
            {
                const json_t *body = json_object_get(root, "data");
                const char *database = json_string_value(json_object_get(body, "database"));
                const char *reason = json_string_value(json_object_get(body, "reason"));
                const char *subject = json_string_value(json_object_get(root, "subject"));
                json_int_t query_ref = json_integer_value(json_object_get(body, "query_ref"));

                if (nats_invalidate_hook) {
                    nats_invalidate_hook(database, query_ref, reason);
                }
                /* body is owned by root. Offer before json_decref. */
                nats_relay_offer("cache.invalidate_by_ref", subject, body);
            }
            json_decref(root);
            return;
        }
        if (nats_registry_envelope_ok(root) &&
            nats_registry_subject_ok(wire_subject, root)) {
            if (nats_dispatch_is_self(root)) {
                json_decref(root);
                log_this(SR_NATS, "NATS skip self", LOG_LEVEL_TRACE, 0);
                return;
            }
            if (app_config->nats.Presence.Enabled) {
                nats_registry_apply(root);
                log_this(SR_NATS, "NATS dispatch app_state", LOG_LEVEL_TRACE, 0);
            } else {
                log_this(SR_NATS, "NATS envelope dropped", LOG_LEVEL_TRACE, 0);
            }
            json_decref(root);
            return;
        }
        if (nats_relay_envelope_ok(root) &&
            nats_relay_subject_ok(wire_subject, root)) {
            if (nats_dispatch_is_self(root)) {
                json_decref(root);
                log_this(SR_NATS, "NATS skip self", LOG_LEVEL_TRACE, 0);
                return;
            }
            {
                const char *event_name = json_string_value(json_object_get(root, "event"));
                const char *subject = json_string_value(json_object_get(root, "subject"));
                const json_t *body = json_object_get(root, "data");

                nats_relay_offer(event_name, subject, body);
            }
            json_decref(root);
            return;
        }
        json_decref(root);
        log_this(SR_NATS, "NATS envelope dropped", LOG_LEVEL_TRACE, 0);
    }
}
