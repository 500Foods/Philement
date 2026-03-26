/*
 * Chat Proxy Multi - Thread-Safe Streaming with libcurl multi Interface
 *
 * Provides non-blocking, event-driven streaming proxy using libcurl's multi
 * socket API. Integrates with LWS event loop for thread-safe WebSocket writes.
 *
 * Architecture:
 * - Single CURLM handle manages all concurrent streams
 * - curl_multi_socket_action() integrated with LWS poll
 * - Thread-safe chunk queues for LWS write callbacks
 * - All lws_write() calls happen from LWS service thread
 */

#ifndef PROXY_MULTI_H
#define PROXY_MULTI_H

// Project includes
#include <src/hydrogen.h>
#include <curl/curl.h>

// Local includes
#include "engine_cache.h"
#include "resp_parser.h"

// Forward declarations for LWS types
struct lws;
struct lws_context;

// Forward declaration for stream chunk callback (defined in resp_parser.h)
struct ChatStreamChunk;

// Callback for stream chunks (same signature as existing callback)
typedef void (*ChatProxyStreamChunkCallback)(const struct ChatStreamChunk* chunk, void* user_data);

// Maximum concurrent streams per multi handle
#define PROXY_MULTI_MAX_STREAMS 1024

// Chunk queue element for streaming
typedef struct StreamChunkNode {
    char* json_data;                 // JSON string of the chunk
    size_t data_len;                 // Length of data
    struct StreamChunkNode* next;    // Next chunk in queue
} StreamChunkNode;

// Thread-safe chunk queue for a single stream
typedef struct {
    StreamChunkNode* head;           // First chunk (consumer side)
    StreamChunkNode* tail;           // Last chunk (producer side)
    size_t count;                    // Number of queued chunks
    pthread_mutex_t mutex;           // Queue lock
} StreamChunkQueue;

// Active streaming request context
typedef struct MultiStreamContext {
    // Identification
    char* request_id;                // Request ID for logging
    char* engine_name;               // Engine name for logging
    
    // LWS connection info
    struct lws* wsi;                 // WebSocket connection
    void* session_data;              // Session data (WebSocketSessionData*)
    volatile bool* connection_valid; // Connection valid flag pointer
    volatile bool* stream_active;    // Stream active flag pointer
    
    // CURL easy handle
    CURL* easy_handle;               // CURL easy handle for this stream
    struct curl_slist* headers;      // Request headers
    char* request_body;              // Owned copy of request JSON body
    
    // Stream state
    StreamChunkQueue chunk_queue;    // Thread-safe chunk queue
    bool stream_completed;           // Stream completion flag
    char* finish_reason;             // Final finish reason
    int chunk_index;                 // Current chunk index
    bool first_chunk_logged;         // First chunk TTFB logged
    struct timespec start_time;      // Stream start time
    
    // Callback for chunks (passed through to CURL context)
    ChatProxyStreamChunkCallback chunk_callback;
    void* user_data;                 // User data for chunk callback
    
    // Callback for stream completion
    void (*completion_callback)(void* user_data, bool success);
    void* completion_user_data;      // User data for completion callback
    
    // Linked list pointers
    struct MultiStreamContext* next;
    struct MultiStreamContext* prev;
} MultiStreamContext;

// Multi-curl streaming manager
typedef struct {
    CURLM* multi_handle;             // CURL multi handle
    struct lws_context* lws_context; // LWS context for integration
    MultiStreamContext* active_streams; // Active streams list
    pthread_mutex_t streams_mutex;   // Protects active_streams list
    int max_host_connections;        // Max connections per host
    int max_total_connections;       // Max total connections
    bool initialized;                // Initialization flag
    // Worker thread for driving curl_multi_perform
    pthread_t worker_thread;         // Background thread to drive multi handle
    volatile bool shutdown_requested; // Shutdown flag for worker thread
} MultiStreamManager;

// ============================================================================
// Manager Lifecycle
// ============================================================================

