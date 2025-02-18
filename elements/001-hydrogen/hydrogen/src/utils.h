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

// Memory tracking constants and structures
#define MAX_SERVICE_THREADS 32
#define MAX_QUEUE_BLOCKS 128

// File descriptor information structure
typedef struct {
    int fd;                 // File descriptor number
    char type[32];         // Type (socket, file, pipe, etc.)
    char description[256]; // Detailed description
} FileDescriptorInfo;

// Memory metrics structure
typedef struct {
    size_t virtual_bytes;     // Virtual memory usage in bytes
    size_t resident_bytes;    // Resident memory usage in bytes
} MemoryMetrics;

// Thread memory metrics (alias for compatibility)
typedef MemoryMetrics ThreadMemoryMetrics;

// Queue memory and entry tracking
typedef struct {
    size_t block_count;                      // Number of allocated blocks
    size_t total_allocation;                 // Total bytes allocated
    size_t entry_count;                      // Number of entries in queue
    MemoryMetrics metrics;                   // Queue memory usage
    size_t block_sizes[MAX_QUEUE_BLOCKS];    // Size of each allocated block
} QueueMemoryMetrics;

// Service thread and memory information
typedef struct {
    pthread_t thread_ids[MAX_SERVICE_THREADS];  // Array of pthread IDs
    pid_t thread_tids[MAX_SERVICE_THREADS];     // Array of Linux thread IDs
    int thread_count;                           // Number of active threads
    size_t virtual_memory;                      // Total virtual memory for service
    size_t resident_memory;                     // Total resident memory for service
    ThreadMemoryMetrics thread_metrics[MAX_SERVICE_THREADS];  // Memory metrics per thread
    double memory_percent;                      // Percentage of total process memory
} ServiceThreads;

// Global tracking structures
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_threads;
extern ServiceThreads print_threads;

// Queue memory tracking
extern QueueMemoryMetrics log_queue_memory;
extern QueueMemoryMetrics print_queue_memory;

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

// Memory tracking initialization
void init_service_threads(ServiceThreads *threads);
void init_queue_memory(QueueMemoryMetrics *queue);

// Thread management
void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id);

// Memory tracking
void update_service_thread_metrics(ServiceThreads *threads);
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id);

// Queue entry tracking
void track_queue_entry_added(QueueMemoryMetrics *queue);
void track_queue_entry_removed(QueueMemoryMetrics *queue);

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
// Generate system status report in JSON format
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics);

// Get detailed file descriptor information
json_t* get_file_descriptors_json(void);

// Format and output a log message directly to console
// Matches the format of the logging queue system:
// timestamp  [ priority ]  [ subsystem ]  message
// Parameters:
//   subsystem: Name of the subsystem (e.g., "Shutdown", "WebServer")
//   priority: Log priority level (0-3)
//   message: The message to log
void console_log(const char* subsystem, int priority, const char* message);

#endif // UTILS_H
