/*
 * WebSocket Media Upload Handler
 *
 * Handles media upload messages via WebSocket protocol.
 * Supports single-message and chunked uploads.
 */

#ifndef WEBSOCKET_SERVER_MEDIA_H
#define WEBSOCKET_SERVER_MEDIA_H

#include <libwebsockets.h>
#include <jansson.h>
#include "websocket_server_internal.h"

/**
 * Handle a media upload message from WebSocket client
 * @param wsi WebSocket connection instance
 * @param session Session data for this connection
 * @param request_json Parsed JSON request (already validated)
 * @return 0 on success, -1 on error
 */
int handle_media_upload_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json);

/**
 * Handle a media chunk message from WebSocket client (for chunked uploads)
 * @param wsi WebSocket connection instance
 * @param session Session data for this connection
 * @param request_json Parsed JSON request (already validated)
 * @return 0 on success, -1 on error
 */
int handle_media_chunk_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json);

/**
 * Initialize media subsystem (called during server startup)
 * @return 0 on success, -1 on failure
 */
int media_subsystem_init(void);

/**
 * Cleanup media subsystem (called during server shutdown)
 */
void media_subsystem_cleanup(void);

/**
 * Cleanup media-specific resources for a session
 * @param session Session data to clean up
 */
void media_session_cleanup(WebSocketSessionData *session);

#endif // WEBSOCKET_SERVER_MEDIA_H