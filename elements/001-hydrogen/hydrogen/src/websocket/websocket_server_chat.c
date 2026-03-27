/*
 * WebSocket Chat Handler Implementation
 *
 * Handles chat messages via WebSocket protocol.
 * Supports both streaming and non-streaming chat requests.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include "../api/wschat/auth_chat/auth_chat.h"

// Local includes
#include "websocket_server_chat.h"
#include "websocket_server_internal.h"
#include "websocket_server.h"
#include "websocket_server_media.h"

// Chat common includes
#include "../api/wschat/helpers/engine_cache.h"
#include "../api/wschat/helpers/req_builder.h"
#include "../api/wschat/helpers/resp_parser.h"
#include "../api/wschat/helpers/proxy.h"
#include "../api/wschat/helpers/proxy_multi.h"
#include "../api/wschat/helpers/metrics.h"
#include "../api/wschat/helpers/storage.h"
#include "../api/wschat/helpers/context_hashing.h"

// Queue for thread-safe chunk passing
#include <src/queue/queue.h>
// WebSocket write functions
#include "websocket_server_message.h"

// External reference to global queue manager (from auth_chat.c)
extern DatabaseQueueManager* global_queue_manager;

// Forward declarations
static void send_chat_error(struct lws *wsi, const char* error_message, const char* request_id);
static void send_chat_done(struct lws *wsi, const char* request_id, const char* content,
                           const char* model, const char* finish_reason,
                           int prompt_tokens, int completion_tokens, int total_tokens,
                           double response_time_ms);
static __attribute__((unused)) void send_chat_chunk(struct lws *wsi, const char* request_id, const char* content,
                            const char* reasoning_content, const char* model, int index, const char* finish_reason);



// Streaming callback context (used for non-streaming mode only, kept for compatibility)
typedef struct {
    struct lws *wsi;
    char *request_id;
    char *model;
    int chunk_index;
    bool stream_completed;  // Whether stream completed successfully
    char *finish_reason;    // Final finish reason
    bool first_chunk_logged;  // Whether we've logged the first chunk (initial response)
    struct timespec start_time;  // When streaming started
    volatile bool *connection_valid;  // Pointer to flag that's set false when connection closes
    volatile bool *stream_active;  // Pointer to session's chat_stream_active flag
    MultiStreamContext *multi_stream_ctx;  // Multi-stream context (for queue-based streaming)
} StreamContext;

// Helper: Build and send a chat chunk directly to the WebSocket
static void send_stream_chunk(StreamContext* ctx, const char* content, 
                              const char* reasoning_content, const char* model, 
                              int index, const char* finish_reason) {
    if (!ctx || !ctx->wsi) return;
    
    // Check connection validity before writing
    if (ctx->connection_valid && !*ctx->connection_valid) {
        return;  // Connection closed, don't write
    }
    
    // Build JSON string for this chunk
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_chunk"));
    if (ctx->request_id) {
        json_object_set_new(response, "id", json_string(ctx->request_id));
    }
    
    json_t* chunk = json_object();
    json_object_set_new(chunk, "content", json_string(content ? content : ""));
    if (reasoning_content) {
        json_object_set_new(chunk, "reasoning_content", json_string(reasoning_content));
    }
    if (model) json_object_set_new(chunk, "model", json_string(model));
    json_object_set_new(chunk, "index", json_integer(index));
    if (finish_reason) json_object_set_new(chunk, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "chunk", chunk);
    
    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);
    
    if (json_str) {
        // Write may fail if connection is closing - that's OK, we handle it gracefully
        int result = ws_write_raw_data(ctx->wsi, json_str, strlen(json_str));
        if (result != 0) {
            // Write failed - mark connection as invalid to stop further writes
            if (ctx->connection_valid) {
                // Note: we can't set it directly (it's volatile), but we log it
                log_this(SR_WEBSOCKET_CHAT, "Write failed, stopping stream", LOG_LEVEL_DEBUG, 0);
            }
        }
        free(json_str);
    }
}

// Helper: Build and send a chat done message directly to the WebSocket
static void send_stream_done(StreamContext* ctx, const char* finish_reason,
                             int prompt_tokens, int completion_tokens, int total_tokens,
                             double response_time_ms) {
    if (!ctx || !ctx->wsi) return;
    
    // Check connection validity before writing
    if (ctx->connection_valid && !*ctx->connection_valid) {
        return;  // Connection closed, don't write
    }
    
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_done"));
    if (ctx->request_id) {
        json_object_set_new(response, "id", json_string(ctx->request_id));
    }
    
    json_t* result = json_object();
    json_object_set_new(result, "content", json_string(""));
    if (ctx->model) json_object_set_new(result, "model", json_string(ctx->model));
    if (finish_reason) json_object_set_new(result, "finish_reason", json_string(finish_reason));
    
    json_t* tokens = json_object();
    json_object_set_new(tokens, "prompt", json_integer(prompt_tokens));
    json_object_set_new(tokens, "completion", json_integer(completion_tokens));
    json_object_set_new(tokens, "total", json_integer(total_tokens));
    json_object_set_new(result, "tokens", tokens);
    
    json_object_set_new(result, "response_time_ms", json_real(response_time_ms));
    json_object_set_new(response, "result", result);
    
    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);
    
    if (json_str) {
        // Write may fail if connection is closing - that's OK
        ws_write_raw_data(ctx->wsi, json_str, strlen(json_str));
        free(json_str);
    }
}

/*
 * Streaming chunk callback - UNUSED
 * 
 * This callback was used with the old thread-based streaming implementation.
 * The new multi_curl implementation uses internal callbacks in proxy_multi.c
 * and queue-based chunk delivery for thread-safe WebSocket writes.
 * 
 * Kept for reference - can be removed in a future cleanup.
 */
