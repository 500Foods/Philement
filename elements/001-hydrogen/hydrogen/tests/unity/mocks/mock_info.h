/*
 * Mock info functions for unit testing
 *
 * This file provides mock implementations of info functions
 * to enable unit testing without system dependencies.
 */

#ifndef MOCK_INFO_H
#define MOCK_INFO_H

// Include headers that define WebSocketMetrics
#include <src/status/status_core.h>
#include <src/api/system/info/info.h>

// Mock function declarations - always declare for test files
// These will override the real ones when USE_MOCK_INFO is defined
#ifdef USE_MOCK_INFO

// Override info functions with our mocks
#define extract_websocket_metrics mock_extract_websocket_metrics

#endif // USE_MOCK_INFO

// Mock function prototypes (always available)
void mock_extract_websocket_metrics(WebSocketMetrics *metrics);

// Mock control functions (always available)
void mock_info_reset_all(void);
void mock_info_set_websocket_metrics(const WebSocketMetrics *metrics);

#endif // MOCK_INFO_H