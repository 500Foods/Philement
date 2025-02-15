/*
 * Utility functions and constants for the Hydrogen printer.
 * 
 * Provides common functionality used across the application:
 * - Random ID generation using consonants for readability
 * - Shared constants and macros
 */

#ifndef UTILS_H
#define UTILS_H

// System Libraries
#include <stddef.h>
#include <jansson.h>

#define ID_CHARS "BCDFGHKPRST"
#define ID_LEN 5

// Optional WebSocket metrics for status
typedef struct {
    time_t server_start_time;  // For uptime calculation
    int active_connections;    // Current WebSocket connections
    int total_connections;     // Total WebSocket connections
    int total_requests;        // Total WebSocket requests
} WebSocketMetrics;

// Generate a random ID
void generate_id(char *buf, size_t len);

// Generate system status JSON in a thread-safe manner
// If ws_metrics is NULL, WebSocket-specific metrics will be omitted
// Returns a new JSON object that must be freed by the caller using json_decref
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics);

#endif // UTILS_H