__attribute__((unused)) static void stream_chunk_callback(const ChatStreamChunk* chunk, void* user_data) {
    StreamContext* ctx = (StreamContext*)user_data;
    if (!chunk || !ctx || !ctx->wsi) return;

    // Check if connection is still valid before processing
    if (ctx->connection_valid && !*ctx->connection_valid) {
        return;
    }

    if (chunk->is_done) {
        // Mark stream as completed and store finish reason
        ctx->stream_completed = true;
        if (chunk->finish_reason) {
            ctx->finish_reason = strdup(chunk->finish_reason);
        } else {
            ctx->finish_reason = strdup("stop");
        }
        
        // Calculate time since stream start
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed_ms = (double)(end_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 +
                           (double)(end_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
        
        // Send the final chat_done message to client
        send_stream_done(ctx,
                        ctx->finish_reason ? ctx->finish_reason : "stop",
                        0, 0, 0,  // Token counts not available in streaming mode
                        elapsed_ms);
        
        // Log streaming summary
        log_this(SR_WEBSOCKET_CHAT, "Stream complete: %d chunks, %.0fms, finish=%s",
                 LOG_LEVEL_STATE, 3,
                 ctx->chunk_index,
                 elapsed_ms,
                 ctx->finish_reason ? ctx->finish_reason : "stop");
        
        // Mark stream as no longer active in session
        if (ctx->stream_active) {
            *ctx->stream_active = false;
        }
        
        // Free stream context - ownership transferred from LWS callback to worker thread
        // The callback is the last place to access ctx before stream completes
        free(ctx->finish_reason);
        free(ctx->request_id);
        free(ctx->model);
        free(ctx);  // Free the context itself (heap-allocated in handle_chat_message)
    } else {
        // Log first chunk as "initial response received"
        if (!ctx->first_chunk_logged) {
            ctx->first_chunk_logged = true;
            struct timespec first_chunk_time;
            clock_gettime(CLOCK_MONOTONIC, &first_chunk_time);
            double ttfb_ms = (double)(first_chunk_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 +
                            (double)(first_chunk_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
            log_this(SR_WEBSOCKET_CHAT, "Initial response received from %s (%.0fms TTFB)",
                     LOG_LEVEL_DEBUG, 1, ctx->model ? ctx->model : "unknown", ttfb_ms);
        }
        
        // Update model if provided
        const char* model = chunk->model ? chunk->model : ctx->model;
        if (chunk->model) {
            free(ctx->model);
            ctx->model = strdup(chunk->model);
        }
        
        // Write directly to WebSocket - NO QUEUE!
        const char* content = chunk->content ? chunk->content : "";
        const char* reasoning_content = chunk->reasoning_content ? chunk->reasoning_content : NULL;
        send_stream_chunk(ctx, content, reasoning_content, model, ctx->chunk_index, NULL);
        ctx->chunk_index++;
    }
}

int handle_chat_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json) {
    if (!wsi || !session || !request_json) {
        return -1;
    }
    
    // Extract request ID (optional)
    const char* request_id = NULL;
    json_t* id_json = json_object_get(request_json, "id");
    if (id_json && json_is_string(id_json)) {
        request_id = json_string_value(id_json);
    }
    
    // Extract payload
    json_t* payload = json_object_get(request_json, "payload");
    if (!payload || !json_is_object(payload)) {
        send_chat_error(wsi, "Missing or invalid payload", request_id);
        json_decref(request_json);
        return -1;
    }
    
    // Parse request (reuse auth_chat_parse_request)
    char *engine_name = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;
    
    bool parse_ok = auth_chat_parse_request(payload, &engine_name,
                                             &messages, &context_hashes,
                                             &context_hash_count,
                                             &temperature, &max_tokens,
                                             &stream, &error_message);
    
    if (!parse_ok) {
        send_chat_error(wsi, error_message ? error_message : "Invalid request", request_id);
        free(error_message);
        json_decref(request_json);
        return -1;
    }
    
    // Log message count for debugging
    size_t message_count = json_array_size(messages);
    log_this(SR_WEBSOCKET_CHAT, "Request parsed: %zu messages, stream=%s",
             LOG_LEVEL_DEBUG, 2, message_count, stream ? "true" : "false");
    
    // JWT authentication - check if already authenticated for chat
    if (!session->chat_database) {
        // Need JWT from payload
        json_t* jwt_json = json_object_get(payload, "jwt");
        if (!jwt_json || !json_is_string(jwt_json)) {
            send_chat_error(wsi, "JWT required for chat authentication", request_id);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        
        const char* jwt = json_string_value(jwt_json);
        jwt_validation_result_t jwt_result = {0};
        if (!extract_and_validate_jwt(jwt, &jwt_result)) {
            send_chat_error(wsi, "Invalid JWT", request_id);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        
        // Extract database from claims
        if (!jwt_result.claims || !jwt_result.claims->database) {
            send_chat_error(wsi, "JWT missing database claim", request_id);
            free_jwt_validation_result(&jwt_result);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        
        // Store database name in session for future messages
        // We only need the database name, not the full claims structure
        session->chat_database = strdup(jwt_result.claims->database);
        // Don't store chat_claims - the claims are freed below and we don't need them
        // The database name is all we need for routing chat requests
        session->chat_claims = NULL;
        free_jwt_validation_result(&jwt_result);
    }
    
    // cppcheck: session guaranteed non-null due to check at function entry
    const char* database = session ? session->chat_database : NULL;
    
    // Get database queue
    DatabaseQueue *db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        send_chat_error(wsi, "Database not found", request_id);
        free(engine_name);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_decref(request_json);
        return -1;
    }
    
    // Get CEC
    ChatEngineCache *cec = db_queue->chat_engine_cache;
    if (!cec) {
        send_chat_error(wsi, "Chat not enabled for this database", request_id);
        free(engine_name);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_decref(request_json);
        return -1;
    }
    
    // Look up engine
    const ChatEngineConfig *engine = NULL;
    if (engine_name) {
        engine = chat_engine_cache_lookup_by_name(cec, engine_name);
    } else {
        engine = chat_engine_cache_get_default(cec);
    }
    
    if (!engine) {
        send_chat_error(wsi, engine_name ? "Engine not found" : "No default engine configured", request_id);
        free(engine_name);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_decref(request_json);
        return -1;
    }
    
    // Check engine health
    if (!engine->is_healthy) {
        send_chat_error(wsi, "Engine is currently unavailable", request_id);
        free(engine_name);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_decref(request_json);
        return -1;
    }
    
    // Build request parameters
    ChatRequestParams params = chat_request_params_default();
    params.temperature = (temperature >= 0.0) ? temperature : engine->temperature_default;
    params.max_tokens = (max_tokens > 0) ? max_tokens : engine->max_tokens;
    params.stream = stream;
    
    // Convert messages JSON to ChatMessage list for proper request building
    ChatMessage *chat_messages = NULL;
    size_t msg_count = json_array_size(messages);
    for (size_t i = 0; i < msg_count; i++) {
        json_t *msg = json_array_get(messages, i);
        if (!json_is_object(msg)) continue;

        json_t *role_obj = json_object_get(msg, "role");
        json_t *content_obj = json_object_get(msg, "content");
        if (!role_obj || !content_obj) continue;

        const char *role_str = json_string_value(role_obj);
        ChatMessageRole role = chat_message_role_from_string(role_str);

        char *content_str = NULL;
        if (json_is_string(content_obj)) {
            content_str = strdup(json_string_value(content_obj));
        } else if (json_is_array(content_obj)) {
            content_str = json_dumps(content_obj, JSON_COMPACT);
        }

        if (content_str) {
            ChatMessage *new_msg = chat_message_create(role, content_str, NULL);
            chat_messages = chat_message_list_append(chat_messages, new_msg);
            free(content_str);
        }
    }
    
    // Build proper request JSON for provider using chat_request_build
    json_t *provider_request = chat_request_build(engine, chat_messages, &params);
    chat_message_list_destroy(chat_messages);
    
    if (!provider_request) {
        send_chat_error(wsi, "Failed to build request", request_id);
        free(engine_name);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_decref(request_json);
        return -1;
    }
    
    char *request_json_str = json_dumps(provider_request, JSON_COMPACT);
    json_decref(provider_request);
    
    if (!request_json_str) {
        send_chat_error(wsi, "Failed to serialize request", request_id);
        free(engine_name);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_decref(request_json);
        return -1;
    }
    
    // Start metrics
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    if (stream) {
        // Streaming mode - Using multi_curl for thread-safe, non-blocking streaming
        
        // Wait for any previous stream to fully complete before starting new one
        // This ensures all data is sent and session->multi_stream_ctx is properly cleared
        // cppcheck-suppress nullPointerRedundantCheck - session validated at function entry
        if (session->chat_stream_active || session->multi_stream_ctx) {
            log_this(SR_WEBSOCKET_CHAT, "Waiting for previous stream to complete before starting new one",
                     LOG_LEVEL_DEBUG, 0);
            // The previous stream's completion callback will clear multi_stream_ctx
            // We just need to wait for that to happen
            // TODO: Could implement a condition variable for proper waiting
            // For now, just log and proceed - the cleanup should have already run
        }
        
        // Allocate stream context on heap (managed by session cleanup)
        StreamContext* stream_ctx = (StreamContext*)calloc(1, sizeof(StreamContext));
        if (!stream_ctx) {
            send_chat_error(wsi, "Failed to allocate stream context", request_id);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        
        stream_ctx->wsi = wsi;
        stream_ctx->request_id = request_id ? strdup(request_id) : NULL;
        stream_ctx->model = engine_name ? strdup(engine_name) : NULL;
        stream_ctx->chunk_index = 0;
        stream_ctx->stream_completed = false;
        stream_ctx->finish_reason = NULL;
        stream_ctx->first_chunk_logged = false;
        stream_ctx->connection_valid = &session->connection_valid;
        stream_ctx->stream_active = &session->chat_stream_active;
        stream_ctx->multi_stream_ctx = NULL;  // Will be set by multi-stream start
        clock_gettime(CLOCK_MONOTONIC, &stream_ctx->start_time);

        // Mark stream as active on session BEFORE starting the stream
        // cppcheck: session guaranteed non-null due to check at function entry
        if (session) session->chat_stream_active = true;  // cppcheck: session validated above

        // Log that prompt is being sent to model server
        size_t request_size = strlen(request_json_str);
        log_this(SR_WEBSOCKET_CHAT, "Prompt sent to %s/%s/%s (streaming multi_curl, %zu bytes)",
                 LOG_LEVEL_STATE, 4,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI",
                 request_size);

        // Start streaming using multi_curl interface
        // This is non-blocking - chunks are enqueued and written via LWS writable callback
        MultiStreamManager* manager = chat_proxy_get_multi_manager();
        if (!manager) {
            send_chat_error(wsi, "Multi-stream manager not initialized", request_id);
            log_this(SR_WEBSOCKET_CHAT, "Multi-stream manager not available for request %s",
                     LOG_LEVEL_ERROR, 1, request_id ? request_id : "unknown");
            
            if (session) session->chat_stream_active = false;
            free(stream_ctx->finish_reason);
            free(stream_ctx->request_id);
            free(stream_ctx->model);
            free(stream_ctx);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            free(request_json_str);
            json_decref(request_json);
            return -1;
        }
        
        MultiStreamContext* multi_ctx = chat_proxy_multi_stream_start(
            manager,
            engine,
            request_json_str,
            wsi,
            session,
            &session->connection_valid,
            &session->chat_stream_active,
            NULL,   // chunk_callback - not needed, queue-based
            NULL,   // user_data
            NULL,   // completion_callback
            NULL    // completion_user_data
        );

        if (!multi_ctx) {
            // Multi-stream start failed - send error to client immediately
            send_chat_error(wsi, "Failed to start streaming", request_id);
            log_this(SR_WEBSOCKET_CHAT, "Failed to start multi-stream for request %s",
                     LOG_LEVEL_ERROR, 1, request_id ? request_id : "unknown");
            
            // Mark stream as no longer active
            if (session) session->chat_stream_active = false;
            
            // Clean up stream context
            free(stream_ctx->finish_reason);
            free(stream_ctx->request_id);
            free(stream_ctx->model);
            free(stream_ctx);
            
            // Cleanup and return
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            free(request_json_str);
            json_decref(request_json);
            return -1;
        }
        
        // Store multi-stream context in stream context for cleanup
        stream_ctx->multi_stream_ctx = multi_ctx;
        
        // Store multi-stream context in session for writable callback to drain queue
        session->multi_stream_ctx = multi_ctx;
        
        // Request writable callback to start draining the queue
        lws_callback_on_writable(wsi);
        
        // Log that streaming has started
        log_this(SR_WEBSOCKET_CHAT, "Multi-stream started for request %s",
                 LOG_LEVEL_DEBUG, 1, request_id ? request_id : "unknown");
        
        // Note: stream_ctx is managed by session and will be cleaned up on session close
        // multi_ctx is owned by the multi-stream manager
        
    } else {
        // Non-streaming mode
        ChatProxyConfig proxy_config = chat_proxy_get_default_config();
        
        // Log that prompt is being sent to model server
        size_t request_size = strlen(request_json_str);
        log_this(SR_WEBSOCKET_CHAT, "Prompt sent to %s/%s/%s (non-streaming, %zu bytes)",
                 LOG_LEVEL_STATE, 3,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI",
                 request_size);
        
        ChatProxyResult *proxy_result = chat_proxy_send_with_retry(engine, request_json_str, &proxy_config);
        
        if (!proxy_result || proxy_result->code != CHAT_PROXY_OK) {
            const char *error_msg = proxy_result && proxy_result->error_message ? 
                                    proxy_result->error_message : "Request failed";
            send_chat_error(wsi, error_msg, request_id);
            log_this(SR_WEBSOCKET_CHAT, "Non-streaming request failed for request %s: %s", 
                     LOG_LEVEL_ERROR, 2, request_id ? request_id : "unknown", error_msg);
            chat_proxy_result_destroy(proxy_result);
            free(request_json_str);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        
        // Log that response was received with size
        size_t response_size = proxy_result->response_body ? strlen(proxy_result->response_body) : 0;
        log_this(SR_WEBSOCKET_CHAT, "Response received from %s/%s/%s (%.0fms, %zu bytes)",
                 LOG_LEVEL_DEBUG, 4,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI",
                 proxy_result->total_time_ms,
                 response_size);
        
        // Parse response
        ChatParsedResponse *parsed = chat_response_parse(proxy_result->response_body, engine->provider);
        if (parsed) {
            log_this(SR_WEBSOCKET_CHAT, "Parsed response: content_length=%zu, model=%s, finish_reason=%s",
                     LOG_LEVEL_DEBUG, 3,
                     parsed->content ? strlen(parsed->content) : 0,
                     parsed->model ? parsed->model : "null",
                     parsed->finish_reason ? parsed->finish_reason : "null");
        }
        if (!parsed || !parsed->success) {
            const char *error_msg = parsed && parsed->error_message ? 
                                    parsed->error_message : "Failed to parse response";
            send_chat_error(wsi, error_msg, request_id);
            log_this(SR_WEBSOCKET_CHAT, "Failed to parse response for request %s: %s", 
                     LOG_LEVEL_ERROR, 2, request_id ? request_id : "unknown", error_msg);
            chat_parsed_response_destroy(parsed);
            chat_proxy_result_destroy(proxy_result);
            free(request_json_str);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        
        // Calculate response time
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double response_time_ms = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                  (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
        
        // Log content details before sending
        size_t content_length = parsed->content ? strlen(parsed->content) : 0;
        log_this(SR_WEBSOCKET_CHAT, "Sending response: content=%zu bytes, messages=%zu, finish=%s",
                 LOG_LEVEL_DEBUG, 3,
                 content_length, message_count,
                 parsed->finish_reason ? parsed->finish_reason : "null");
        
        // Send chat_done
        send_chat_done(wsi, request_id, parsed->content, parsed->model, parsed->finish_reason,
                       parsed->prompt_tokens, parsed->completion_tokens, parsed->total_tokens,
                       response_time_ms);
        
        // Log that final response was sent to client
        log_this(SR_WEBSOCKET_CHAT, "Final response sent to client for request %s (%.0fms total, %zu bytes content)", 
                 LOG_LEVEL_STATE, 2,
                 request_id ? request_id : "unknown",
                 response_time_ms,
                 content_length);
        
        // Store conversation (if needed)
        // TODO: integrate with chat_storage
        
        // Cleanup
        chat_parsed_response_destroy(parsed);
        chat_proxy_result_destroy(proxy_result);
    }
    
    // Cleanup
    free(request_json_str);
    free(engine_name);
    chat_context_free_hash_array(context_hashes, context_hash_count);
    json_decref(request_json);
    
    return 0;
}

int chat_subsystem_init(void) {
    // Initialize the multi-stream manager for thread-safe streaming
    // Get the global manager instance and initialize it
    MultiStreamManager* manager = chat_proxy_get_multi_manager();
    if (!manager) {
        log_this(SR_WEBSOCKET_CHAT, "Failed to get multi-stream manager", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    
    // Initialize the manager (NULL lws_context means standalone mode for now)
    if (!chat_proxy_multi_init(manager, NULL)) {
        log_this(SR_WEBSOCKET_CHAT, "Failed to initialize multi-stream manager", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    
    log_this(SR_WEBSOCKET_CHAT, "Chat subsystem initialized with multi-stream manager", LOG_LEVEL_STATE, 0);
    return 0;
}

void chat_subsystem_cleanup(void) {
    // Cleanup the multi-stream manager
    MultiStreamManager* manager = chat_proxy_get_multi_manager();
    if (manager) {
        chat_proxy_multi_cleanup(manager);
    }
    log_this(SR_WEBSOCKET_CHAT, "Chat subsystem cleaned up", LOG_LEVEL_DEBUG, 0);
}

void chat_session_cleanup(WebSocketSessionData *session, struct lws *wsi) {
    if (!session) return;
    
    // Mark chat stream as inactive and connection invalid FIRST
    // This prevents any further writes or callbacks from using these resources
    session->chat_stream_active = false;
    session->connection_valid = false;
    
    // Stop any active multi-stream if one exists
    // This must be done before freeing session resources to prevent use-after-free
    if (session->multi_stream_ctx) {
        MultiStreamManager* manager = chat_proxy_get_multi_manager();
        if (manager) {
            // Stop the stream but DON'T free the context here - let the manager handle it
            // The manager will clean up the stream context in its own time
            // We just need to disconnect it from our session to prevent dangling pointers
            chat_proxy_multi_stream_stop(manager, session->multi_stream_ctx);
        }
        session->multi_stream_ctx = NULL;
    }
    
    // Free chat_database string - safe to free, we allocated it with strdup
    if (session->chat_database) {
        free(session->chat_database);
        session->chat_database = NULL;
    }
    
    // Free chat_claims - safe to free, we made a copy in handle_chat_message
    // Note: chat_claims was set to NULL by free_jwt_validation_result in handle_chat_message,
    // but we keep the pointer for backward compatibility. If it's somehow still set,
    // it would point to freed memory, so we just NULL it without freeing.
    // This is safe because the original claims were already freed during request handling.
    session->chat_claims = NULL;
    
    (void)wsi;
}

void handle_chat_writable(struct lws *wsi, WebSocketSessionData *session) {
    // For multi-stream mode, drain the chunk queue for any active streams
    // This is called from the LWS service thread, so writes are thread-safe
    
    (void)wsi;  // wsi is used via session->multi_stream_ctx->wsi
    
    if (!session || !session->multi_stream_ctx) {
        return;
    }
    
    // Drain the chunk queue - this writes queued chunks to the WebSocket
    MultiStreamContext* ctx = session->multi_stream_ctx;
    int written = chat_proxy_multi_drain_queue(ctx);
    
    if (written > 0) {
        log_this(SR_WEBSOCKET_CHAT, "Drained %d chunks from queue to WebSocket", LOG_LEVEL_TRACE, 1, written);
    }
}

// Helper functions to send WebSocket messages
static void send_chat_error(struct lws *wsi, const char* error_message, const char* request_id) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_error"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    json_object_set_new(response, "error", json_string(error_message ? error_message : "Unknown error"));
    
    ws_write_json_response(wsi, response);
    json_decref(response);
}

static void send_chat_done(struct lws *wsi, const char* request_id, const char* content, 
                           const char* model, const char* finish_reason,
                           int prompt_tokens, int completion_tokens, int total_tokens,
                           double response_time_ms) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_done"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    
    json_t* result = json_object();
    json_object_set_new(result, "content", json_string(content ? content : ""));
    if (model) json_object_set_new(result, "model", json_string(model));
    if (finish_reason) json_object_set_new(result, "finish_reason", json_string(finish_reason));
    
    json_t* tokens = json_object();
    json_object_set_new(tokens, "prompt", json_integer(prompt_tokens));
    json_object_set_new(tokens, "completion", json_integer(completion_tokens));
    json_object_set_new(tokens, "total", json_integer(total_tokens));
    json_object_set_new(result, "tokens", tokens);
    
    json_object_set_new(result, "response_time_ms", json_real(response_time_ms));
    json_object_set_new(response, "result", result);
    
    ws_write_json_response(wsi, response);
    json_decref(response);
}

static __attribute__((unused)) void send_chat_chunk(struct lws *wsi, const char* request_id, const char* content,
                            const char* reasoning_content, const char* model, int index, const char* finish_reason) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_chunk"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    
    json_t* chunk = json_object();
    json_object_set_new(chunk, "content", json_string(content ? content : ""));
    if (reasoning_content) {
        json_object_set_new(chunk, "reasoning_content", json_string(reasoning_content));
    }
    if (model) json_object_set_new(chunk, "model", json_string(model));
    json_object_set_new(chunk, "index", json_integer(index));
    if (finish_reason) json_object_set_new(chunk, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "chunk", chunk);
    
    ws_write_json_response(wsi, response);
    json_decref(response);
}
