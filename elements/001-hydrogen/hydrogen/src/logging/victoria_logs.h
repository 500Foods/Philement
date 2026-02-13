/*
 * VictoriaLogs Integration
 *
 * Provides direct HTTP logging to VictoriaLogs server without dependencies
 * on other subsystems. Logs are queued and sent by a dedicated thread
 * with intelligent dual-timer batching for optimal performance.
 *
 * This module is intentionally independent of the config system - it uses
 * environment variables only and initializes early in startup.
 */

#ifndef VICTORIA_LOGS_H
#define VICTORIA_LOGS_H

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

// VictoriaLogs configuration structure
// Populated from environment variables during early startup
typedef struct VictoriaLogsConfig {
    bool enabled;              // VICTORIALOGS_URL is set and valid
    char* url;                 // Full URL including path and query params
    int min_level;             // VICTORIALOGS_LVL mapped to numeric value

    // Cached Kubernetes metadata
    char* k8s_namespace;
    char* k8s_pod_name;
    char* k8s_container_name;
    char* k8s_node_name;
    char* host;
} VictoriaLogsConfig;

// Queue node for VictoriaLogs messages
typedef struct VLQueueNode {
    char* message;             // The JSON message (dynamically allocated)
    struct VLQueueNode* next;  // Next node in queue
} VLQueueNode;

// VictoriaLogs message queue (thread-safe)
typedef struct VLMessageQueue {
    VLQueueNode* head;         // Front of queue (dequeue from here)
    VLQueueNode* tail;         // Back of queue (enqueue here)
    size_t size;               // Current queue size
    size_t max_size;           // Maximum queue size before dropping
    pthread_mutex_t mutex;     // Queue mutex
    pthread_cond_t cond;       // Condition variable for thread signaling
} VLMessageQueue;

// VictoriaLogs thread state
typedef struct VLThreadState {
    pthread_t thread;          // The worker thread
    bool running;              // Thread is running
    bool shutdown;             // Shutdown requested
    
    // Batching state
    char* batch_buffer;        // Buffer for batched messages
    size_t batch_buffer_size;  // Current buffer usage
    size_t batch_count;        // Number of messages in batch
    time_t first_message_time; // Time first message was added to batch
    
    // Timer state
    struct timespec short_timer;  // 1s timer (resets on each log)
    struct timespec long_timer;   // 10s timer (periodic flush)
    bool short_timer_active;      // Whether short timer is counting
    bool first_log_sent;          // Whether first log has been sent
    
    // Queue
    VLMessageQueue queue;      // Message queue
} VLThreadState;

// Global configuration instance - initialized at startup
extern VictoriaLogsConfig victoria_logs_config;

// Global thread state
extern VLThreadState victoria_logs_thread;

// Maximum size for a single VictoriaLogs message
#define VICTORIA_LOGS_MAX_MESSAGE_SIZE 4096

// HTTP timeout in seconds for VictoriaLogs requests
#define VICTORIA_LOGS_TIMEOUT_SEC 5

// Maximum batch size before sending (messages)
#define VICTORIA_LOGS_BATCH_SIZE 50

// Maximum batch buffer size (bytes)
#define VICTORIA_LOGS_MAX_BATCH_BUFFER (1024 * 1024)  // 1MB

// Short timer interval (1 second) - resets on each log
#define VICTORIA_LOGS_SHORT_TIMER_SEC 1

// Long timer interval (10 seconds) - periodic flush during heavy load
#define VICTORIA_LOGS_LONG_TIMER_SEC 10

// Maximum queue size before dropping messages
#define VICTORIA_LOGS_MAX_QUEUE_SIZE 10000

/**
 * Initialize VictoriaLogs configuration from environment variables
 *
 * This function must be called EARLY in startup (after init_startup_log_level)
 * and BEFORE any logging occurs. It reads:
 * - VICTORIALOGS_URL: Full URL including path and query params
 * - VICTORIALOGS_LVL: Minimum log level (default: DEBUG)
 * - K8S_NAMESPACE, K8S_POD_NAME, K8S_NODE_NAME, K8S_CONTAINER_NAME
 *
 * If VICTORIALOGS_URL is not set, VictoriaLogs is silently disabled.
 * If VICTORIALOGS_URL is set, a dedicated thread is started for sending logs.
 *
 * @return true on success, false on error (VictoriaLogs will be disabled)
 */
bool init_victoria_logs(void);

/**
 * Cleanup VictoriaLogs configuration and resources
 *
 * Called during shutdown to flush pending logs and free allocated memory.
 * Signals the worker thread to stop and waits for it to finish.
 */
void cleanup_victoria_logs(void);

/**
 * Send a log message to VictoriaLogs
 *
 * This function quickly enqueues the message for the worker thread.
 * It is non-blocking and returns immediately.
 *
 * @param subsystem The subsystem name (e.g., "Logging", "Config")
 * @param message The log message text
 * @param priority The log priority level (LOG_LEVEL_TRACE, etc.)
 * @return true if enqueued successfully, false otherwise (e.g., queue full)
 */
bool victoria_logs_send(const char* subsystem, const char* message, int priority);

/**
 * Check if VictoriaLogs is enabled and ready
 *
 * @return true if VictoriaLogs is enabled and can accept logs
 */
bool victoria_logs_is_enabled(void);

/**
 * Parse log level string to numeric value
 *
 * Internal function exposed for testing.
 *
 * @param level_str The level string (e.g., "DEBUG", "STATE")
 * @param default_level Default value if string is NULL or invalid
 * @return The numeric log level
 */
int victoria_logs_parse_level(const char* level_str, int default_level);

/**
 * Escape a string for JSON output
 *
 * Internal function exposed for testing.
 * Replaces special characters with their JSON escape sequences.
 *
 * @param input The input string
 * @param output The output buffer
 * @param output_size The output buffer size
 * @return Number of characters written, or -1 if buffer too small
 */
int victoria_logs_escape_json(const char* input, char* output, size_t output_size);

/**
 * Flush any pending logs immediately (used during shutdown)
 *
 * Internal function exposed for shutdown sequence.
 */
void victoria_logs_flush(void);

#endif // VICTORIA_LOGS_H
