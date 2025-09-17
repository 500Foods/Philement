/*
 * Mock status functions for unit testing
 *
 * This file provides mock implementations of status collection functions
 * to enable unit testing without system dependencies.
 */

#include "mock_status.h"
#include <jansson.h>
#include <stdlib.h>

// Static variable to store mock result
static json_t* mock_json_result = NULL;

// Mock implementation of get_system_status_json
json_t* mock_get_system_status_json(const WebSocketMetrics *ws_metrics) {
    (void)ws_metrics; // Suppress unused parameter warning
    printf("MOCK: mock_get_system_status_json called\n");

    if (mock_json_result) {
        // Return a copy of the mock result
        return json_deep_copy(mock_json_result);
    }

    // Return a default mock JSON object
    json_t* root = json_object();
    if (root) {
        json_object_set_new(root, "status", json_string("mock"));
        json_object_set_new(root, "timestamp", json_integer(1234567890));
    }
    return root;
}

// Mock control functions
void mock_status_reset_all(void) {
    if (mock_json_result) {
        json_decref(mock_json_result);
        mock_json_result = NULL;
    }
}

void mock_status_set_json_result(json_t* result) {
    if (mock_json_result) {
        json_decref(mock_json_result);
    }
    mock_json_result = result ? json_deep_copy(result) : NULL;
}