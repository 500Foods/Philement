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
    
    // LWS connection info (NULL for REST SSE streams)
    struct lws* wsi;                 // WebSocket connection
    void* session_data;              // Session data (WebSocketSessionData*)
    volatile bool* connection_valid; // Connection valid flag pointer
    volatile bool* stream_active;    // Stream active flag pointer
    bool is_rest;                    // true for REST SSE (no LWS)
    
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
    
    // Context hashing stats (collected before provider call, attached to chat_done)
    bool has_context_hashing_stats;
    size_t ctx_hashes_used;
    size_t ctx_hashes_missed;
    size_t ctx_bandwidth_saved_bytes;
    double ctx_bandwidth_saved_percent;

    // Linked list pointers
    struct MultiStreamContext* next;
    struct MultiStreamContext* prev;
} MultiStreamContext;

// Internal CURL stream context (stored via CURLOPT_PRIVATE)
typedef struct {
    MultiStreamContext* stream_ctx;
    ChatProxyStreamChunkCallback chunk_callback;
    void* user_data;
    char* line_buffer;
    size_t line_buffer_len;
    size_t line_buffer_capacity;
    size_t bytes_received;
    size_t chunks_processed;
    bool first_data_logged;
    bool seen_done;
    char* post_done_buffer;
    size_t post_done_len;
    size_t post_done_capacity;
} CurlStreamContext;

// Chunk queue internal functions (implemented in proxy_mq.c)
void chunk_queue_init(StreamChunkQueue* queue);
void chunk_queue_destroy(StreamChunkQueue* queue);
bool chunk_queue_enqueue(StreamChunkQueue* queue, const char* json_data, size_t data_len);
StreamChunkNode* chunk_queue_dequeue(StreamChunkQueue* queue);
bool chunk_queue_has_data(const StreamChunkQueue* queue);
size_t chunk_queue_get_count(const StreamChunkQueue* queue);

// CURL callbacks (implemented in proxy_mc.c)
size_t multi_stream_write_callback(const void* contents, size_t size, size_t nmemb, void* userp);
int multi_stream_debug_callback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr);

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
    bool worker_thread_started;      // True only if worker_thread was spawned
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
 * Process a single completed CURL transfer (a CURLMSG_DONE result).
 * Exposed (non-static) so the Unity test framework can drive the transfer
 * completion logic directly without the full curl_multi event loop.
 * @param manager Manager instance
 * @param easy The CURL easy handle that completed
 * @param res The CURL result code for the transfer
 * @param http_code The HTTP response code (0 if unavailable)
 */
void chat_proxy_multi_handle_completed_transfer(MultiStreamManager* manager, CURL* easy, CURLcode res, long http_code);

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
 * Request writable callback from LWS
 * @param context Stream context
 */
void chat_proxy_multi_request_writable(MultiStreamContext* context);

// ============================================================================
// Utility Functions
// ============================================================================

void* chat_proxy_multi_worker_thread(void* arg);

#endif // PROXY_MULTI_H
