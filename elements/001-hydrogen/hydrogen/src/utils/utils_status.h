/*
 * System status and metrics reporting utilities.
 * 
 * Provides functionality for:
 * - System status reporting
 * - File descriptor tracking
 * - Process memory metrics
 * - WebSocket statistics
 */

#ifndef UTILS_STATUS_H
#define UTILS_STATUS_H

#include <jansson.h>
#include <time.h>
#include "../config/configuration.h"

// File descriptor information structure
typedef struct {
    int fd;                 // File descriptor number
    char type[DEFAULT_FD_TYPE_SIZE];         // Type (socket, file, pipe, etc.)
    char description[DEFAULT_FD_DESCRIPTION_SIZE]; // Detailed description
} FileDescriptorInfo;

// WebSocket metrics structure
typedef struct {
    time_t server_start_time;  // Server start timestamp
    int active_connections;    // Current live connections
    int total_connections;     // Historical connection count
    int total_requests;        // Total processed requests
} WebSocketMetrics;

// System status functions
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics);
json_t* get_file_descriptors_json(void);

#endif // UTILS_STATUS_H