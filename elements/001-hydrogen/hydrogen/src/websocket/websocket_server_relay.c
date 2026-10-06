/*
 * Session list for the NATS WebSocket relay.
 *
 * Connect appends, close removes, and the mutex covers both. The NATS
 * thread only enqueues. LWS_CALLBACK_SERVER_WRITEABLE calls lws_write.
 */

#include <src/hydrogen.h>

#include <src/nats/nats_internal.h>
#include <src/websocket/websocket_server_relay.h>

#include <stdlib.h>
#include <string.h>

#include "websocket_server.h"
#include "websocket_server_message.h"

typedef struct WsRelayNode {
    struct lws *wsi;
    WebSocketSessionData *session;
    struct WsRelayNode *next;
} WsRelayNode;

static WsRelayNode *ws_relay_head = NULL;
static pthread_mutex_t ws_relay_mu = PTHREAD_MUTEX_INITIALIZER;

void ws_relay_free_names(char **names, size_t count) {
    size_t i;

    if (!names) {
        return;
    }
    for (i = 0; i < count && i < NATS_MAX_RELAY_EVENTS; i++) {
        free(names[i]);
        names[i] = NULL;
    }
}

void ws_relay_session_free_events(WebSocketSessionData *session) {
    size_t i;

    if (!session) {
        return;
    }
    for (i = 0; i < NATS_MAX_RELAY_EVENTS; i++) {
        free(session->subscribed_events[i]);
        session->subscribed_events[i] = NULL;
    }
    session->subscribed_event_count = 0;
}

void ws_relay_session_clear(WebSocketSessionData *session) {
    size_t i;

    if (!session) {
        return;
    }
    ws_relay_session_free_events(session);
    for (i = 0; i < WS_RELAY_QUEUE_DEPTH; i++) {
        free(session->relay_queue[i]);
        session->relay_queue[i] = NULL;
    }
    session->relay_queue_count = 0;
}

void ws_relay_session_add(struct lws *wsi, WebSocketSessionData *session) {
    WsRelayNode *node;

    if (!wsi || !session) {
        return;
    }
    node = calloc(1, sizeof(*node));
    if (!node) {
        return;
    }
    node->wsi = wsi;
    node->session = session;
    if (pthread_mutex_lock(&ws_relay_mu) != 0) {
        free(node);
        return;
    }
    node->next = ws_relay_head;
    ws_relay_head = node;
    pthread_mutex_unlock(&ws_relay_mu);
}

void ws_relay_session_remove(WebSocketSessionData *session) {
    WsRelayNode *prev = NULL;
    WsRelayNode *node;
    WsRelayNode *doomed = NULL;

    if (!session) {
        return;
    }
    if (pthread_mutex_lock(&ws_relay_mu) != 0) {
        return;
    }
    node = ws_relay_head;
    while (node) {
        if (node->session == session) {
            WsRelayNode *drop = node;

            node = node->next;
            if (prev) {
                prev->next = node;
            } else {
                ws_relay_head = node;
            }
            drop->next = doomed;
            doomed = drop;
        } else {
            prev = node;
            node = node->next;
        }
    }
    ws_relay_session_clear(session);
    pthread_mutex_unlock(&ws_relay_mu);
    while (doomed) {
        WsRelayNode *next = doomed->next;

        free(doomed);
        doomed = next;
    }
}

bool ws_relay_session_wants(const WebSocketSessionData *session, const char *event) {
    size_t i;

    if (!session || !event) {
        return false;
    }
    for (i = 0; i < session->subscribed_event_count && i < NATS_MAX_RELAY_EVENTS; i++) {
        if (session->subscribed_events[i] &&
            strcmp(session->subscribed_events[i], event) == 0) {
            return true;
        }
    }
    return false;
}

int ws_relay_write_error(struct lws *wsi, const char *reason) {
    json_t *root = json_object();
    int rc;

    if (!root) {
        return -1;
    }
    json_object_set_new(root, "type", json_string("nats_subscribe_error"));
    json_object_set_new(root, "error", json_string(reason ? reason : "bad_events"));
    rc = ws_write_json_response(wsi, root);
    json_decref(root);
    log_this(SR_WEBSOCKET, "NATS subscribe rejected", LOG_LEVEL_DEBUG, 0);
    return rc;
}

int ws_relay_write_ok(struct lws *wsi, WebSocketSessionData *session) {
    json_t *root = json_object();
    json_t *events = json_array();
    int rc;

    if (!root || !events || !session) {
        json_decref(root);
        json_decref(events);
        return -1;
    }
    if (pthread_mutex_lock(&ws_relay_mu) != 0) {
        json_decref(root);
        json_decref(events);
        return -1;
    }
    {
        size_t i;

        for (i = 0; i < session->subscribed_event_count && i < NATS_MAX_RELAY_EVENTS; i++) {
            if (session->subscribed_events[i]) {
                json_array_append_new(events, json_string(session->subscribed_events[i]));
            }
        }
    }
    pthread_mutex_unlock(&ws_relay_mu);
    json_object_set_new(root, "type", json_string("nats_subscribe_ok"));
    json_object_set_new(root, "events", events);
    rc = ws_write_json_response(wsi, root);
    json_decref(root);
    return rc;
}

