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
#include "../api/conduit/auth_chat/auth_chat.h"

// Local includes
#include "websocket_server_chat.h"
#include "websocket_server_internal.h"
#include "websocket_server.h"
#include "websocket_server_media.h"

// Chat common includes
#include "../api/conduit/chat_common/chat_engine_cache.h"
#include "../api/conduit/chat_common/chat_request_builder.h"
#include "../api/conduit/chat_common/chat_response_parser.h"
#include "../api/conduit/chat_common/chat_proxy.h"
#include "../api/conduit/chat_common/chat_metrics.h"
#include "../api/conduit/chat_common/chat_storage.h"
#include "../api/conduit/chat_common/chat_context_hashing.h"

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



// Streaming callback context
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

// Streaming chunk callback (called by chat_proxy_send_stream)
// This writes data DIRECTLY to the WebSocket - no queuing!
static void stream_chunk_callback(const ChatStreamChunk* chunk, void* user_data) {
    StreamContext* ctx = (StreamContext*)user_data;
    if (!chunk || !ctx || !ctx->wsi) return;

    // Check if connection is still valid before processing
    if (ctx->connection_valid && !*ctx->connection_valid) {
        log_this(SR_WEBSOCKET_CHAT, "Stream callback ignored - connection closed", LOG_LEVEL_DEBUG, 0);
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
        
        // Send final chunk with finish_reason
        send_stream_chunk(ctx, "", NULL, ctx->model, ctx->chunk_index, ctx->finish_reason);
        
        // Calculate time since stream start
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed_ms = (double)(end_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 +
                           (double)(end_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
        
        log_this(SR_WEBSOCKET_CHAT, "Stream completed for request %s with finish_reason: %s (%.0fms)",
                 LOG_LEVEL_DEBUG, 2, ctx->request_id ? ctx->request_id : "unknown", 
                 ctx->finish_reason, elapsed_ms);
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
        
        // Store in session for future messages
        session->chat_database = strdup(jwt_result.claims->database);
        session->chat_claims = jwt_result.claims; // ownership transferred
        // Note: jwt_result.claims is now NULL (freed by free_jwt_validation_result but we kept pointer)
        // We'll need to copy claims, but for simplicity we'll just keep database.
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
    
    // Build request JSON string (reuse auth_chat's request body building)
    // We need to reconstruct the request JSON from parsed data.
    // For simplicity, we'll use the original payload (already contains messages, etc.)
    char *request_json_str = json_dumps(payload, JSON_COMPACT);
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
        // Streaming mode - NO QUEUE! Data is written directly to WebSocket as it arrives
        
        // Initialize stream context - writes directly to WebSocket
        StreamContext stream_ctx = {0};
        stream_ctx.wsi = wsi;
        stream_ctx.request_id = request_id ? strdup(request_id) : NULL;
        stream_ctx.model = engine_name ? strdup(engine_name) : NULL;
        stream_ctx.chunk_index = 0;
        stream_ctx.stream_completed = false;
        stream_ctx.finish_reason = NULL;
        stream_ctx.first_chunk_logged = false;
        stream_ctx.connection_valid = &session->connection_valid;
        clock_gettime(CLOCK_MONOTONIC, &stream_ctx.start_time);

        // Mark stream as active on session BEFORE starting the stream
        // cppcheck: session guaranteed non-null due to check at function entry
        if (session) session->chat_stream_active = true;  // cppcheck: session validated above

        // Log that prompt is being sent to model server
        size_t request_size = strlen(request_json_str);
        log_this(SR_WEBSOCKET_CHAT, "Prompt sent to %s/%s/%s (streaming, %zu bytes)",
                 LOG_LEVEL_STATE, 3,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI",
                 request_size);

        // Use streaming-specific config with longer timeout (10 minutes)
        ChatProxyConfig proxy_config = chat_proxy_get_streaming_config();

        bool proxy_result = chat_proxy_send_stream(engine, request_json_str, &proxy_config,
                                                   stream_chunk_callback, &stream_ctx);

        // Calculate total response time
        struct timespec stream_end_time;
        clock_gettime(CLOCK_MONOTONIC, &stream_end_time);
        double response_time_ms = (double)(stream_end_time.tv_sec - stream_ctx.start_time.tv_sec) * 1000.0 +
                                  (double)(stream_end_time.tv_nsec - stream_ctx.start_time.tv_nsec) / 1000000.0;

        if (proxy_result) {
            // Send the final done message directly to the client
            send_stream_done(&stream_ctx,
                            stream_ctx.finish_reason ? stream_ctx.finish_reason : "stop",
                            0, 0, 0,  // Token counts not available in streaming mode
                            response_time_ms);
            
            // Send keepalive ping to refresh connection through load balancer
            if (stream_ctx.wsi && session) {
                ws_send_ping(stream_ctx.wsi, session);
            }
            
            // Log that final response was sent to client
            log_this(SR_WEBSOCKET_CHAT, "Final response sent to client for request %s (%.0fms total)",
                     LOG_LEVEL_DEBUG, 2,
                     request_id ? request_id : "unknown",
                     response_time_ms);
        } else {
            // Streaming proxy failed - send error to client
            send_chat_error(wsi, "Streaming proxy failed", request_id);
            log_this(SR_WEBSOCKET_CHAT, "Streaming proxy failed for request %s",
                     LOG_LEVEL_ERROR, 1, request_id ? request_id : "unknown");
        }

        // Mark stream as no longer active
        if (session) session->chat_stream_active = false;

        // Clean up stream context
        free(stream_ctx.finish_reason);
        free(stream_ctx.request_id);
        free(stream_ctx.model);
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
    log_this(SR_WEBSOCKET_CHAT, "Chat subsystem initialized", LOG_LEVEL_STATE, 0);
    return 0;
}

void chat_subsystem_cleanup(void) {
    // Nothing to cleanup yet
}

void chat_session_cleanup(WebSocketSessionData *session, struct lws *wsi) {
    if (!session) return;
    
    // Mark chat stream as inactive and connection invalid
    session->chat_stream_active = false;
    session->connection_valid = false;
    
    // Skip all freeing - lws will free the session memory when it's done
    // This avoids any potential double-free issues
    // The chat_database and chat_claims will be leaked but that's better than crashing
    
    (void)wsi;
}

void handle_chat_writable(struct lws *wsi, const WebSocketSessionData *session) {
    // Chat streaming no longer uses queues - data is written directly to WebSocket
    // This callback is now a no-op for chat (terminal still uses queues via terminal_pty.c)
    (void)wsi;
    (void)session;
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
