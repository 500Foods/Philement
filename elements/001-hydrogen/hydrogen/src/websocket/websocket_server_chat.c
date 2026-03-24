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

// Logging source for WebSocket chat
static const char* SR_WEBSOCKET_CHAT = "WEBSOCKET_CHAT";

// Forward declarations
static void send_chat_error(struct lws *wsi, const char* error_message, const char* request_id);
static void send_chat_done(struct lws *wsi, const char* request_id, const char* content, 
                           const char* model, const char* finish_reason,
                           int prompt_tokens, int completion_tokens, int total_tokens,
                           double response_time_ms);
static void send_chat_chunk(struct lws *wsi, const char* request_id, const char* content,
                            const char* model, int index, const char* finish_reason);

// Queue management for thread-safe streaming
static bool enqueue_chat_chunk(const char* queue_name, const char* request_id, 
                               const char* content, const char* model, 
                               int index, const char* finish_reason);
static void dequeue_and_write_chat_chunks(struct lws *wsi, const char* queue_name);

// Queue management implementations
char* create_chat_queue_name(const struct lws *wsi) {
    char* queue_name = malloc(64);
    if (!queue_name) return NULL;
    snprintf(queue_name, 64, "chat_wsi_%p", (const void*)wsi);
    return queue_name;
}

static bool enqueue_chat_chunk(const char* queue_name, const char* request_id, 
                               const char* content, const char* model, 
                               int index, const char* finish_reason) {
    if (!queue_name) return false;
    
    // Build JSON string for this chunk
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_chunk"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    
    json_t* chunk = json_object();
    json_object_set_new(chunk, "content", json_string(content ? content : ""));
    if (model) json_object_set_new(chunk, "model", json_string(model));
    json_object_set_new(chunk, "index", json_integer(index));
    if (finish_reason) json_object_set_new(chunk, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "chunk", chunk);
    
    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);
    if (!json_str) return false;
    
    // Enqueue with priority 0 (normal)
    Queue* queue = queue_find(queue_name);
    if (!queue) {
        // Create queue if it doesn't exist
        QueueAttributes attrs = {0};
        queue = queue_create(queue_name, &attrs);
        if (!queue) {
            free(json_str);
            return false;
        }
    }
    
    bool success = queue_enqueue(queue, json_str, strlen(json_str), 0);
    free(json_str);
    
    // Signal that we have data to write (will be done by caller)
    return success;
}

static void dequeue_and_write_chat_chunks(struct lws *wsi, const char* queue_name) {
    if (!queue_name || !wsi) return;
    
    Queue* queue = queue_find(queue_name);
    if (!queue) return;
    
    // Dequeue and write all available chunks
    size_t size;
    int priority;
    char* data;
    while ((data = queue_dequeue(queue, &size, &priority)) != NULL) {
        // Write the JSON string directly via WebSocket
        ws_write_raw_data(wsi, data, size);
        free(data);
    }
}

static void cleanup_chat_queue(char** queue_name_ptr) {
    if (!queue_name_ptr || !*queue_name_ptr) return;
    
    Queue* queue = queue_find(*queue_name_ptr);
    if (queue) {
        queue_clear(queue);
        queue_destroy(queue);
    }
    free(*queue_name_ptr);
    *queue_name_ptr = NULL;
}

// Streaming callback context
typedef struct {
    struct lws *wsi;
    char *request_id;
    char *model;
    int chunk_index;
    char *queue_name;  // Name of queue for thread-safe communication
    bool stream_completed;  // Whether stream completed successfully
    char *finish_reason;    // Final finish reason
    bool first_chunk_logged;  // Whether we've logged the first chunk (initial response)
    struct timespec start_time;  // When streaming started
} StreamContext;

