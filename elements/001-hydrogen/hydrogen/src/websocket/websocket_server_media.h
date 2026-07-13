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

/**
 * Send a media upload error response to the client
 * @param wsi WebSocket connection instance
 * @param error_message Error message (NULL is tolerated -> "Unknown error")
 * @param request_id Optional request identifier to echo back (may be NULL)
 */
void send_media_upload_error(struct lws *wsi, const char* error_message, const char* request_id);

/**
 * Send a media upload success response to the client
 * @param wsi WebSocket connection instance
 * @param request_id Optional request identifier to echo back (may be NULL)
 * @param media_hash SHA-256 hash of the stored media
 * @param media_size Size of the stored media in bytes
 * @param mime_type Optional MIME type (may be NULL)
 */
void send_media_upload_success(struct lws *wsi, const char* request_id, const char* media_hash,
                               size_t media_size, const char* mime_type);

#endif // WEBSOCKET_SERVER_MEDIA_H