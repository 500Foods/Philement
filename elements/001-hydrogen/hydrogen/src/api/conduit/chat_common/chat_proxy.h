/*
 * Chat Proxy - HTTP Client for AI API Requests
 *
 * Uses libcurl to send HTTP POST requests to external AI APIs.
 * Handles request timing, response collection, and error conditions.
 */

#ifndef CHAT_PROXY_H
#define CHAT_PROXY_H

// Project includes
#include <src/hydrogen.h>
#include <curl/curl.h>

// Local includes
#include "chat_engine_cache.h"
#include "chat_response_parser.h"

// Proxy result codes
typedef enum {
    CHAT_PROXY_OK = 0,              // Success
    CHAT_PROXY_ERROR_INIT,          // CURL initialization failed
    CHAT_PROXY_ERROR_CONNECT,       // Connection failed
    CHAT_PROXY_ERROR_TIMEOUT,       // Request timeout
    CHAT_PROXY_ERROR_HTTP_4XX,      // HTTP 4xx error
    CHAT_PROXY_ERROR_HTTP_5XX,      // HTTP 5xx error
    CHAT_PROXY_ERROR_MEMORY,        // Memory allocation failed
    CHAT_PROXY_ERROR_NETWORK,       // Network error
    CHAT_PROXY_ERROR_UNKNOWN        // Unknown error
} ChatProxyResultCode;

// Response data structure for CURL write callback
typedef struct ChatResponseBuffer {
    char* data;
    size_t size;
    size_t capacity;
} ChatResponseBuffer;

// Chat proxy result structure
typedef struct ChatProxyResult {
    ChatProxyResultCode code;       // Result code
    int http_status;                // HTTP status code
    char* response_body;            // Response body (JSON)
    size_t response_size;           // Response size in bytes
    double total_time_ms;           // Total request time in milliseconds
    char* error_message;            // Error message (if any)
} ChatProxyResult;

// Proxy configuration
typedef struct ChatProxyConfig {
    int connect_timeout_seconds;    // Connection timeout
    int request_timeout_seconds;    // Request timeout
    int max_retries;                // Maximum retry attempts
    bool verify_ssl;                // Verify SSL certificates
} ChatProxyConfig;

// Function prototypes

// Initialize proxy system
bool chat_proxy_init(void);
void chat_proxy_cleanup(void);

// Create and destroy proxy results
ChatProxyResult* chat_proxy_result_create(void);
void chat_proxy_result_destroy(ChatProxyResult* result);

// Main proxy function - send request to AI API
ChatProxyResult* chat_proxy_send_request(const ChatEngineConfig* engine,
                                         const char* request_json,
                                         const ChatProxyConfig* config);

// Retry logic with exponential backoff
ChatProxyResult* chat_proxy_send_with_retry(const ChatEngineConfig* engine,
                                             const char* request_json,
                                             const ChatProxyConfig* config);

// Multi-request structure for parallel execution
typedef struct ChatMultiRequest {
    const ChatEngineConfig* engine;   // Engine configuration
    const char* request_json;         // Request JSON body
    char* engine_name;                // Engine name (for result correlation)
} ChatMultiRequest;

// Multi-result structure for parallel execution
typedef struct ChatMultiResult {
    ChatProxyResult** results;        // Array of results (matches request order)
    size_t count;                     // Number of results
    double total_time_ms;             // Total time for all requests
    size_t success_count;             // Number of successful requests
    size_t failure_count;             // Number of failed requests
} ChatMultiResult;

// Create and destroy multi-result
ChatMultiResult* chat_multi_result_create(size_t count);
void chat_multi_result_destroy(ChatMultiResult* multi_result);

// Send multiple requests in parallel using curl_multi
// This executes requests to multiple AI engines simultaneously (fan-out pattern)
ChatMultiResult* chat_proxy_send_multi(const ChatMultiRequest* requests,
                                        size_t request_count,
                                        const ChatProxyConfig* config);

// Streaming callback for each parsed chunk
typedef void (*ChatProxyStreamChunkCallback)(const ChatStreamChunk* chunk, void* user_data);

// Send request in streaming mode, calling callback for each chunk
bool chat_proxy_send_stream(const ChatEngineConfig* engine,
                            const char* request_json,
                            const ChatProxyConfig* config,
                            ChatProxyStreamChunkCallback chunk_callback,
                            void* user_data);

// Utility functions
const char* chat_proxy_result_code_to_string(ChatProxyResultCode code);
bool chat_proxy_result_is_success(const ChatProxyResult* result);
bool chat_proxy_result_should_retry(const ChatProxyResult* result);

// Default configuration
ChatProxyConfig chat_proxy_get_default_config(void);

#endif // CHAT_PROXY_H
