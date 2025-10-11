/*
 * Mock status functions for unit testing
 *
 * This file provides mock implementations of status collection functions
 * to enable unit testing without system dependencies.
 */

#ifndef MOCK_STATUS_H
#define MOCK_STATUS_H

#include <jansson.h>

// Include the header that defines WebSocketMetrics
#include <src/status/status_core.h>

// Mock function declarations - always declare for test files
// These will override the real ones when USE_MOCK_STATUS is defined
#ifdef USE_MOCK_STATUS

// Override status functions with our mocks
#define get_system_status_json mock_get_system_status_json

#endif // USE_MOCK_STATUS

// Mock function prototypes (always available)
json_t* mock_get_system_status_json(const WebSocketMetrics *ws_metrics);

// Mock control functions (always available)
void mock_status_reset_all(void);
void mock_status_set_json_result(json_t* result);

#endif // MOCK_STATUS_H