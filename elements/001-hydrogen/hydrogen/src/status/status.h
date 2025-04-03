/*
 * System status and metrics reporting utilities.
 * 
 * Provides functionality for:
 * - System status reporting
 * - File descriptor tracking
 * - Process memory metrics
 * - WebSocket statistics
 */

#ifndef STATUS_H
#define STATUS_H

#include <jansson.h>
#include <time.h>
#include "../config/config.h"

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

// Memory tracking functions
void get_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap);

#endif // STATUS_H