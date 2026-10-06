/*
 * One JSON envelope and one PUB.
 *
 * cache.invalidate_by_ref is published on the suffix cache.invalidate.
 * Matching result-cache rows are dropped before nats_client_publish.
 * The filtered body is offered to WebSocket sessions before the
 * envelope is released. nats_publish_event uses the same envelope
 * for every other event and does not evict. This file does not
 * parse a MSG or dial.
 */

#include <src/hydrogen.h>

#include <src/nats/nats_internal.h>
#include <src/nats/nats_subject.h>

#include <limits.h>
#include <string.h>
#include <time.h>

const char *nats_broadcast_suffix(const char *event) {
    if (!event || event[0] == '\0' || strncmp(event, "cluster.", 8) == 0) {
        return NULL;
    }
    if (strcmp(event, "cache.invalidate_by_ref") == 0) {
        return "cache.invalidate";
    }
    return NULL;
}

bool nats_broadcast_data_ok(const json_t *data) {
    const json_t *database;
    const json_t *query_ref;
    const json_t *reason;
    const char *name;

    if (!json_is_object(data)) {
        return false;
    }
    database = json_object_get(data, "database");
    query_ref = json_object_get(data, "query_ref");
    reason = json_object_get(data, "reason");
    if (!json_is_string(database) || !json_is_integer(query_ref) ||
        !json_is_string(reason)) {
        return false;
    }
    name = json_string_value(database);
    if (!name || name[0] == '\0') {
        return false;
    }
    {
        json_int_t ref = json_integer_value(query_ref);

        if (ref < (json_int_t)INT_MIN || ref > (json_int_t)INT_MAX) {
            return false;
        }
    }
    return true;
}

int nats_broadcast_timestamp(char *dst, size_t cap) {
    time_t now;
    struct tm utc;

    if (!dst || cap < 21) {
        return -1;
    }
    now = time(NULL);
    if (gmtime_r(&now, &utc) == NULL) {
        return -1;
    }
    if (strftime(dst, cap, "%Y-%m-%dT%H:%M:%SZ", &utc) == 0) {
        return -1;
    }
    return 0;
}

int nats_broadcast(const char *event, const json_t *data) {
    const NATSConfig *cfg;
    const char *suffix;
    const char *instance;
    const json_t *database;
    const json_t *query_ref;
    const json_t *reason;
    char *subject;
    json_t *envelope;
    json_t *body;
    char *payload;
    char timestamp[32];
    int rc;

    if (!app_config) {
        return -1;
    }
    cfg = &app_config->nats;
    suffix = nats_broadcast_suffix(event);
    if (!suffix || !nats_broadcast_data_ok(data)) {
        return -1;
    }
    /* An empty InstanceId was set on purpose. Do not call gethostname. */
    instance = cfg->InstanceId ? cfg->InstanceId : "";
    subject = nats_subject_build(cfg->ClusterId, suffix);
    if (!subject) {
        return -1;
    }
    if (nats_broadcast_timestamp(timestamp, sizeof(timestamp)) != 0) {
        free(subject);
        return -1;
    }
    database = json_object_get(data, "database");
    query_ref = json_object_get(data, "query_ref");
    reason = json_object_get(data, "reason");
    envelope = json_object();
    body = json_object();
    if (!envelope || !body) {
        json_decref(envelope);
        json_decref(body);
        free(subject);
        return -1;
    }
    json_object_set_new(body, "database", json_string(json_string_value(database)));
    json_object_set_new(body, "query_ref", json_integer(json_integer_value(query_ref)));
    json_object_set_new(body, "reason", json_string(json_string_value(reason)));
    json_object_set_new(envelope, "event", json_string(event));
    json_object_set_new(envelope, "subject", json_string(subject));
    json_object_set_new(envelope, "timestamp", json_string(timestamp));
    json_object_set_new(envelope, "source", json_string(instance));
    json_object_set_new(envelope, "instance_id", json_string(instance));
    json_object_set_new(envelope, "data", body);
    payload = json_dumps(envelope, JSON_COMPACT);
    if (!payload) {
        json_decref(envelope);
        free(subject);
        return -1;
    }
    /* body is owned by the envelope. Offer before json_decref. */
    nats_relay_offer(event, subject, body);
    json_decref(envelope);
    /* Local evict does not wait for a MSG. Do not log the payload. */
    nats_invalidate_query_ref(json_string_value(database),
                              json_integer_value(query_ref));
    rc = nats_client_publish(subject, payload, strlen(payload));
    free(payload);
    free(subject);
    return rc;
}

int nats_publish_event(const char *event, const json_t *data) {
    const NATSConfig *cfg;
    const char *instance;
    char *subject;
    json_t *envelope;
    json_t *body;
    char *payload;
    char timestamp[32];
    int rc;

    if (!app_config || !event || !json_is_object(data)) {
        return -1;
    }
    /* Presence and cache invalidation keep their own publishers. */
    if (strcmp(event, "app_state") == 0 ||
        strcmp(event, "cache.invalidate_by_ref") == 0) {
        return -1;
    }
    cfg = &app_config->nats;
    instance = cfg->InstanceId ? cfg->InstanceId : "";
    subject = nats_subject_build(cfg->ClusterId, event);
    if (!subject) {
        return -1;
    }
    if (nats_broadcast_timestamp(timestamp, sizeof(timestamp)) != 0) {
        free(subject);
        return -1;
    }
    envelope = json_object();
    body = json_deep_copy(data);
    if (!envelope || !body) {
        json_decref(envelope);
        json_decref(body);
        free(subject);
        return -1;
    }
    json_object_set_new(envelope, "event", json_string(event));
    json_object_set_new(envelope, "subject", json_string(subject));
    json_object_set_new(envelope, "timestamp", json_string(timestamp));
    json_object_set_new(envelope, "source", json_string(instance));
    json_object_set_new(envelope, "instance_id", json_string(instance));
    json_object_set_new(envelope, "data", body);
    payload = json_dumps(envelope, JSON_COMPACT);
    if (!payload) {
        json_decref(envelope);
        free(subject);
        return -1;
    }
    /* body is owned by the envelope. Offer before json_decref.
     * Do not evict and do not log the payload. */
    nats_relay_offer(event, subject, body);
    json_decref(envelope);
    rc = nats_client_publish(subject, payload, strlen(payload));
    free(payload);
    free(subject);
    return rc;
}
