/*
 * WebSocket Chat Handler
 *
 * Handles chat messages via WebSocket protocol.
 * Supports both streaming and non-streaming chat requests.
 */

#ifndef WEBSOCKET_SERVER_CHAT_H
#define WEBSOCKET_SERVER_CHAT_H

#include <libwebsockets.h>
#include <jansson.h>
#include "websocket_server_internal.h"

/**
 * Handle a chat message from WebSocket client
 * @param wsi WebSocket connection instance
 * @param session Session data for this connection
 * @param request_json Parsed JSON request (already validated)
 * @return 0 on success, -1 on error
 */
int handle_chat_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json);

/**
 * Initialize chat subsystem (called during server startup)
 * @return 0 on success, -1 on failure
 */
int chat_subsystem_init(void);

/**
 * Cleanup chat subsystem (called during server shutdown)
 */
void chat_subsystem_cleanup(void);

/**
 * Cleanup chat-specific resources for a session
 * @param session Session data to clean up
 */
void chat_session_cleanup(WebSocketSessionData *session);

/**
 * Create a unique queue name based on WebSocket connection
 * @param wsi WebSocket connection instance
 * @return Newly allocated queue name (caller must free)
 */
char* create_chat_queue_name(const struct lws *wsi);

/**
 * Handle writable callback for chat streaming
 * @param wsi WebSocket connection instance
 * @param session Session data for this connection
 */
void handle_chat_writable(struct lws *wsi, const WebSocketSessionData *session);

#endif // WEBSOCKET_SERVER_CHAT_H
