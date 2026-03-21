/*
 * Chat Proxy - HTTP Client for AI API Requests
 *
 * Uses libcurl to send HTTP POST requests to external AI APIs.
 * Handles request timing, response collection, and error conditions.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "chat_proxy.h"

// Default proxy configuration
#define DEFAULT_CONNECT_TIMEOUT_SECONDS 10
#define DEFAULT_REQUEST_TIMEOUT_SECONDS 60
#define DEFAULT_MAX_RETRIES 2
#define DEFAULT_VERIFY_SSL true

// Initial response buffer size (64KB)
#define INITIAL_RESPONSE_BUFFER_SIZE (64 * 1024)

// Maximum response size (8MB to prevent memory exhaustion)
#define MAX_RESPONSE_SIZE (8 * 1024 * 1024)

// CURL write callback for collecting response data
static size_t chat_proxy_write_callback(const void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    ChatResponseBuffer* buffer = (ChatResponseBuffer*)userp;

    // Check for maximum size
    if (buffer->size + realsize > MAX_RESPONSE_SIZE) {
        log_this(SR_CHAT, "Response exceeds maximum size limit", LOG_LEVEL_ERROR, 0);
        return 0;  // This will cause CURL to return an error
    }

    // Resize buffer if needed
    if (buffer->size + realsize > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        while (new_capacity < buffer->size + realsize) {
            new_capacity *= 2;
        }

        char* new_data = (char*)realloc(buffer->data, new_capacity);
        if (!new_data) {
            log_this(SR_CHAT, "Failed to allocate memory for response buffer", LOG_LEVEL_ERROR, 0);
            return 0;
        }

        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }

    // Append data
    memcpy(buffer->data + buffer->size, contents, realsize);
    buffer->size += realsize;
    buffer->data[buffer->size] = '\0';

    return realsize;
}

// Initialize proxy system
bool chat_proxy_init(void) {
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result != CURLE_OK) {
        log_this(SR_CHAT, "Failed to initialize CURL global: %s", LOG_LEVEL_ERROR, 1, curl_easy_strerror(result));
        return false;
    }
    log_this(SR_CHAT, "Chat proxy initialized (libcurl %s)", LOG_LEVEL_DEBUG, 1, curl_version());
    return true;
}

// Cleanup proxy system
void chat_proxy_cleanup(void) {
    curl_global_cleanup();
    log_this(SR_CHAT, "Chat proxy cleaned up", LOG_LEVEL_DEBUG, 0);
}

// Create proxy result
ChatProxyResult* chat_proxy_result_create(void) {
    ChatProxyResult* result = (ChatProxyResult*)calloc(1, sizeof(ChatProxyResult));
    if (!result) {
        return NULL;
    }
    result->code = CHAT_PROXY_ERROR_UNKNOWN;
    result->http_status = 0;
    result->response_body = NULL;
    result->response_size = 0;
    result->total_time_ms = 0.0;
    result->error_message = NULL;
    return result;
}

// Destroy proxy result
void chat_proxy_result_destroy(ChatProxyResult* result) {
    if (!result) return;
    free(result->response_body);
    free(result->error_message);
    free(result);
}

// Get default proxy configuration
ChatProxyConfig chat_proxy_get_default_config(void) {
    ChatProxyConfig config;
    config.connect_timeout_seconds = DEFAULT_CONNECT_TIMEOUT_SECONDS;
    config.request_timeout_seconds = DEFAULT_REQUEST_TIMEOUT_SECONDS;
    config.max_retries = DEFAULT_MAX_RETRIES;
    config.verify_ssl = DEFAULT_VERIFY_SSL;
    return config;
}

// Convert result code to string
const char* chat_proxy_result_code_to_string(ChatProxyResultCode code) {
    switch (code) {
        case CHAT_PROXY_OK: return "Success";
        case CHAT_PROXY_ERROR_INIT: return "CURL initialization failed";
        case CHAT_PROXY_ERROR_CONNECT: return "Connection failed";
        case CHAT_PROXY_ERROR_TIMEOUT: return "Request timeout";
        case CHAT_PROXY_ERROR_HTTP_4XX: return "HTTP client error (4xx)";
        case CHAT_PROXY_ERROR_HTTP_5XX: return "HTTP server error (5xx)";
        case CHAT_PROXY_ERROR_MEMORY: return "Memory allocation failed";
        case CHAT_PROXY_ERROR_NETWORK: return "Network error";
        case CHAT_PROXY_ERROR_UNKNOWN: return "Unknown error";
        default: return "Unknown";
    }
}

// Check if result is success
bool chat_proxy_result_is_success(const ChatProxyResult* result) {
    return result && result->code == CHAT_PROXY_OK && result->http_status >= 200 && result->http_status < 300;
}

// Check if request should be retried
bool chat_proxy_result_should_retry(const ChatProxyResult* result) {
    if (!result) return false;

    // Retry on specific result codes
    if (result->code == CHAT_PROXY_ERROR_TIMEOUT ||
        result->code == CHAT_PROXY_ERROR_NETWORK ||
        result->code == CHAT_PROXY_ERROR_CONNECT) {
        return true;
    }

    // Retry on specific HTTP status codes
    if (result->http_status == 429 ||  // Too Many Requests
        result->http_status == 502 ||  // Bad Gateway
        result->http_status == 503 ||  // Service Unavailable
        result->http_status == 504) {  // Gateway Timeout
        return true;
    }

    return false;
}

// Main proxy function - send request to AI API
ChatProxyResult* chat_proxy_send_request(const ChatEngineConfig* engine,
                                         const char* request_json,
                                         const ChatProxyConfig* config) {
    ChatProxyResult* result = chat_proxy_result_create();
    if (!result) {
        return NULL;
    }

    if (!engine || !request_json) {
        result->code = CHAT_PROXY_ERROR_INIT;
        result->error_message = strdup("Invalid parameters");
        return result;
    }

    // Initialize response buffer
    ChatResponseBuffer response_buffer;
    response_buffer.data = (char*)malloc(INITIAL_RESPONSE_BUFFER_SIZE);
    if (!response_buffer.data) {
        result->code = CHAT_PROXY_ERROR_MEMORY;
        result->error_message = strdup("Failed to allocate response buffer");
        return result;
    }
    response_buffer.data[0] = '\0';
    response_buffer.size = 0;
    response_buffer.capacity = INITIAL_RESPONSE_BUFFER_SIZE;

    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        result->code = CHAT_PROXY_ERROR_INIT;
        result->error_message = strdup("Failed to initialize CURL");
        free(response_buffer.data);
        return result;
    }

    // Build headers based on provider
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (engine->provider == CEC_PROVIDER_ANTHROPIC) {
        // Anthropic uses x-api-key header
        char auth_header[CEC_MAX_KEY_LEN + 32];
        snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", engine->api_key);
        headers = curl_slist_append(headers, auth_header);
        // Anthropic requires version header
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
        // OpenAI-compatible providers use Bearer token
        char auth_header[CEC_MAX_KEY_LEN + 32];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", engine->api_key);
        headers = curl_slist_append(headers, auth_header);
    }

    // Configure CURL options
    curl_easy_setopt(curl, CURLOPT_URL, engine->api_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, chat_proxy_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response_buffer);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)config->connect_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)config->request_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config->verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config->verify_ssl ? 2L : 0L);

    // Enable following redirects (some APIs may redirect)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);

    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hydrogen-Chat-Proxy/1.0");

    // Perform request
    CURLcode curl_result = curl_easy_perform(curl);

    // Get timing info
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &result->total_time_ms);
    result->total_time_ms *= 1000.0;  // Convert to milliseconds

    // Get HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    result->http_status = (int)http_code;

    // Process result
    if (curl_result != CURLE_OK) {
        // CURL error
        if (curl_result == CURLE_COULDNT_CONNECT ||
            curl_result == CURLE_COULDNT_RESOLVE_HOST) {
            result->code = CHAT_PROXY_ERROR_CONNECT;
        } else if (curl_result == CURLE_OPERATION_TIMEDOUT) {
            result->code = CHAT_PROXY_ERROR_TIMEOUT;
        } else if (curl_result == CURLE_OUT_OF_MEMORY) {
            result->code = CHAT_PROXY_ERROR_MEMORY;
        } else if (curl_result == CURLE_WRITE_ERROR) {
            result->code = CHAT_PROXY_ERROR_MEMORY;
        } else {
            result->code = CHAT_PROXY_ERROR_NETWORK;
        }
        result->error_message = strdup(curl_easy_strerror(curl_result));
        log_this(SR_CHAT, "CURL error: %s", LOG_LEVEL_ERROR, 1, curl_easy_strerror(curl_result));
    } else {
        // HTTP response received
        result->response_body = response_buffer.data;
        result->response_size = response_buffer.size;

        if (http_code >= 200 && http_code < 300) {
            result->code = CHAT_PROXY_OK;
            log_this(SR_CHAT, "Request successful: HTTP %ld, %.2fms", LOG_LEVEL_DEBUG, 2, http_code, result->total_time_ms);
        } else if (http_code >= 400 && http_code < 500) {
            result->code = CHAT_PROXY_ERROR_HTTP_4XX;
            result->error_message = strdup("HTTP client error");
            log_this(SR_CHAT, "HTTP client error: %ld", LOG_LEVEL_ERROR, 1, http_code);
        } else if (http_code >= 500) {
            result->code = CHAT_PROXY_ERROR_HTTP_5XX;
            result->error_message = strdup("HTTP server error");
            log_this(SR_CHAT, "HTTP server error: %ld", LOG_LEVEL_ERROR, 1, http_code);
        } else {
            result->code = CHAT_PROXY_ERROR_UNKNOWN;
            result->error_message = strdup("Unexpected HTTP response");
            log_this(SR_CHAT, "Unexpected HTTP response: %ld", LOG_LEVEL_ERROR, 1, http_code);
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // If there was an error, free the response buffer
    if (result->code != CHAT_PROXY_OK && response_buffer.data != result->response_body) {
        free(response_buffer.data);
    }

    return result;
}

// Send with retry logic
ChatProxyResult* chat_proxy_send_with_retry(const ChatEngineConfig* engine,
                                             const char* request_json,
                                             const ChatProxyConfig* config) {
    ChatProxyResult* result = NULL;
    int attempts = 0;
    int max_attempts = config->max_retries + 1;  // Initial attempt + retries

    while (attempts < max_attempts) {
        attempts++;

        // Send request
        result = chat_proxy_send_request(engine, request_json, config);

        // Check if successful or shouldn't retry
        if (chat_proxy_result_is_success(result) || !chat_proxy_result_should_retry(result)) {
            break;
        }

        // Don't retry on last attempt
        if (attempts >= max_attempts) {
            log_this(SR_CHAT, "All retry attempts exhausted", LOG_LEVEL_ERROR, 0);
            break;
        }

        // Calculate backoff delay with jitter: base * 2^(attempt-1) + random(0-1000ms)
        int base_delay = 1000;  // 1 second base
        int delay_ms = base_delay * (1 << (attempts - 1));
        int jitter = rand() % 1000;  // 0-1000ms jitter
        delay_ms += jitter;

        log_this(SR_CHAT, "Retrying after %dms (attempt %d/%d)", LOG_LEVEL_DEBUG, 3, delay_ms, attempts, max_attempts);

        // Sleep before retry (convert to microseconds for usleep)
        usleep((useconds_t)(delay_ms * 1000));

        // Clean up failed result before retry
        chat_proxy_result_destroy(result);
        result = NULL;
    }

    return result ? result : chat_proxy_result_create();
}

// Create multi-result structure
ChatMultiResult* chat_multi_result_create(size_t count) {
    ChatMultiResult* multi = (ChatMultiResult*)calloc(1, sizeof(ChatMultiResult));
    if (!multi) {
        return NULL;
    }

    multi->results = (ChatProxyResult**)calloc(count, sizeof(ChatProxyResult*));
    if (!multi->results) {
        free(multi);
        return NULL;
    }

    multi->count = count;
    multi->total_time_ms = 0.0;
    multi->success_count = 0;
    multi->failure_count = 0;

    return multi;
}

// Destroy multi-result structure
void chat_multi_result_destroy(ChatMultiResult* multi_result) {
    if (!multi_result) return;

    if (multi_result->results) {
        for (size_t i = 0; i < multi_result->count; i++) {
            chat_proxy_result_destroy(multi_result->results[i]);
        }
        free(multi_result->results);
    }

    free(multi_result);
}

// Structure to hold per-request context for curl_multi
typedef struct MultiRequestContext {
    ChatResponseBuffer buffer;
    CURL* curl;
    struct curl_slist* headers;
    ChatProxyResult* result;
    const ChatEngineConfig* engine;
    char engine_name[CEC_MAX_NAME_LEN];
} MultiRequestContext;

// Send multiple requests in parallel using curl_multi
ChatMultiResult* chat_proxy_send_multi(const ChatMultiRequest* requests,
                                        size_t request_count,
                                        const ChatProxyConfig* config) {
    if (!requests || request_count == 0) {
        return NULL;
    }

    ChatMultiResult* multi_result = chat_multi_result_create(request_count);
    if (!multi_result) {
        return NULL;
    }

    // Allocate context array for each request
    MultiRequestContext* contexts = (MultiRequestContext*)calloc(request_count, sizeof(MultiRequestContext));
    if (!contexts) {
        chat_multi_result_destroy(multi_result);
        return NULL;
    }

    // Initialize curl_multi handle
    CURLM* multi_handle = curl_multi_init();
    if (!multi_handle) {
        log_this(SR_CHAT, "Failed to initialize curl_multi handle", LOG_LEVEL_ERROR, 0);
        free(contexts);
        chat_multi_result_destroy(multi_result);
        return NULL;
    }

    // Track start time
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Setup all requests
    for (size_t i = 0; i < request_count; i++) {
        const ChatMultiRequest* req = &requests[i];
        MultiRequestContext* ctx = &contexts[i];

        // Store engine reference and name
        ctx->engine = req->engine;
        if (req->engine) {
            snprintf(ctx->engine_name, CEC_MAX_NAME_LEN, "%s", req->engine->name);
        } else {
            strcpy(ctx->engine_name, "unknown");
        }

        // Initialize response buffer
        ctx->buffer.data = (char*)malloc(INITIAL_RESPONSE_BUFFER_SIZE);
        if (!ctx->buffer.data) {
            ctx->result = chat_proxy_result_create();
            ctx->result->code = CHAT_PROXY_ERROR_MEMORY;
            ctx->result->error_message = strdup("Failed to allocate response buffer");
            multi_result->failure_count++;
            continue;
        }
        ctx->buffer.data[0] = '\0';
        ctx->buffer.size = 0;
        ctx->buffer.capacity = INITIAL_RESPONSE_BUFFER_SIZE;

        // Validate engine
        if (!req->engine || !req->request_json) {
            ctx->result = chat_proxy_result_create();
            ctx->result->code = CHAT_PROXY_ERROR_INIT;
            ctx->result->error_message = strdup("Invalid parameters");
            multi_result->failure_count++;
            continue;
        }

        // Create CURL easy handle
        ctx->curl = curl_easy_init();
        if (!ctx->curl) {
            ctx->result = chat_proxy_result_create();
            ctx->result->code = CHAT_PROXY_ERROR_INIT;
            ctx->result->error_message = strdup("Failed to initialize CURL");
            multi_result->failure_count++;
            continue;
        }

        // Build headers based on provider
        ctx->headers = NULL;
        ctx->headers = curl_slist_append(ctx->headers, "Content-Type: application/json");

        if (req->engine->provider == CEC_PROVIDER_ANTHROPIC) {
            // Anthropic uses x-api-key header
            char auth_header[CEC_MAX_KEY_LEN + 32];
            snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", req->engine->api_key);
            ctx->headers = curl_slist_append(ctx->headers, auth_header);
            // Anthropic requires version header
            ctx->headers = curl_slist_append(ctx->headers, "anthropic-version: 2023-06-01");
        } else {
            // OpenAI-compatible providers use Bearer token
            char auth_header[CEC_MAX_KEY_LEN + 32];
            snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", req->engine->api_key);
            ctx->headers = curl_slist_append(ctx->headers, auth_header);
        }

        // Configure CURL options
        curl_easy_setopt(ctx->curl, CURLOPT_URL, req->engine->api_url);
        curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, req->request_json);
        curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, ctx->headers);
        curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, chat_proxy_write_callback);
        curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, (void*)&ctx->buffer);
        curl_easy_setopt(ctx->curl, CURLOPT_CONNECTTIMEOUT, (long)config->connect_timeout_seconds);
        curl_easy_setopt(ctx->curl, CURLOPT_TIMEOUT, (long)config->request_timeout_seconds);
        curl_easy_setopt(ctx->curl, CURLOPT_SSL_VERIFYPEER, config->verify_ssl ? 1L : 0L);
        curl_easy_setopt(ctx->curl, CURLOPT_SSL_VERIFYHOST, config->verify_ssl ? 2L : 0L);
        curl_easy_setopt(ctx->curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(ctx->curl, CURLOPT_MAXREDIRS, 3L);
        curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, "Hydrogen-Chat-Proxy/1.0");

        // Add to multi handle
        curl_multi_add_handle(multi_handle, ctx->curl);
    }

    // Execute all requests in parallel
    int still_running = 0;
    curl_multi_perform(multi_handle, &still_running);

    // Wait for all requests to complete
    while (still_running) {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms

        fd_set fdread, fdwrite, fdexcep;
        int maxfd = -1;
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        if (maxfd == -1) {
            usleep(100000);  // 100ms
        } else {
            select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        curl_multi_perform(multi_handle, &still_running);
    }

    // Process results
    for (size_t i = 0; i < request_count; i++) {
        MultiRequestContext* ctx = &contexts[i];

        // If result was already set (error during setup), use it
        if (ctx->result) {
            multi_result->results[i] = ctx->result;
            continue;
        }

        // Create result from CURL response
        ChatProxyResult* result = chat_proxy_result_create();
        if (!result) {
            result = chat_proxy_result_create();
            result->code = CHAT_PROXY_ERROR_MEMORY;
            result->error_message = strdup("Failed to create result");
            multi_result->results[i] = result;
            multi_result->failure_count++;
            continue;
        }

        // Get CURL result
        CURLcode curl_result;
        curl_easy_getinfo(ctx->curl, CURLINFO_PRIVATE, &curl_result);
        curl_result = CURLE_OK;  // Default - we'll check from perform result

        // Get timing and HTTP status
        curl_easy_getinfo(ctx->curl, CURLINFO_TOTAL_TIME, &result->total_time_ms);
        result->total_time_ms *= 1000.0;

        long http_code = 0;
        curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &http_code);
        result->http_status = (int)http_code;

        // Read any error message from CURL
        const char* curl_error = curl_easy_strerror(curl_result);

        // Process based on HTTP status
        if (http_code >= 200 && http_code < 300) {
            result->code = CHAT_PROXY_OK;
            result->response_body = ctx->buffer.data;
            result->response_size = ctx->buffer.size;
            multi_result->success_count++;
            log_this(SR_CHAT, "Multi-request successful for %s: HTTP %ld, %.2fms",
                     LOG_LEVEL_DEBUG, 3, ctx->engine_name, http_code, result->total_time_ms);
        } else if (http_code >= 400 && http_code < 500) {
            result->code = CHAT_PROXY_ERROR_HTTP_4XX;
            result->error_message = strdup("HTTP client error");
            result->response_body = ctx->buffer.data;
            result->response_size = ctx->buffer.size;
            multi_result->failure_count++;
            log_this(SR_CHAT, "Multi-request client error for %s: HTTP %ld",
                     LOG_LEVEL_ERROR, 2, ctx->engine_name, http_code);
        } else if (http_code >= 500) {
            result->code = CHAT_PROXY_ERROR_HTTP_5XX;
            result->error_message = strdup("HTTP server error");
            result->response_body = ctx->buffer.data;
            result->response_size = ctx->buffer.size;
            multi_result->failure_count++;
            log_this(SR_CHAT, "Multi-request server error for %s: HTTP %ld",
                     LOG_LEVEL_ERROR, 2, ctx->engine_name, http_code);
        } else {
            result->code = CHAT_PROXY_ERROR_NETWORK;
            result->error_message = strdup(curl_error);
            free(ctx->buffer.data);
            multi_result->failure_count++;
            log_this(SR_CHAT, "Multi-request failed for %s: %s",
                     LOG_LEVEL_ERROR, 2, ctx->engine_name, curl_error);
        }

        multi_result->results[i] = result;
    }

    // Cleanup
    for (size_t i = 0; i < request_count; i++) {
        MultiRequestContext* ctx = &contexts[i];
        if (ctx->curl) {
            curl_multi_remove_handle(multi_handle, ctx->curl);
            curl_easy_cleanup(ctx->curl);
        }
        if (ctx->headers) {
            curl_slist_free_all(ctx->headers);
        }
        // Note: ctx->buffer.data is either freed or transferred to result
    }

    curl_multi_cleanup(multi_handle);
    free(contexts);

    // Calculate total time
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    multi_result->total_time_ms = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                   (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    log_this(SR_CHAT, "Multi-request completed: %zu success, %zu failed, %.2fms total",
             LOG_LEVEL_STATE, 3, multi_result->success_count, multi_result->failure_count,
             multi_result->total_time_ms);

    return multi_result;
}
