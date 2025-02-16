/*
 * Utility functions and constants for the Hydrogen printer.
 * 
 * Provides common functionality used throughout the application, focusing
 * on reusable operations and shared data structures. These utilities are
 * designed for reliability and thread safety where needed.
 * 
 * ID Generation:
 * - Consonant-based for readability
 * - Cryptographically secure random source
 * - Configurable length
 * - Collision avoidance
 * 
 * Status Reporting:
 * - Thread-safe JSON generation
 * - Component status aggregation
 * - Performance metrics
 * - Resource usage tracking
 * 
 * Thread Safety:
 * - Thread-safe function implementations
 * - Safe memory management
 * - Atomic operations where needed
 * - Resource cleanup handling
 * 
 * Error Handling:
 * - Consistent error reporting
 * - Resource cleanup on errors
 * - NULL pointer safety
 * - Bounds checking
 */

#ifndef UTILS_H
#define UTILS_H

// System Libraries
#include <stddef.h>
#include <jansson.h>

// Constants for ID generation
// ID_CHARS: Consonants chosen for readability
// ID_LEN: Length providing sufficient uniqueness
#define ID_CHARS "BCDFGHKPRST"  // Consonants for readable IDs
#define ID_LEN 5                // Balance of uniqueness and readability

// WebSocket metrics structure
// Tracks real-time WebSocket server statistics:
// - Server uptime tracking
// - Connection counting
// - Request monitoring
// - Performance metrics
typedef struct {
    time_t server_start_time;  // Server start timestamp
    int active_connections;    // Current live connections
    int total_connections;     // Historical connection count
    int total_requests;        // Total processed requests
} WebSocketMetrics;

// Generate a random identifier string
// Uses secure random source to create readable IDs:
// - Consonant-based for clarity
// - Fixed length for consistency
// - NULL-terminated output
// Parameters:
//   buf: Output buffer (must be at least len+1 bytes)
//   len: Desired ID length (typically ID_LEN)
void generate_id(char *buf, size_t len);

// Generate system status report in JSON format
// Thread-safe status collection and JSON generation:
// - Component status aggregation
// - Resource usage statistics
// - Performance metrics
// - WebSocket statistics (optional)
// Parameters:
//   ws_metrics: Optional WebSocket metrics (NULL to omit)
// Returns:
//   json_t*: New JSON object (caller must json_decref)
//   NULL: On memory allocation failure
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics);

#endif // UTILS_H