int ws_relay_handle_subscribe(struct lws *wsi, WebSocketSessionData *session, json_t *root) {
    json_t *events;
    char *names[NATS_MAX_RELAY_EVENTS];
    size_t count = 0;

    if (!session) {
        json_decref(root);
        return -1;
    }
    if (!app_config || !app_config->nats.WebSocketRelay.Enabled) {
        ws_relay_write_error(wsi, "disabled");
        json_decref(root);
        return 0;
    }
    events = json_object_get(root, "events");
    if (!json_is_array(events) || json_array_size(events) > NATS_MAX_RELAY_EVENTS) {
        ws_relay_write_error(wsi, "bad_events");
        json_decref(root);
        return 0;
    }
    memset(names, 0, sizeof(names));
    {
        size_t n = json_array_size(events);
        size_t i;

        for (i = 0; i < n; i++) {
            json_t *item = json_array_get(events, i);
            const char *name;
            size_t seen;
            bool duplicate = false;

            if (!json_is_string(item)) {
                continue;
            }
            name = json_string_value(item);
            if (!name || name[0] == '\0' || !nats_relay_event_allowed(name)) {
                continue;
            }
            for (seen = 0; seen < count; seen++) {
                if (strcmp(names[seen], name) == 0) {
                    duplicate = true;
                }
            }
            if (duplicate) {
                continue;
            }
            names[count] = strdup(name);
            if (!names[count]) {
                ws_relay_free_names(names, count);
                ws_relay_write_error(wsi, "unavailable");
                json_decref(root);
                return 0;
            }
            count++;
        }
    }
    if (pthread_mutex_lock(&ws_relay_mu) != 0) {
        ws_relay_free_names(names, count);
        ws_relay_write_error(wsi, "unavailable");
        json_decref(root);
        return 0;
    }
    ws_relay_session_free_events(session);
    {
        size_t i;

        for (i = 0; i < count; i++) {
            session->subscribed_events[i] = names[i];
            names[i] = NULL;
        }
        session->subscribed_event_count = count;
    }
    pthread_mutex_unlock(&ws_relay_mu);
    ws_relay_write_ok(wsi, session);
    json_decref(root);
    return 0;
}

void ws_relay_enqueue(const char *event, const char *subject, const json_t *data) {
    json_t *frame;
    json_t *copy;
    char *text;

    if (!event || event[0] == '\0' || !subject || subject[0] == '\0' ||
        !json_is_object(data)) {
        return;
    }
    frame = json_object();
    copy = json_deep_copy(data);
    if (!frame || !copy) {
        json_decref(frame);
        json_decref(copy);
        return;
    }
    json_object_set_new(frame, "type", json_string("nats_event"));
    json_object_set_new(frame, "event", json_string(event));
    json_object_set_new(frame, "subject", json_string(subject));
    json_object_set_new(frame, "data", copy);
    text = json_dumps(frame, JSON_COMPACT);
    json_decref(frame);
    if (!text) {
        return;
    }
    log_this(SR_NATS, "NATS relay %s", LOG_LEVEL_TRACE, 1, event);
    if (pthread_mutex_lock(&ws_relay_mu) != 0) {
        free(text);
        return;
    }
    {
        WsRelayNode *node = ws_relay_head;
        int dropped = 0;

        while (node) {
            WebSocketSessionData *session = node->session;

            if (session && session->authenticated && session->connection_valid &&
                ws_relay_session_wants(session, event)) {
                if (session->relay_queue_count >= WS_RELAY_QUEUE_DEPTH) {
                    dropped = 1;
                } else {
                    char *queued = strdup(text);

                    if (queued) {
                        session->relay_queue[session->relay_queue_count] = queued;
                        session->relay_queue_count++;
                        lws_callback_on_writable(node->wsi);
                    }
                }
            }
            node = node->next;
        }
        pthread_mutex_unlock(&ws_relay_mu);
        free(text);
        if (dropped) {
            log_this(SR_NATS, "NATS relay queue full", LOG_LEVEL_TRACE, 0);
        }
    }
}

void ws_relay_on_writable(struct lws *wsi, WebSocketSessionData *session) {
    char *text = NULL;
    bool more = false;

    if (!wsi || !session) {
        return;
    }
    if (pthread_mutex_lock(&ws_relay_mu) != 0) {
        return;
    }
    if (session->relay_queue_count > 0) {
        size_t i;

        text = session->relay_queue[0];
        for (i = 1; i < session->relay_queue_count; i++) {
            session->relay_queue[i - 1] = session->relay_queue[i];
        }
        session->relay_queue_count--;
        session->relay_queue[session->relay_queue_count] = NULL;
        more = session->relay_queue_count > 0;
    }
    pthread_mutex_unlock(&ws_relay_mu);
    if (!text) {
        return;
    }
    ws_write_raw_data(wsi, text, strlen(text));
    free(text);
    if (more) {
        lws_callback_on_writable(wsi);
    }
}
