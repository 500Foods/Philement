// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include <sys/sysinfo.h>

extern WebSocketServerContext *ws_context;

// Handle real-time status request via WebSocket
//
// Status reporting design prioritizes:
// 1. Data Accuracy
//    - Atomic metric collection
//    - Consistent timestamps
//    - Race condition prevention
//    - State synchronization
//
// 2. Performance
//    - Efficient JSON generation
//    - Memory pooling
//    - Minimal allocations
//    - Zero-copy where possible
//
// 3. Reliability
//    - Memory leak prevention
//    - Error recovery paths
//    - Resource cleanup
//    - Partial success handling
//
// 4. Client Experience
//    - Consistent message format
//    - Meaningful metrics
//    - Real-time updates
//    - Low latency delivery
void handle_status_request(struct lws *wsi)
{
    if (!ws_context) {
        log_this(SR_WEBSOCKET, "No server context available", LOG_LEVEL_ERROR, 0);
        return;
    }

    log_this(SR_WEBSOCKET, "Preparing status response", LOG_LEVEL_STATE, 0);

    // Safely copy metrics under lock
    pthread_mutex_lock(&ws_context->mutex);
    const WebSocketMetrics metrics = {
        .server_start_time = ws_context->start_time,
        .active_connections = ws_context->active_connections,
        .total_connections = ws_context->total_connections,
        .total_requests = ws_context->total_requests
    };
    pthread_mutex_unlock(&ws_context->mutex);

    // Get system status JSON with WebSocket metrics
    json_t *root = get_system_status_json(&metrics);
    char *response_str = json_dumps(root, JSON_COMPACT);
    
    // Pretty print the JSON for better debugging
    char *pretty_str = json_dumps(root, JSON_INDENT(2));
    if (pretty_str) {
        log_this(SR_WEBSOCKET, "Status response (pretty printed):", LOG_LEVEL_STATE, 0);
        // Split into lines for better readability
        char *line_start = pretty_str;
        char *line_end;
        while ((line_end = strchr(line_start, '\n')) != NULL) {
            *line_end = '\0';
            log_this(SR_WEBSOCKET, "  %s", LOG_LEVEL_STATE, 1, line_start);
            line_start = line_end + 1;
        }
        // Handle last line if it doesn't end with newline
        if (*line_start) {
            log_this(SR_WEBSOCKET, "  %s", LOG_LEVEL_STATE, 1, line_start);
        }
        free(pretty_str);
    }
    
    json_decref(root);

    size_t len = strlen(response_str);

    // Prepare and send WebSocket message
    unsigned char *buf = malloc(LWS_PRE + len);
    if (buf) {
        memcpy(buf + LWS_PRE, response_str, len);
        int written = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        log_this(SR_WEBSOCKET, "Wrote %d bytes to WebSocket", LOG_LEVEL_STATE, 1, written);
        free(buf);
        
        // Note: Removed lws_callback_on_writable() call as it creates race condition
        // with clients that close immediately after receiving data (like websocat --one-message)
    } else {
        log_this(SR_WEBSOCKET, "Failed to allocate buffer for response", LOG_LEVEL_ERROR, 0);
    }

    free(response_str);
}
