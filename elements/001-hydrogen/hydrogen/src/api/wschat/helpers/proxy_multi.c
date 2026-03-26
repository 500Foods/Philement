/*
 * Chat Proxy Multi - Thread-Safe Streaming with libcurl multi Interface
 *
 * Provides non-blocking, event-driven streaming proxy using libcurl's multi
 * socket API. Integrates with LWS event loop for thread-safe WebSocket writes.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "proxy_multi.h"
#include "resp_parser.h"
#include "proxy.h"

// WebSocket message functions for ws_write_raw_data
#include <src/websocket/websocket_server_message.h>

// System includes
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

// ============================================================================
// Internal Constants
// ============================================================================

// Initial chunk queue capacity
#define INITIAL_CHUNK_CAPACITY 32

// Maximum chunks per queue before backpressure
#define MAX_CHUNKS_PER_QUEUE 1024

// ============================================================================
// Static Variables
// ============================================================================

// Global multi stream manager (singleton for simplicity)
static MultiStreamManager* g_multi_manager = NULL;

// ============================================================================
// Chunk Queue Implementation
// ============================================================================

// Initialize chunk queue
static void chunk_queue_init(StreamChunkQueue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
}

// Destroy chunk queue
static void chunk_queue_destroy(StreamChunkQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    
    StreamChunkNode* node = queue->head;
    while (node) {
        StreamChunkNode* next = node->next;
        free(node->json_data);
        free(node);
        node = next;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
}

// Enqueue a chunk (producer thread - CURL callback)
static bool chunk_queue_enqueue(StreamChunkQueue* queue, const char* json_data, size_t data_len) {
    pthread_mutex_lock(&queue->mutex);
    
    // Check queue size limit (backpressure)
    if (queue->count >= MAX_CHUNKS_PER_QUEUE) {
        pthread_mutex_unlock(&queue->mutex);
        log_this(SR_CHAT, "Chunk queue full, dropping chunk", LOG_LEVEL_ALERT, 0);
        return false;
    }
    
    // Allocate node
    StreamChunkNode* node = (StreamChunkNode*)malloc(sizeof(StreamChunkNode));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    
    // Allocate and copy data
    node->json_data = (char*)malloc(data_len + 1);
    if (!node->json_data) {
        free(node);
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    
    memcpy(node->json_data, json_data, data_len);
    node->json_data[data_len] = '\0';
    node->data_len = data_len;
    node->next = NULL;
    
    // Add to queue
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->count++;
    
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Dequeue a chunk (consumer thread - LWS callback)
static StreamChunkNode* chunk_queue_dequeue(StreamChunkQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    
    StreamChunkNode* node = queue->head;
    if (node) {
        queue->head = node->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
        queue->count--;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return node;
}

// Check if queue has data
static bool chunk_queue_has_data(const StreamChunkQueue* queue) {
    // Note: This is a snapshot check, actual data may change immediately after
    return queue->head != NULL;
}

// Get queue count
static size_t chunk_queue_get_count(const StreamChunkQueue* queue) {
    return queue->count;
}

// ============================================================================
// CURL Callbacks
// ============================================================================

// Forward declaration for chunk callback
typedef void (*ChunkCallbackFunc)(const ChatStreamChunk* chunk, void* user_data);

// Per-stream context for CURL callbacks (stored in CURL easy handle)
typedef struct {
    MultiStreamContext* stream_ctx;
    ChunkCallbackFunc chunk_callback;
    void* user_data;
    char* line_buffer;
    size_t line_buffer_len;
    size_t line_buffer_capacity;
    size_t bytes_received;
    size_t chunks_processed;
    bool first_data_logged;
} CurlStreamContext;

// CURL write callback for streaming
static size_t multi_stream_write_callback(const void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    CurlStreamContext* curl_ctx = (CurlStreamContext*)userp;
    
    if (!curl_ctx || !curl_ctx->stream_ctx) {
        return 0;
    }
    
    MultiStreamContext* stream_ctx = curl_ctx->stream_ctx;
    
    // DEBUG: Log raw data received
    if (realsize > 0 && realsize < 512) {
        char debug_buf[512];
        memcpy(debug_buf, contents, realsize < 511 ? realsize : 511);
        debug_buf[realsize < 511 ? realsize : 511] = '\0';
        log_this(SR_CHAT, "Multi-stream RAW (%zu bytes): '%s'", LOG_LEVEL_DEBUG, 2, realsize, debug_buf);
    } else if (realsize >= 512) {
        log_this(SR_CHAT, "Multi-stream RAW (%zu bytes) - too large to display", LOG_LEVEL_DEBUG, 1, realsize);
    }
    
    // Track bytes received
    curl_ctx->bytes_received += realsize;
    
    // Log first data arrival (TTFB diagnostic)
    if (!curl_ctx->first_data_logged && realsize > 0) {
        curl_ctx->first_data_logged = true;
        log_this(SR_CHAT, "Multi-stream first data received: %zu bytes (TTFB)", LOG_LEVEL_DEBUG, 1, realsize);
    }
    
    // Check connection validity
    if (stream_ctx->connection_valid && !*stream_ctx->connection_valid) {
        return 0;  // Stop receiving data
    }
    
    // Append incoming data to line buffer
    const char* data = (const char*)contents;
    size_t pos = 0;
    
    while (pos < realsize) {
        // Look for newline
        size_t remaining = realsize - pos;
        const char* newline = memchr(data + pos, '\n', remaining);
        size_t chunk_len = newline ? (size_t)(newline - (data + pos)) : remaining;
        
        // Ensure line buffer has enough capacity
        size_t needed = curl_ctx->line_buffer_len + chunk_len + 1;
        if (needed > curl_ctx->line_buffer_capacity) {
            size_t new_capacity = curl_ctx->line_buffer_capacity * 2;
            while (new_capacity < needed) new_capacity *= 2;
            char* new_buffer = realloc(curl_ctx->line_buffer, new_capacity);
            if (!new_buffer) {
                return 0;
            }
            curl_ctx->line_buffer = new_buffer;
            curl_ctx->line_buffer_capacity = new_capacity;
        }
        
        // Copy chunk
        memcpy(curl_ctx->line_buffer + curl_ctx->line_buffer_len, data + pos, chunk_len);
        curl_ctx->line_buffer_len += chunk_len;
        curl_ctx->line_buffer[curl_ctx->line_buffer_len] = '\0';
        
        if (newline) {
            // Complete line, parse and queue it
            ChatStreamChunk* chunk = chat_stream_chunk_parse(curl_ctx->line_buffer);
            if (chunk) {
                curl_ctx->chunks_processed++;
                
                // Build JSON for queue
                json_t* response = json_object();
                json_object_set_new(response, "type", json_string("chat_chunk"));
                if (stream_ctx->request_id) {
                    json_object_set_new(response, "id", json_string(stream_ctx->request_id));
                }
                
                json_t* chunk_json = json_object();
                json_object_set_new(chunk_json, "content", json_string(chunk->content ? chunk->content : ""));
                if (chunk->reasoning_content) {
                    json_object_set_new(chunk_json, "reasoning_content", json_string(chunk->reasoning_content));
                }
                if (chunk->model) json_object_set_new(chunk_json, "model", json_string(chunk->model));
                json_object_set_new(chunk_json, "index", json_integer(stream_ctx->chunk_index));
                if (chunk->finish_reason) json_object_set_new(chunk_json, "finish_reason", json_string(chunk->finish_reason));
                json_object_set_new(response, "chunk", chunk_json);
                
                char* json_str = json_dumps(response, JSON_COMPACT);
                json_decref(response);
                
                if (json_str) {
                    // Enqueue chunk for LWS thread
                    chunk_queue_enqueue(&stream_ctx->chunk_queue, json_str, strlen(json_str));
                    
                    // Request writable callback from LWS
                    chat_proxy_multi_request_writable(stream_ctx);
                    
                    free(json_str);
                }
                
                stream_ctx->chunk_index++;
                chat_stream_chunk_destroy(chunk);
            }
            // Reset line buffer
            curl_ctx->line_buffer_len = 0;
            curl_ctx->line_buffer[0] = '\0';
            pos += chunk_len + 1;
        } else {
            // No newline yet, wait for more data
            pos += chunk_len;
        }
    }
    
    return realsize;
}

// CURL debug callback for connection diagnostics
// cppcheck-suppress constParameterCallback - CURL callback signature is fixed
static int multi_stream_debug_callback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr) {
    (void)handle;
    (void)userptr;
    
    if (type == CURLINFO_TEXT && size > 0) {
        char debug_buf[256];
        size_t len = size < 255 ? size : 255;
        memcpy(debug_buf, data, len);
        debug_buf[len] = '\0';
        
        // Strip trailing newline
        while (len > 0 && (debug_buf[len-1] == '\n' || debug_buf[len-1] == '\r')) {
            debug_buf[--len] = '\0';
        }
        
        // Log interesting connection events
        if (strstr(debug_buf, "Connected to") || 
            strstr(debug_buf, "TLS") ||
            strstr(debug_buf, "SSL")) {
            log_this(SR_CHAT, "Multi CURL: %s", LOG_LEVEL_DEBUG, 1, debug_buf);
        }
    }
    
    return 0;
}

// CURL socket callback (called when socket state changes)
static int multi_socket_callback(CURL* easy, int sockfd, int action, void* userp, void* sockp) {
    (void)easy;
    (void)sockp;
    
    MultiStreamManager* manager = (MultiStreamManager*)userp;
    
    if (!manager || !manager->lws_context) {
        return 0;
    }
    
    // In a full LWS integration, we would:
    // 1. Use lws_sock_file_descriptor_type for the socket
    // 2. Call lws_evlib_wait or similar to integrate with LWS poll
    
    // For now, we rely on curl_multi_perform() in the perform loop
    // which handles socket activity without explicit LWS integration
    
    (void)sockfd;
    (void)action;
    
    return 0;
}

// CURL timer callback (called when timeout changes)
static int multi_timer_callback(CURLM* multi, long timeout_ms, void* userp) {
    (void)multi;
    
    MultiStreamManager* manager = (MultiStreamManager*)userp;
    
    if (!manager) {
        return 0;
    }
    
    // Timer management would be handled by the event loop
    // For our simple implementation, we use polling in perform()
    
    (void)timeout_ms;
    
    return 0;
}

// Forward declaration for worker thread
static void* multi_worker_thread(void* arg);

// ============================================================================
// Manager Implementation
// ============================================================================

bool chat_proxy_multi_init(MultiStreamManager* manager, struct lws_context* lws_context) {
    if (!manager || manager->initialized) {
        return false;
    }
    
    // Initialize CURL multi handle
    manager->multi_handle = curl_multi_init();
    if (!manager->multi_handle) {
        log_this(SR_CHAT, "Failed to initialize CURL multi handle", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    manager->lws_context = lws_context;
    manager->active_streams = NULL;
    pthread_mutex_init(&manager->streams_mutex, NULL);
    manager->max_host_connections = 50;  // Default per-provider limit
    manager->max_total_connections = 200;
    manager->shutdown_requested = false;
    
    // Configure multi handle
    curl_multi_setopt(manager->multi_handle, CURLMOPT_SOCKETFUNCTION, multi_socket_callback);
    curl_multi_setopt(manager->multi_handle, CURLMOPT_SOCKETDATA, manager);
    curl_multi_setopt(manager->multi_handle, CURLMOPT_TIMERFUNCTION, multi_timer_callback);
    curl_multi_setopt(manager->multi_handle, CURLMOPT_TIMERDATA, manager);
    curl_multi_setopt(manager->multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, (long)manager->max_host_connections);
    curl_multi_setopt(manager->multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long)manager->max_total_connections);
    
    manager->initialized = true;
    g_multi_manager = manager;
    
    log_this(SR_CHAT, "Multi-stream manager initialized (max %d per host, %d total)", 
             LOG_LEVEL_STATE, 2, manager->max_host_connections, manager->max_total_connections);
    
    // Start worker thread to drive the multi handle
    if (pthread_create(&manager->worker_thread, NULL, multi_worker_thread, manager) != 0) {
        log_this(SR_CHAT, "Failed to create multi-stream worker thread", LOG_LEVEL_ERROR, 0);
        curl_multi_cleanup(manager->multi_handle);
        manager->initialized = false;
        return false;
    }
    
    log_this(SR_CHAT, "Multi-stream worker thread started", LOG_LEVEL_STATE, 0);
    
    return true;
}

// Worker thread function that drives the multi handle
static void* multi_worker_thread(void* arg) {
    MultiStreamManager* manager = (MultiStreamManager*)arg;
    
    log_this(SR_CHAT, "Multi-stream worker thread started", LOG_LEVEL_DEBUG, 0);
    
    while (!manager->shutdown_requested) {
        // Process any pending transfers
        if (manager->initialized && chat_proxy_multi_perform(manager)) {
            // There are active transfers, sleep briefly
            usleep(10000);  // 10ms polling interval
        } else {
            // No active transfers, sleep longer
            usleep(50000);  // 50ms when idle
        }
    }
    
    log_this(SR_CHAT, "Multi-stream worker thread stopped", LOG_LEVEL_DEBUG, 0);
    return NULL;
}

void chat_proxy_multi_cleanup(MultiStreamManager* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    pthread_mutex_lock(&manager->streams_mutex);
    
    // Stop all active streams
    MultiStreamContext* stream = manager->active_streams;
    while (stream) {
        MultiStreamContext* next = stream->next;
        
        if (stream->easy_handle) {
            curl_multi_remove_handle(manager->multi_handle, stream->easy_handle);
            curl_easy_cleanup(stream->easy_handle);
        }
        if (stream->headers) {
            curl_slist_free_all(stream->headers);
        }
        
        // Free CURL stream context
        // Note: We need to get it from CURL private data
        void* priv_data = NULL;
        curl_easy_getinfo(stream->easy_handle, CURLINFO_PRIVATE, &priv_data);
        if (priv_data) {
            CurlStreamContext* curl_ctx = (CurlStreamContext*)priv_data;
            free(curl_ctx->line_buffer);
            free(curl_ctx);
        }
        
        chunk_queue_destroy(&stream->chunk_queue);
        free(stream->request_id);
        free(stream->engine_name);
        free(stream->finish_reason);
        free(stream->request_body);
        free(stream);
        
        stream = next;
    }
    
    manager->active_streams = NULL;
    
    pthread_mutex_unlock(&manager->streams_mutex);
    pthread_mutex_destroy(&manager->streams_mutex);
    
    // Cleanup CURL multi
    curl_multi_cleanup(manager->multi_handle);
    
    manager->initialized = false;
    if (g_multi_manager == manager) {
        g_multi_manager = NULL;
    }
    
    log_this(SR_CHAT, "Multi-stream manager cleaned up", LOG_LEVEL_DEBUG, 0);
}

bool chat_proxy_multi_perform(MultiStreamManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }
    
    int still_running = 0;
    CURLMcode mc = curl_multi_perform(manager->multi_handle, &still_running);
    
    if (mc != CURLM_OK) {
        log_this(SR_CHAT, "curl_multi_perform error: %s", LOG_LEVEL_ERROR, 1, curl_multi_strerror(mc));
        return still_running > 0;
    }
    
    // Check for completed transfers
    CURLMsg* msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(manager->multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            CURLcode res = msg->data.result;
            
            // Log CURL result and HTTP status for debugging
            long http_code = 0;
            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &http_code);
            log_this(SR_CHAT, "Multi-stream transfer complete: CURL result=%d (%s), HTTP %ld", LOG_LEVEL_DEBUG, 3,
                     (int)res, curl_easy_strerror(res), http_code);
            
            // Get stream context from CURL private data
            void* priv_data = NULL;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &priv_data);
            
            if (priv_data) {
                CurlStreamContext* curl_ctx = (CurlStreamContext*)priv_data;
                MultiStreamContext* stream_ctx = curl_ctx->stream_ctx;
                
                if (stream_ctx) {
                    // Process any remaining data in line buffer
                    if (curl_ctx->line_buffer_len > 0 && res == CURLE_OK) {
                        ChatStreamChunk* chunk = chat_stream_chunk_parse(curl_ctx->line_buffer);
                        if (chunk) {
                            // Build and enqueue final chunk
                            json_t* response = json_object();
                            json_object_set_new(response, "type", json_string("chat_chunk"));
                            if (stream_ctx->request_id) {
                                json_object_set_new(response, "id", json_string(stream_ctx->request_id));
                            }
                            
                            json_t* chunk_json = json_object();
                            json_object_set_new(chunk_json, "content", json_string(chunk->content ? chunk->content : ""));
                            if (chunk->model) json_object_set_new(chunk_json, "model", json_string(chunk->model));
                            json_object_set_new(chunk_json, "index", json_integer(stream_ctx->chunk_index));
                            if (chunk->finish_reason) {
                                json_object_set_new(chunk_json, "finish_reason", json_string(chunk->finish_reason));
                            }
                            json_object_set_new(response, "chunk", chunk_json);
                            
                            char* json_str = json_dumps(response, JSON_COMPACT);
                            json_decref(response);
                            
                            if (json_str) {
                                chunk_queue_enqueue(&stream_ctx->chunk_queue, json_str, strlen(json_str));
                                chat_proxy_multi_request_writable(stream_ctx);
                                free(json_str);
                            }
                            
                            stream_ctx->chunk_index++;
                            chat_stream_chunk_destroy(chunk);
                        }
                    }
                    
                    // Mark stream as completed
                    stream_ctx->stream_completed = true;
                    
                    // Build and enqueue done message
                    json_t* done_response = json_object();
                    json_object_set_new(done_response, "type", json_string("chat_done"));
                    if (stream_ctx->request_id) {
                        json_object_set_new(done_response, "id", json_string(stream_ctx->request_id));
                    }
                    
                    json_t* result = json_object();
                    json_object_set_new(result, "content", json_string(""));
                    if (stream_ctx->engine_name) {
                        json_object_set_new(result, "model", json_string(stream_ctx->engine_name));
                    }
                    json_object_set_new(result, "finish_reason", 
                                       json_string(stream_ctx->finish_reason ? stream_ctx->finish_reason : "stop"));
                    
                    // Calculate timing
                    struct timespec end_time;
                    clock_gettime(CLOCK_MONOTONIC, &end_time);
                    double elapsed_ms = (double)(end_time.tv_sec - stream_ctx->start_time.tv_sec) * 1000.0 +
                                       (double)(end_time.tv_nsec - stream_ctx->start_time.tv_nsec) / 1000000.0;
                    json_object_set_new(result, "response_time_ms", json_real(elapsed_ms));
                    
                    json_object_set_new(done_response, "result", result);
                    
                    char* done_json = json_dumps(done_response, JSON_COMPACT);
                    json_decref(done_response);
                    
                    if (done_json) {
                        chunk_queue_enqueue(&stream_ctx->chunk_queue, done_json, strlen(done_json));
                        chat_proxy_multi_request_writable(stream_ctx);
                        free(done_json);
                    }
                    
                    // Log completion
                    log_this(SR_CHAT, "Multi-stream complete: %zu chunks, %.0fms, finish=%s",
                             LOG_LEVEL_STATE, 3,
                             curl_ctx->chunks_processed,
                             elapsed_ms,
                             stream_ctx->finish_reason ? stream_ctx->finish_reason : "stop");
                    
                    // Remove from CURL multi
                    curl_multi_remove_handle(manager->multi_handle, easy);
                    
                    // Cleanup CURL context
                    free(curl_ctx->line_buffer);
                    free(curl_ctx);
                    
                    // Update session flags
                    if (stream_ctx->stream_active) {
                        *stream_ctx->stream_active = false;
                    }
                    
                    // Remove from active streams list
                    pthread_mutex_lock(&manager->streams_mutex);
                    if (stream_ctx->prev) {
                        stream_ctx->prev->next = stream_ctx->next;
                    } else {
                        manager->active_streams = stream_ctx->next;
                    }
                    if (stream_ctx->next) {
                        stream_ctx->next->prev = stream_ctx->prev;
                    }
                    pthread_mutex_unlock(&manager->streams_mutex);
                    
                    // Free stream context
                    free(stream_ctx->request_id);
                    free(stream_ctx->engine_name);
                    free(stream_ctx->finish_reason);
                    free(stream_ctx->request_body);
                    free(stream_ctx);
                }
            }
        }
    }
    
    return still_running > 0;
}

CURLM* chat_proxy_multi_get_handle(const MultiStreamManager* manager) {
    return manager ? manager->multi_handle : NULL;
}

// ============================================================================
// Stream Management Implementation
// ============================================================================

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
) {
    if (!manager || !manager->initialized || !engine || !request_json || !wsi) {
        return NULL;
    }
    
    // Allocate stream context
    MultiStreamContext* stream_ctx = (MultiStreamContext*)calloc(1, sizeof(MultiStreamContext));
    if (!stream_ctx) {
        return NULL;
    }
    
    // Initialize stream context
    stream_ctx->request_id = NULL;  // Will be set by caller if needed
    stream_ctx->engine_name = strdup(engine->name[0] ? engine->name : "unknown");
    stream_ctx->wsi = wsi;
    stream_ctx->session_data = session_data;
    stream_ctx->connection_valid = connection_valid;
    stream_ctx->stream_active = stream_active;
    stream_ctx->chunk_callback = chunk_callback;
    stream_ctx->user_data = user_data;
    stream_ctx->completion_callback = completion_callback;
    stream_ctx->completion_user_data = completion_user_data;
    stream_ctx->stream_completed = false;
    stream_ctx->finish_reason = NULL;
    stream_ctx->chunk_index = 0;
    stream_ctx->first_chunk_logged = false;
    clock_gettime(CLOCK_MONOTONIC, &stream_ctx->start_time);
    
    // Initialize chunk queue
    chunk_queue_init(&stream_ctx->chunk_queue);
    
    // Allocate CURL easy handle
    stream_ctx->easy_handle = curl_easy_init();
    if (!stream_ctx->easy_handle) {
        log_this(SR_CHAT, "Failed to initialize CURL easy handle for multi-stream", LOG_LEVEL_ERROR, 0);
        chunk_queue_destroy(&stream_ctx->chunk_queue);
        free(stream_ctx->engine_name);
        free(stream_ctx);
        return NULL;
    }
    
    // Allocate CURL stream context
    CurlStreamContext* curl_ctx = (CurlStreamContext*)calloc(1, sizeof(CurlStreamContext));
    if (!curl_ctx) {
        curl_easy_cleanup(stream_ctx->easy_handle);
        chunk_queue_destroy(&stream_ctx->chunk_queue);
        free(stream_ctx->engine_name);
        free(stream_ctx);
        return NULL;
    }
    
    curl_ctx->stream_ctx = stream_ctx;
    curl_ctx->chunk_callback = chunk_callback;
    curl_ctx->user_data = user_data;
    curl_ctx->line_buffer_capacity = 4096;
    curl_ctx->line_buffer = (char*)malloc(curl_ctx->line_buffer_capacity);
    if (!curl_ctx->line_buffer) {
        free(curl_ctx);
        curl_easy_cleanup(stream_ctx->easy_handle);
        chunk_queue_destroy(&stream_ctx->chunk_queue);
        free(stream_ctx->engine_name);
        free(stream_ctx);
        return NULL;
    }
    curl_ctx->line_buffer[0] = '\0';
    curl_ctx->line_buffer_len = 0;
    curl_ctx->bytes_received = 0;
    curl_ctx->chunks_processed = 0;
    curl_ctx->first_data_logged = false;
    
    // Store CURL context in CURL private data
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_PRIVATE, curl_ctx);
    
    // Build headers
    stream_ctx->headers = NULL;
    stream_ctx->headers = curl_slist_append(stream_ctx->headers, "Content-Type: application/json");
    stream_ctx->headers = curl_slist_append(stream_ctx->headers, "Accept: text/event-stream");
    
    // Log provider type for debugging
    log_this(SR_CHAT, "Multi-stream provider type: %d (OpenAI=%d, Anthropic=%d)", LOG_LEVEL_DEBUG, 3,
             engine->provider, CEC_PROVIDER_OPENAI, CEC_PROVIDER_ANTHROPIC);
    
    if (engine->provider == CEC_PROVIDER_ANTHROPIC) {
        char auth_header[CEC_MAX_KEY_LEN + 32];
        snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", engine->api_key);
        stream_ctx->headers = curl_slist_append(stream_ctx->headers, auth_header);
        stream_ctx->headers = curl_slist_append(stream_ctx->headers, "anthropic-version: 2023-06-01");
        log_this(SR_CHAT, "Multi-stream using Anthropic headers", LOG_LEVEL_DEBUG, 0);
    } else {
        char auth_header[CEC_MAX_KEY_LEN + 32];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", engine->api_key);
        stream_ctx->headers = curl_slist_append(stream_ctx->headers, auth_header);
        log_this(SR_CHAT, "Multi-stream using OpenAI headers", LOG_LEVEL_DEBUG, 0);
    }
    
    // Log the API URL for debugging
    log_this(SR_CHAT, "Multi-stream API URL: %s", LOG_LEVEL_DEBUG, 1, engine->api_url);
    
    // Configure CURL options
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_URL, engine->api_url);
    
    // DEBUG: Log the request body being sent
    size_t request_len = strlen(request_json);
    log_this(SR_CHAT, "Multi-stream sending request (%zu bytes): %.200s%s", LOG_LEVEL_DEBUG, 3,
             request_len, request_json, request_len > 200 ? "..." : "");
    
    // Make a copy of the request body that we own (original may be freed)
    stream_ctx->request_body = strdup(request_json);
    if (!stream_ctx->request_body) {
        log_this(SR_CHAT, "Failed to copy request body", LOG_LEVEL_ERROR, 0);
        free(curl_ctx->line_buffer);
        free(curl_ctx);
        curl_easy_cleanup(stream_ctx->easy_handle);
        chunk_queue_destroy(&stream_ctx->chunk_queue);
        free(stream_ctx->engine_name);
        free(stream_ctx);
        return NULL;
    }
    
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_POSTFIELDS, stream_ctx->request_body);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_POSTFIELDSIZE, (long)request_len);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_HTTPHEADER, stream_ctx->headers);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_WRITEFUNCTION, multi_stream_write_callback);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_WRITEDATA, curl_ctx);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_TIMEOUT, 600L);  // 10 minutes for streaming
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_USERAGENT, "Hydrogen-Chat-Proxy-Multi/1.0");
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_NOSIGNAL, 1L);  // Thread-safe
    
    // Enable debug callback
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_DEBUGFUNCTION, multi_stream_debug_callback);
    curl_easy_setopt(stream_ctx->easy_handle, CURLOPT_VERBOSE, 1L);
    
    // Add to multi handle
    CURLMcode mc = curl_multi_add_handle(manager->multi_handle, stream_ctx->easy_handle);
    if (mc != CURLM_OK) {
        log_this(SR_CHAT, "Failed to add handle to multi: %s", LOG_LEVEL_ERROR, 1, curl_multi_strerror(mc));
        free(curl_ctx->line_buffer);
        free(curl_ctx);
        curl_slist_free_all(stream_ctx->headers);
        curl_easy_cleanup(stream_ctx->easy_handle);
        chunk_queue_destroy(&stream_ctx->chunk_queue);
        free(stream_ctx->engine_name);
        free(stream_ctx->request_body);
        free(stream_ctx);
        return NULL;
    }
    
    // Add to active streams list
    pthread_mutex_lock(&manager->streams_mutex);
    stream_ctx->next = manager->active_streams;
    stream_ctx->prev = NULL;
    if (manager->active_streams) {
        manager->active_streams->prev = stream_ctx;
    }
    manager->active_streams = stream_ctx;
    pthread_mutex_unlock(&manager->streams_mutex);
    
    // Mark stream as active
    if (stream_ctx->stream_active) {
        *stream_ctx->stream_active = true;
    }
    
    log_this(SR_CHAT, "Multi-stream started: %s", LOG_LEVEL_DEBUG, 1, engine->name);
    
    return stream_ctx;
}

void chat_proxy_multi_stream_stop(MultiStreamManager* manager, MultiStreamContext* context) {
    if (!manager || !context) {
        return;
    }
    
    // Remove from CURL multi
    if (context->easy_handle) {
        curl_multi_remove_handle(manager->multi_handle, context->easy_handle);
        curl_easy_cleanup(context->easy_handle);
    }
    
    // Get and free CURL context
    void* priv_data = NULL;
    if (context->easy_handle) {
        curl_easy_getinfo(context->easy_handle, CURLINFO_PRIVATE, &priv_data);
    }
    if (priv_data) {
        CurlStreamContext* curl_ctx = (CurlStreamContext*)priv_data;
        free(curl_ctx->line_buffer);
        free(curl_ctx);
    }
    
    // Free headers
    if (context->headers) {
        curl_slist_free_all(context->headers);
    }
    
    // Destroy chunk queue
    chunk_queue_destroy(&context->chunk_queue);
    
    // Remove from active streams list
    pthread_mutex_lock(&manager->streams_mutex);
    if (context->prev) {
        context->prev->next = context->next;
    } else {
        manager->active_streams = context->next;
    }
    if (context->next) {
        context->next->prev = context->prev;
    }
    pthread_mutex_unlock(&manager->streams_mutex);
    
    // Update session flags
    if (context->stream_active) {
        *context->stream_active = false;
    }
    
    // Call completion callback if set
    if (context->completion_callback) {
        context->completion_callback(context->completion_user_data, false);
    }
    
    // Free resources
    free(context->request_id);
    free(context->engine_name);
    free(context->finish_reason);
    free(context->request_body);
    free(context);
}

void chat_proxy_multi_socket_action(MultiStreamManager* manager, int sockfd, int action) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    int running_handles = 0;
    curl_multi_socket_action(manager->multi_handle, (curl_socket_t)sockfd, action, &running_handles);
}

void chat_proxy_multi_timer_callback(const MultiStreamManager* manager, long timeout_ms) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    // Timer handling would be done by the event loop
    (void)timeout_ms;
}

// ============================================================================
// Queue Operations Implementation
// ============================================================================

int chat_proxy_multi_drain_queue(MultiStreamContext* context) {
    if (!context) {
        return -1;
    }
    
    int chunks_written = 0;
    
    while (chunk_queue_has_data(&context->chunk_queue)) {
        StreamChunkNode* node = chunk_queue_dequeue(&context->chunk_queue);
        if (!node) {
            break;
        }
        
        // Check connection validity before writing
        if (context->connection_valid && !*context->connection_valid) {
            free(node->json_data);
            free(node);
            break;
        }
        
        // Write to WebSocket (this MUST be called from LWS service thread)
        int result = ws_write_raw_data(context->wsi, node->json_data, node->data_len);
        
        free(node->json_data);
        free(node);
        
        if (result != 0) {
            log_this(SR_WEBSOCKET_CHAT, "Multi-stream write failed", LOG_LEVEL_DEBUG, 0);
            break;
        }
        
        chunks_written++;
    }
    
    return chunks_written;
}

bool chat_proxy_multi_has_queued_data(const MultiStreamContext* context) {
    if (!context) {
        return false;
    }
    return chunk_queue_has_data(&context->chunk_queue);
}

void chat_proxy_multi_request_writable(MultiStreamContext* context) {
    if (!context || !context->wsi) {
        return;
    }
    
    // Request LWS writable callback
    // This will trigger handle_chat_writable() on the LWS service thread
    lws_callback_on_writable(context->wsi);
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

void chat_proxy_multi_get_stats(const MultiStreamContext* context, 
                                size_t* chunks_queued, size_t* chunks_sent) {
    if (!context) {
        return;
    }
    
    if (chunks_queued) {
        *chunks_queued = chunk_queue_get_count(&context->chunk_queue);
    }
    if (chunks_sent) {
        *chunks_sent = (size_t)context->chunk_index;
    }
}

bool chat_proxy_multi_has_active_streams(const MultiStreamManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }
    
    pthread_mutex_lock((pthread_mutex_t*)&manager->streams_mutex);
    bool has_streams = manager->active_streams != NULL;
    pthread_mutex_unlock((pthread_mutex_t*)&manager->streams_mutex);
    
    return has_streams;
}

size_t chat_proxy_multi_get_stream_count(const MultiStreamManager* manager) {
    if (!manager || !manager->initialized) {
        return 0;
    }
    
    size_t count = 0;
    
    pthread_mutex_lock((pthread_mutex_t*)&manager->streams_mutex);
    MultiStreamContext* stream = manager->active_streams;
    while (stream) {
        count++;
        stream = stream->next;
    }
    pthread_mutex_unlock((pthread_mutex_t*)&manager->streams_mutex);
    
    return count;
}
