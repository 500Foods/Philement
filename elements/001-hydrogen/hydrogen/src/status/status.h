/*
 * System Status Interface
 * 
 * Provides high-level functions for collecting and formatting system status
 * information in both JSON and Prometheus formats.
 */

#ifndef STATUS_H
#define STATUS_H

#include <jansson.h>
#include "status_core.h"

// Initialize the status collection system
void status_init(void);

// Clean up the status collection system
void status_cleanup(void);

// Helper function to collect all metrics
SystemMetrics* collect_all_metrics(const WebSocketMetrics *ws_metrics);

// Get complete system status in JSON format
// This is the original format used by the /api/system/info endpoint
// Returns a new JSON object that must be freed by the caller
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics);

// Get system status in Prometheus format
// Returns a newly allocated string containing metrics in Prometheus format
// The string must be freed by the caller
char* get_system_status_prometheus(const WebSocketMetrics *ws_metrics);

#endif /* STATUS_H */
