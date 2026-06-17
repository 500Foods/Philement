/*
 * Chat Proxy Multi Callbacks - CURL Callbacks for Multi-Stream
 *
 * Write callback for parsing SSE streaming responses and debug callback
 * for connection diagnostics. These run in the CURL worker thread.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "proxy_multi.h"
#include "resp_parser.h"

// System includes
#include <string.h>
#include <time.h>

// CURL write callback for streaming
size_t multi_stream_write_callback(const void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    CurlStreamContext* curl_ctx = (CurlStreamContext*)userp;
    
    if (!curl_ctx || !curl_ctx->stream_ctx) {
        return 0;
    }
    
    MultiStreamContext* stream_ctx = curl_ctx->stream_ctx;
    
    // Track bytes received
    curl_ctx->bytes_received += realsize;
    
    // TRACE: Log raw data in batches (every 50 chunks) to reduce log spam
    // Individual chunks are too verbose, but batching gives visibility into data flow
    if (realsize > 0 && curl_ctx->chunks_processed > 0 && (curl_ctx->chunks_processed % 50) == 0) {
        log_this(SR_CHAT, "Multi-stream progress: %zu chunks, %zu bytes total", 
                 LOG_LEVEL_TRACE, 2, curl_ctx->chunks_processed, curl_ctx->bytes_received);
    }
    
    // Log first data arrival (TTFB diagnostic)
    if (!curl_ctx->first_data_logged && realsize > 0) {
        curl_ctx->first_data_logged = true;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed_ms = (double)(now.tv_sec - stream_ctx->start_time.tv_sec) * 1000.0 +
                           (double)(now.tv_nsec - stream_ctx->start_time.tv_nsec) / 1000000.0;
        log_this(SR_CHAT, "Multi-stream TTFB: %.0fms (%zu bytes)", LOG_LEVEL_STATE, 2, elapsed_ms, realsize);
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
            // Log raw SSE data periodically for debugging (first 3 chunks and every 50th)
            if (curl_ctx->chunks_processed < 3 || curl_ctx->chunks_processed % 50 == 0) {
                log_this(SR_CHAT, "Raw SSE line #%zu (%zu bytes): %.200s",
                         LOG_LEVEL_DEBUG, 3, curl_ctx->chunks_processed, 
                         curl_ctx->line_buffer_len, curl_ctx->line_buffer);
            }

            // After [DONE], capture any subsequent lines as post-done data
            // (providers may send retrieval/citation metadata after the SSE stream)
            if (curl_ctx->seen_done) {
                // Accumulate non-empty lines into post_done_buffer
                if (curl_ctx->line_buffer_len > 0) {
                    // Initialize post_done_buffer lazily
                    if (!curl_ctx->post_done_buffer) {
                        curl_ctx->post_done_capacity = 4096;
                        curl_ctx->post_done_buffer = (char*)malloc(curl_ctx->post_done_capacity);
                        if (curl_ctx->post_done_buffer) {
                            curl_ctx->post_done_buffer[0] = '\0';
                            curl_ctx->post_done_len = 0;
                        }
                    }
                    if (curl_ctx->post_done_buffer) {
                        size_t needed_cap = curl_ctx->post_done_len + curl_ctx->line_buffer_len + 2;
                        if (needed_cap > curl_ctx->post_done_capacity) {
                            size_t new_cap = curl_ctx->post_done_capacity * 2;
                            while (new_cap < needed_cap) new_cap *= 2;
                            char* new_buf = realloc(curl_ctx->post_done_buffer, new_cap);
                            if (new_buf) {
                                curl_ctx->post_done_buffer = new_buf;
                                curl_ctx->post_done_capacity = new_cap;
                            }
                        }
                        memcpy(curl_ctx->post_done_buffer + curl_ctx->post_done_len,
                               curl_ctx->line_buffer, curl_ctx->line_buffer_len);
                        curl_ctx->post_done_len += curl_ctx->line_buffer_len;
                        curl_ctx->post_done_buffer[curl_ctx->post_done_len++] = '\n';
                        curl_ctx->post_done_buffer[curl_ctx->post_done_len] = '\0';
                    }
                    log_this(SR_CHAT, "Post-[DONE] data captured (%zu bytes): %.200s",
                             LOG_LEVEL_DEBUG, 2, curl_ctx->line_buffer_len, curl_ctx->line_buffer);
                }
            } else {
                // Normal SSE processing: parse and queue the line as a stream chunk
                ChatStreamChunk* chunk = chat_stream_chunk_parse(curl_ctx->line_buffer);
                if (chunk) {
                    // Mark when we see [DONE] so subsequent lines go to post_done_buffer
                    if (chunk->is_done) {
                        curl_ctx->seen_done = true;
                    }

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
                    // Include any provider-specific extra fields (e.g., retrieval data)
                    if (chunk->extra_fields) {
                        const char* key;
                        json_t* value;
                        json_object_foreach(chunk->extra_fields, key, value) {
                            json_object_set(chunk_json, key, value);
                        }
                    }
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
// cppcheck-suppress constParameterPointer - CURL callback signature is fixed (char* data required by libcurl)
int multi_stream_debug_callback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr) {
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
        
        // Only log key connection events, skip TLS handshake details
        if (strstr(debug_buf, "Connected to") || 
            strstr(debug_buf, "Trying ") ||
            strstr(debug_buf, "SSL certificate verify")) {
            log_this(SR_CHAT, "Multi CURL: %s", LOG_LEVEL_DEBUG, 1, debug_buf);
        }
        // TLS handshake details are logged at TRACE if needed
        else if (strstr(debug_buf, "TLS") || strstr(debug_buf, "SSL")) {
            log_this(SR_CHAT, "Multi CURL: %s", LOG_LEVEL_TRACE, 1, debug_buf);
        }
    } else if (type == CURLINFO_HEADER_OUT && size > 0) {
        log_this(SR_CHAT, "Multi CURL: Sent %zu bytes headers", LOG_LEVEL_TRACE, 1, size);
    } else if (type == CURLINFO_HEADER_IN && size > 0) {
        // Parse HTTP status from first line
        const char* p = data;
        const char* nl = memchr(p, '\n', size);
        size_t line_len = nl ? (size_t)(nl - p) : size;
        if (line_len > 0 && p[line_len-1] == '\r') line_len--;
        if (line_len > 0) {
            // Use separate specifiers to avoid log_this validation warning with %.*s
            char header_buf[256];
            size_t copy_len = line_len < sizeof(header_buf) - 1 ? line_len : sizeof(header_buf) - 1;
            memcpy(header_buf, p, copy_len);
            header_buf[copy_len] = '\0';
            log_this(SR_CHAT, "Multi CURL: Received %s", LOG_LEVEL_DEBUG, 1, header_buf);
        }
    }
    
    return 0;
}
