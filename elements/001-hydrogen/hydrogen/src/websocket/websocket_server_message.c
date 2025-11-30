 /*
 * WebSocket Message Processing
 *
 * Handles incoming WebSocket messages:
 * - Message buffering and assembly
 * - JSON parsing and validation
 * - Message type routing
 * - Error handling
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server.h"        // For handle_status_request
#include "websocket_server_internal.h"

// Libwebsockets header for struct lws
#include <libwebsockets.h>

// External reference to the server context
extern WebSocketServerContext *ws_context;

// Forward declarations of message type handlers
int handle_message_type(struct lws *wsi, const char *type);

// Forward declarations for terminal functions (implemented in websocket_server_terminal.c)
int handle_terminal_message(struct lws *wsi);

// Forward declarations for WebSocket utility functions
int ws_write_raw_data(struct lws *wsi, const char *data, size_t len);
json_t* create_json_response(const char *type, const char *data);
json_t* create_pty_output_json(const char *buffer, size_t data_size);

int validate_session_and_context(const WebSocketSessionData *session)
{
    if (!session || !ws_context) {
        log_this(SR_WEBSOCKET, "Invalid session or context", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    return 0;
}

int buffer_message_data(struct lws *wsi, const void *in, size_t len)
{
    // Check message size
    if (ws_context->message_length + len > ws_context->max_message_size) {
        log_this(SR_WEBSOCKET, "Message too large (max size: %zu bytes)", LOG_LEVEL_ALERT, 1, ws_context->max_message_size);
        ws_context->message_length = 0; // Reset buffer
        return -1;
    }

    // Copy data to message buffer
    memcpy(ws_context->message_buffer + ws_context->message_length, in, len);
    ws_context->message_length += len;

    // If not final fragment, wait for more
    if (!lws_is_final_fragment(wsi)) {
        return 0;
    }

    // Null terminate the message buffer
    ws_context->message_buffer[ws_context->message_length] = '\0';
    
    // Return the message length (the reset will happen after processing)
    return (int)ws_context->message_length;
}

int parse_and_handle_message(struct lws *wsi)
{
    // Parse JSON message
    json_error_t error;
    json_t *root = json_loads((const char *)ws_context->message_buffer, 0, &error);
    
    if (!root) {
        log_this(SR_WEBSOCKET, "Error parsing JSON: %s", LOG_LEVEL_ALERT, 1, error.text);
        return 0;
    }

    // Extract message type
    json_t *type_json = json_object_get(root, "type");
    int result = -1;

    if (json_is_string(type_json)) {
        const char *type = json_string_value(type_json);
        log_this(SR_WEBSOCKET, "Processing message type: %s", LOG_LEVEL_STATE, 1, type);
        result = handle_message_type(wsi, type);
    } else {
        log_this(SR_WEBSOCKET, "Missing or invalid 'type' in request", LOG_LEVEL_STATE, 0);
    }

    json_decref(root);
    return result;
}

int ws_handle_receive(struct lws *wsi, const WebSocketSessionData *session, const void *in, size_t len)
{
    if (validate_session_and_context(session) != 0) {
        return -1;
    }

    // Verify authentication
    if (!ws_is_authenticated(session)) {
        log_this(SR_WEBSOCKET, "Received data from unauthenticated connection", LOG_LEVEL_ALERT, 0);
        return -1;
    }

    pthread_mutex_lock(&ws_context->mutex);
    ws_context->total_requests++;
    
    int buffer_result = buffer_message_data(wsi, in, len);
    if (buffer_result <= 0) {
        // -1 for error, 0 for incomplete fragment
        pthread_mutex_unlock(&ws_context->mutex);
        return buffer_result;
    }

    // buffer_result now contains the message length
    // Save it before unlocking the mutex
    size_t saved_message_length = (size_t)buffer_result;

    pthread_mutex_unlock(&ws_context->mutex);

    // Comment out verbose message logging to reduce log size
    // log_this(SR_WEBSOCKET, "Processing complete message: %s", LOG_LEVEL_STATE, 1, ws_context->message_buffer);

    int result = parse_and_handle_message(wsi);
    
    // Reset message length after processing is complete
    pthread_mutex_lock(&ws_context->mutex);
    ws_context->message_length = 0;
    pthread_mutex_unlock(&ws_context->mutex);
    
    // Suppress unused variable warning
    (void)saved_message_length;
    
    return result;
}

int handle_message_type(struct lws *wsi, const char *type)
{
    if (strcmp(type, "status") == 0) {
        log_this(SR_WEBSOCKET, "Handling status request", LOG_LEVEL_STATE, 0);
        handle_status_request(wsi);
        return 0;
    }

    // Route terminal messages to terminal handlers
    // Terminal protocol uses 'input', 'resize', 'ping' message types
    if (strcmp(type, "input") == 0 || strcmp(type, "resize") == 0 || strcmp(type, "ping") == 0) {
        return handle_terminal_message(wsi);
    }

    log_this(SR_WEBSOCKET, "Unknown message type: %s", LOG_LEVEL_STATE, 1, type);
    return -1;
}


// Helper function to write a JSON response
int ws_write_json_response(struct lws *wsi, json_t *json)
{
    char *response_str = json_dumps(json, JSON_COMPACT);
    if (!response_str) {
        log_this(SR_WEBSOCKET, "Failed to serialize JSON response", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    size_t response_len = strlen(response_str);
    int result = ws_write_raw_data(wsi, response_str, response_len);

    free(response_str);

    // Return the length of data written for backward compatibility with tests
    return result == 0 ? (int)response_len : result;
}

// Helper function to write raw data to WebSocket
int ws_write_raw_data(struct lws *wsi, const char *data, size_t len)
{
    unsigned char *buf = malloc(LWS_PRE + len);
    if (!buf) {
        log_this(SR_WEBSOCKET, "Failed to allocate WebSocket buffer", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    memcpy(buf + LWS_PRE, data, len);
    int result = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);

    free(buf);
    return result >= 0 ? 0 : -1;
}

// Helper function to create a simple JSON response with type and data
json_t* create_json_response(const char *type, const char *data)
{
    json_t *json_response = json_object();
    if (json_response) {
        json_object_set_new(json_response, "type", json_string(type));
        if (data) {
            json_object_set_new(json_response, "data", json_string(data));
        }
    }
    return json_response;
}

// Helper function to create PTY output JSON response
json_t* create_pty_output_json(const char *buffer, size_t data_size)
{
    if (!buffer || data_size == 0) {
        return create_json_response("output", "");
    }

    char *data_str = strndup(buffer, data_size);
    if (!data_str) {
        return NULL;
    }

    json_t *json_response = create_json_response("output", data_str);

    free(data_str);
    return json_response;
}

