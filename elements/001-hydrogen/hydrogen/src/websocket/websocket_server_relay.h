/*
 * WebSocket relay of eligible NATS events.
 *
 * The session list and the writable callback live here. NATS decides
 * which event is eligible and then calls ws_relay_enqueue. lws_write
 * runs on the libwebsockets thread.
 */

#ifndef WEBSOCKET_SERVER_RELAY_H
#define WEBSOCKET_SERVER_RELAY_H

#include <jansson.h>

#include "websocket_server_internal.h"

void ws_relay_session_add(struct lws *wsi, WebSocketSessionData *session);
void ws_relay_session_remove(WebSocketSessionData *session);
void ws_relay_session_free_events(WebSocketSessionData *session);
void ws_relay_session_clear(WebSocketSessionData *session);
void ws_relay_free_names(char **names, size_t count);
bool ws_relay_session_wants(const WebSocketSessionData *session, const char *event);
int ws_relay_handle_subscribe(struct lws *wsi, WebSocketSessionData *session, json_t *root);
int ws_relay_write_error(struct lws *wsi, const char *reason);
int ws_relay_write_ok(struct lws *wsi, WebSocketSessionData *session);
void ws_relay_enqueue(const char *event, const char *subject, const json_t *data);
void ws_relay_on_writable(struct lws *wsi, WebSocketSessionData *session);

#endif /* WEBSOCKET_SERVER_RELAY_H */
