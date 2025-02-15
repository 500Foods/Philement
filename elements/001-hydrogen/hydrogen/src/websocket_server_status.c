/*
 * WebSocket server status handler for the Hydrogen printer.
 * 
 * Provides real-time server statistics and status information via WebSocket,
 * including uptime, connection counts, and request metrics. Formats the data
 * as JSON for easy consumption by client applications.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <time.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>
#include <libwebsockets.h>

// Project headers
#include "websocket_server.h"
#include "logging.h"
#include "configuration.h"
#include "utils.h"

extern AppConfig *app_config;
extern time_t server_start_time;
extern int ws_connections;
extern int ws_connections_total;
extern int ws_requests;

void handle_status_request(struct lws *wsi)
{
    log_this("WebSocket", "Preparing status response", 0, true, true, true);

    // Prepare WebSocket metrics
    WebSocketMetrics metrics = {
        .server_start_time = server_start_time,
        .active_connections = ws_connections,
        .total_connections = ws_connections_total,
        .total_requests = ws_requests
    };

    // Get system status JSON with WebSocket metrics
    json_t *root = get_system_status_json(&metrics);
    char *response_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);

    size_t len = strlen(response_str);
    log_this("WebSocket", "Status response: %s", 0, true, true, true, response_str);

    // Prepare and send WebSocket message
    unsigned char *buf = malloc(LWS_PRE + len);
    if (buf) {
        memcpy(buf + LWS_PRE, response_str, len);
        int written = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        log_this("WebSocket", "Wrote %d bytes to WebSocket", 0, true, true, true, written);
        free(buf);
    } else {
        log_this("WebSocket", "Failed to allocate buffer for response", 3, true, true, true);
    }

    free(response_str);
}
