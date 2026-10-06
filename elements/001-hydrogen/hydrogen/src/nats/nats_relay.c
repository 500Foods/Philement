/*
 * Decide which NATS event may reach a WebSocket client.
 *
 * The offer is shared. nats_broadcast calls it on the send path.
 * nats_dispatch_message calls it for a peer. Disabled relay returns
 * before any session list walk. This file does not dial, publish, or
 * take nats_client_mu.
 */

#include <src/hydrogen.h>

#include <src/config/config_nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_subject.h>
#include <src/websocket/websocket_server_relay.h>

#include <string.h>

_Static_assert(NATS_MAX_RELAY_EVENTS == 64, "relay event cap is 64");

bool nats_relay_event_allowed(const char *event) {
    if (!app_config || !event || event[0] == '\0' ||
        !app_config->nats.WebSocketRelay.Enabled) {
        return false;
    }
    {
        const NATSWebSocketRelayConfig *relay = &app_config->nats.WebSocketRelay;
        size_t n = relay->EventCount;
        size_t i;

        if (n > NATS_MAX_RELAY_EVENTS) {
            n = NATS_MAX_RELAY_EVENTS;
        }
        for (i = 0; i < n; i++) {
            if (relay->Events[i] && strcmp(relay->Events[i], event) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool nats_relay_envelope_ok(const json_t *root) {
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
        const char *event_name;
        const char *stamp;
        const char *subject_name;

        if (!json_is_string(event) || !json_is_string(timestamp) ||
            !json_is_string(source) || !json_is_string(instance) ||
            !json_is_string(subject) || !json_is_object(data)) {
            return false;
        }
        event_name = json_string_value(event);
        stamp = json_string_value(timestamp);
        subject_name = json_string_value(subject);
        if (!event_name || event_name[0] == '\0' || !stamp || stamp[0] == '\0' ||
            !subject_name || subject_name[0] == '\0') {
            return false;
        }
        return nats_relay_event_allowed(event_name);
    }
}

bool nats_relay_subject_ok(const char *wire_subject, const json_t *root) {
    if (!app_config || !wire_subject || !json_is_object(root)) {
        return false;
    }
    {
        const char *claimed = json_string_value(json_object_get(root, "subject"));
        const char *event_name = json_string_value(json_object_get(root, "event"));
        char *expected;
        bool match;

        if (!claimed || !event_name || strcmp(claimed, wire_subject) != 0) {
            return false;
        }
        expected = nats_subject_build(app_config->nats.ClusterId, event_name);
        if (!expected) {
            return false;
        }
        match = strcmp(wire_subject, expected) == 0;
        free(expected);
        return match;
    }
}

void nats_relay_offer(const char *event, const char *subject, const json_t *data) {
    /* event_allowed is false when the relay is disabled, so enqueue
     * never walks the session list on that path. */
    if (!nats_relay_event_allowed(event) || !subject || subject[0] == '\0' ||
        !json_is_object(data)) {
        return;
    }
    ws_relay_enqueue(event, subject, data);
}
