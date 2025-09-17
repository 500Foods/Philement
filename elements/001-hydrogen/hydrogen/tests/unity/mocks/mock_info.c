/*
 * Mock info functions for unit testing
 *
 * This file provides mock implementations of info functions
 * to enable unit testing without system dependencies.
 */

#include "mock_info.h"
#include <stdlib.h>

// Static variable to store mock WebSocket metrics
static WebSocketMetrics mock_websocket_metrics_data = {0};

// Mock implementation of extract_websocket_metrics
void mock_extract_websocket_metrics(WebSocketMetrics *metrics) {
    if (metrics) {
        *metrics = mock_websocket_metrics_data;
    }
}

// Mock control functions
void mock_info_reset_all(void) {
    mock_websocket_metrics_data = (WebSocketMetrics){0};
}

void mock_info_set_websocket_metrics(const WebSocketMetrics *metrics) {
    if (metrics) {
        mock_websocket_metrics_data = *metrics;
    } else {
        mock_websocket_metrics_data = (WebSocketMetrics){0};
    }
}