/**
 * Initialize the multi-stream manager
 * @param manager Manager to initialize
 * @param lws_context LWS context for integration (can be NULL for standalone)
 * @return true on success
 */
bool chat_proxy_multi_init(MultiStreamManager* manager, struct lws_context* lws_context);

/**
 * Cleanup the multi-stream manager
 * @param manager Manager to cleanup
 */
void chat_proxy_multi_cleanup(MultiStreamManager* manager);

/**
 * Process CURL multi activity (call from event loop)
 * @param manager Manager instance
 * @return true if there are active streams, false if idle
 */
bool chat_proxy_multi_perform(MultiStreamManager* manager);

/**
 * Get the CURL multi handle for external integration
 * @param manager Manager instance
 * @return CURLM handle
 */
CURLM* chat_proxy_multi_get_handle(const MultiStreamManager* manager);

// ============================================================================
// Stream Management
// ============================================================================

/**
 * Start a new streaming request
 * @param manager Manager instance
 * @param engine Engine configuration
 * @param request_json Request body
 * @param wsi WebSocket connection
 * @param session_data Session data
 * @param connection_valid Pointer to connection valid flag
 * @param stream_active Pointer to stream active flag
 * @param chunk_callback Callback for each chunk
 * @param user_data User data for callback
 * @param completion_callback Optional completion callback
 * @param completion_user_data User data for completion callback
 * @return Stream context on success, NULL on failure
 */
MultiStreamContext* chat_proxy_multi_stream_start(
    MultiStreamManager* manager,
    const ChatEngineConfig* engine,
    const char* request_json,
    struct lws* wsi,
    void* session_data,
    volatile bool* connection_valid,
    volatile bool* stream_active,
    ChatProxyStreamChunkCallback chunk_callback,
    void* user_data,
    void (*completion_callback)(void* user_data, bool success),
    void* completion_user_data
);

/**
 * Stop a streaming request
 * @param manager Manager instance
 * @param context Stream context to stop
 */
void chat_proxy_multi_stream_stop(MultiStreamManager* manager, MultiStreamContext* context);

/**
 * Handle CURL socket activity (call from LWS when socket is ready)
 * @param manager Manager instance
 * @param sockfd Socket file descriptor
 * @param action Action bitmask (CURL_POLL_IN, CURL_POLL_OUT, etc.)
 */
void chat_proxy_multi_socket_action(MultiStreamManager* manager, int sockfd, int action);

/**
 * Handle CURL timer (call from LWS when timer fires)
 * @param manager Manager instance (const - not modified)
 * @param timeout_ms Timeout in milliseconds
 */
void chat_proxy_multi_timer_callback(const MultiStreamManager* manager, long timeout_ms);

// ============================================================================
// Queue Operations (for LWS writable callback)
// ============================================================================

/**
 * Drain chunk queue and write to WebSocket
 * Must be called from LWS service thread (writable callback)
 * @param context Stream context
 * @return Number of chunks written, -1 on error
 */
int chat_proxy_multi_drain_queue(MultiStreamContext* context);

/**
 * Check if stream has queued data ready to write
 * @param context Stream context
 * @return true if queue has data
 */
bool chat_proxy_multi_has_queued_data(const MultiStreamContext* context);

/**
 * Request writable callback from LWS
 * @param context Stream context
 */
void chat_proxy_multi_request_writable(MultiStreamContext* context);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get stream statistics
 * @param context Stream context
 * @param chunks_queued Output: chunks in queue
 * @param chunks_sent Output: total chunks sent
 */
void chat_proxy_multi_get_stats(const MultiStreamContext* context, 
                                size_t* chunks_queued, size_t* chunks_sent);

/**
 * Check if manager has active streams
 * @param manager Manager instance
 * @return true if has active streams
 */
bool chat_proxy_multi_has_active_streams(const MultiStreamManager* manager);

/**
 * Get count of active streams
 * @param manager Manager instance
 * @return Number of active streams
 */
size_t chat_proxy_multi_get_stream_count(const MultiStreamManager* manager);

#endif // PROXY_MULTI_H
