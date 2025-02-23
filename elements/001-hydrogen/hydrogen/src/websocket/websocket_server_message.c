/*
 * WebSocket Message Processing
 *
 * Handles incoming WebSocket messages:
 * - Message buffering and assembly
 * - JSON parsing and validation
 * - Message type routing
 * - Error handling
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <string.h>

// External libraries
#include <jansson.h>
#include <libwebsockets.h>

// Project headers
#include "websocket_server.h"        // For handle_status_request
#include "websocket_server_internal.h"
#include "../logging/logging.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

// Forward declarations of message type handlers
static int handle_message_type(struct lws *wsi, const char *type);

int ws_handle_receive(struct lws *wsi, WebSocketSessionData *session, void *in, size_t len)
{
    if (!session || !ws_context) {
        log_this("WebSocket", "Invalid session or context", LOG_LEVEL_ERROR);
        return -1;
    }

    // Verify authentication
    if (!ws_is_authenticated(session)) {
        log_this("WebSocket", "Received data from unauthenticated connection", LOG_LEVEL_WARN);
        return -1;
    }

    pthread_mutex_lock(&ws_context->mutex);
    ws_context->total_requests++;
    
    // Check message size
    if (ws_context->message_length + len > ws_context->max_message_size) {
        pthread_mutex_unlock(&ws_context->mutex);
        log_this("WebSocket", "Message too large (max size: %zu bytes)", 
                 LOG_LEVEL_WARN, true, true, true,
                 ws_context->max_message_size);
        ws_context->message_length = 0; // Reset buffer
        return -1;
    }

    // Copy data to message buffer
    memcpy(ws_context->message_buffer + ws_context->message_length, in, len);
    ws_context->message_length += len;

    // If not final fragment, wait for more
    if (!lws_is_final_fragment(wsi)) {
        pthread_mutex_unlock(&ws_context->mutex);
        return 0;
    }

    // Null terminate and reset the message buffer
    ws_context->message_buffer[ws_context->message_length] = '\0';
    ws_context->message_length = 0; // Reset for next message

    pthread_mutex_unlock(&ws_context->mutex);

    // Log the complete message for debugging
    log_this("WebSocket", "Processing complete message", LOG_LEVEL_INFO);

    // Parse JSON message
    json_error_t error;
    json_t *root = json_loads((const char *)ws_context->message_buffer, 0, &error);
    
    if (!root) {
        log_this("WebSocket", "Error parsing JSON: %s", LOG_LEVEL_WARN, error.text);
        return 0;
    }

    // Extract message type
    json_t *type_json = json_object_get(root, "type");
    int result = -1;

    if (json_is_string(type_json)) {
        const char *type = json_string_value(type_json);
        log_this("WebSocket", "Processing message type: %s", LOG_LEVEL_INFO, type);
        result = handle_message_type(wsi, type);
    } else {
        log_this("WebSocket", "Missing or invalid 'type' in request", LOG_LEVEL_INFO);
    }

    json_decref(root);
    return result;
}

static int handle_message_type(struct lws *wsi, const char *type)
{
    if (strcmp(type, "status") == 0) {
        log_this("WebSocket", "Handling status request", LOG_LEVEL_INFO);
        handle_status_request(wsi);
        return 0;
    }

    log_this("WebSocket", "Unknown message type: %s", LOG_LEVEL_INFO, type);
    return -1;
}

// Helper function to write a JSON response
int ws_write_json_response(struct lws *wsi, json_t *json)
{
    char *response_str = json_dumps(json, JSON_COMPACT);
    if (!response_str) {
        log_this("WebSocket", "Failed to serialize JSON response", LOG_LEVEL_ERROR);
        return -1;
    }

    size_t len = strlen(response_str);
    unsigned char *buf = malloc(LWS_PRE + len);
    int result = -1;

    if (buf) {
        memcpy(buf + LWS_PRE, response_str, len);
        result = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        free(buf);
    } else {
        log_this("WebSocket", "Failed to allocate response buffer", LOG_LEVEL_ERROR);
    }

    free(response_str);
    return result;
}