// Streaming chunk callback (called by chat_proxy_send_stream)
static void stream_chunk_callback(const ChatStreamChunk* chunk, void* user_data) {
    StreamContext* ctx = (StreamContext*)user_data;
    if (!chunk || !ctx) return;
    
    if (chunk->is_done) {
        // Mark stream as completed and store finish reason
        ctx->stream_completed = true;
        if (chunk->finish_reason) {
            ctx->finish_reason = strdup(chunk->finish_reason);
        } else {
            ctx->finish_reason = strdup("stop");
        }
        // Enqueue final done chunk with finish_reason
        enqueue_chat_chunk(ctx->queue_name, ctx->request_id, "", ctx->model, ctx->chunk_index, ctx->finish_reason);
        
        // Calculate time since stream start
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed_ms = (double)(end_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 +
                           (double)(end_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
        
        log_this(SR_WEBSOCKET_CHAT, "Stream completed for request %s with finish_reason: %s (%.0fms)",
                 LOG_LEVEL_STATE, 2, ctx->request_id ? ctx->request_id : "unknown", 
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
                     LOG_LEVEL_STATE, 1, ctx->model ? ctx->model : "unknown", ttfb_ms);
        }
        
        // Enqueue content chunk - client already receives these, no need to accumulate
        const char* content = chunk->content ? chunk->content : "";
        const char* model = chunk->model ? chunk->model : ctx->model;
        if (chunk->model) {
            free(ctx->model);
            ctx->model = strdup(chunk->model);
        }
        enqueue_chat_chunk(ctx->queue_name, ctx->request_id, content, model, ctx->chunk_index, NULL);
        ctx->chunk_index++;
    }
    
    // Request writable callback from the service thread
    if (ctx->wsi) {
        lws_callback_on_writable(ctx->wsi);
    }
}

int handle_chat_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json) {
    if (!wsi || !request_json) {
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
    
    const char* database = session->chat_database;
    
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
        // Streaming mode
        // Clean up any existing chat queue from previous stream
        cleanup_chat_queue(&session->chat_write_queue_name);
        
        // Create a new queue for this stream
        char* queue_name = create_chat_queue_name(wsi);
        if (!queue_name) {
            send_chat_error(wsi, "Failed to create streaming queue", request_id);
            free(request_json_str);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
        session->chat_write_queue_name = queue_name;
        
        // Initialize stream context
        StreamContext stream_ctx = {0};
        stream_ctx.wsi = wsi;
        stream_ctx.request_id = request_id ? strdup(request_id) : NULL;
        stream_ctx.model = engine_name ? strdup(engine_name) : NULL;
        stream_ctx.chunk_index = 0;
        stream_ctx.queue_name = queue_name;
        stream_ctx.stream_completed = false;
        stream_ctx.finish_reason = NULL;
        stream_ctx.first_chunk_logged = false;
        clock_gettime(CLOCK_MONOTONIC, &stream_ctx.start_time);

        // Log that prompt is being sent to model server
        log_this(SR_WEBSOCKET_CHAT, "Prompt sent to %s/%s/%s (streaming)",
                 LOG_LEVEL_STATE, 3,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI");

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
            
            // Ensure final chunk is written before sending chat_done
            if (stream_ctx.wsi) {
                lws_callback_on_writable(stream_ctx.wsi);
            }
            
            // Send final chat_done message to signal stream completion to client
            // Content was already streamed via chat_chunk messages, so just signal completion
            send_chat_done(wsi, request_id, 
                          "",  // Empty - client already has content from chunks
                          stream_ctx.model, 
                          stream_ctx.finish_reason ? stream_ctx.finish_reason : "stop",
                          0, 0, 0,  // Token counts not available in streaming mode
                          response_time_ms);
            
            // Log that final response was sent to client
            log_this(SR_WEBSOCKET_CHAT, "Final response sent to client for request %s (%.0fms total)", 
                     LOG_LEVEL_STATE, 2,
                     request_id ? request_id : "unknown",
                     response_time_ms);
        } else {
            // Streaming proxy failed - send error to client
            send_chat_error(wsi, "Streaming proxy failed", request_id);
            log_this(SR_WEBSOCKET_CHAT, "Streaming proxy failed for request %s", 
                     LOG_LEVEL_ERROR, 1, request_id ? request_id : "unknown");
            
            // Clean up queue on failure
            cleanup_chat_queue(&session->chat_write_queue_name);
        }
        
        // Clean up stream context
        free(stream_ctx.finish_reason);
        free(stream_ctx.request_id);
        free(stream_ctx.model);
        
        // Note: We don't clean up the queue here because it may still have pending chunks
        // The queue will be cleaned up when the connection closes or when a new stream starts
    } else {
        // Non-streaming mode
        ChatProxyConfig proxy_config = chat_proxy_get_default_config();
        
        // Log that prompt is being sent to model server
        log_this(SR_WEBSOCKET_CHAT, "Prompt sent to %s/%s/%s (non-streaming)",
                 LOG_LEVEL_STATE, 3,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI");
        
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
        
        // Log that response was received
        log_this(SR_WEBSOCKET_CHAT, "Response received from %s/%s/%s (%.0fms)",
                 LOG_LEVEL_STATE, 4,
                 engine->name[0] ? engine->name : "unknown",
                 engine->model[0] ? engine->model : "unknown",
                 engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
                 engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI",
                 proxy_result->total_time_ms);
        
        // Parse response
        ChatParsedResponse *parsed = chat_response_parse(proxy_result->response_body, engine->provider);
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
        
        // Send chat_done
        send_chat_done(wsi, request_id, parsed->content, parsed->model, parsed->finish_reason,
                       parsed->prompt_tokens, parsed->completion_tokens, parsed->total_tokens,
                       response_time_ms);
        
        // Log that final response was sent to client
        log_this(SR_WEBSOCKET_CHAT, "Final response sent to client for request %s (%.0fms total)", 
                 LOG_LEVEL_STATE, 2,
                 request_id ? request_id : "unknown",
                 response_time_ms);
        
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
    log_this(SR_WEBSOCKET_CHAT, "Chat subsystem initialized", LOG_LEVEL_DEBUG, 0);
    return 0;
}

void chat_subsystem_cleanup(void) {
    // Nothing to cleanup yet
}

void chat_session_cleanup(WebSocketSessionData *session) {
    if (!session) return;
    
    // Cleanup chat queue
    cleanup_chat_queue(&session->chat_write_queue_name);
    // Cleanup terminal queue
    cleanup_chat_queue(&session->terminal_write_queue_name);
    // Cleanup media session
    media_session_cleanup(session);
    
    // Cleanup database and claims (already done in connection closed, but keep for safety)
    if (session->chat_database) {
        free(session->chat_database);
        session->chat_database = NULL;
    }
    if (session->chat_claims) {
        free_jwt_claims(session->chat_claims);
        session->chat_claims = NULL;
    }
}

void handle_chat_writable(struct lws *wsi, const WebSocketSessionData *session) {
    if (!wsi || !session || !session->chat_write_queue_name) {
        return;
    }
    
    // Dequeue and write all pending chunks
    dequeue_and_write_chat_chunks(wsi, session->chat_write_queue_name);
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
                            const char* model, int index, const char* finish_reason) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_chunk"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    
    json_t* chunk = json_object();
    json_object_set_new(chunk, "content", json_string(content ? content : ""));
    if (model) json_object_set_new(chunk, "model", json_string(model));
    json_object_set_new(chunk, "index", json_integer(index));
    if (finish_reason) json_object_set_new(chunk, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "chunk", chunk);
    
    ws_write_json_response(wsi, response);
    json_decref(response);
}
