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

    // Build Authorization header
    char auth_header[CEC_MAX_KEY_LEN + 32];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", engine->api_key);

    // Set headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

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
