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
#include "websocket_server_chat_internal.h"
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

int send_chat_proxy_result(struct lws *wsi, const char* request_id,
                           const ChatEngineConfig* engine,
                           ChatProxyResult* proxy_result,
                           struct timespec start_time,
                           size_t message_count) {
    if (!proxy_result || proxy_result->code != CHAT_PROXY_OK) {
        const char *error_msg = proxy_result && proxy_result->error_message ?
                                proxy_result->error_message : "Request failed";
        send_chat_error(wsi, error_msg, request_id);
        log_this(SR_WEBSOCKET_CHAT, "Non-streaming request failed for request %s: %s",
                 LOG_LEVEL_ERROR, 2, request_id ? request_id : "unknown", error_msg);
        chat_proxy_result_destroy(proxy_result);
        return -1;
    }

    size_t response_size = proxy_result->response_body ? strlen(proxy_result->response_body) : 0;
    log_this(SR_WEBSOCKET_CHAT, "Response received from %s/%s/%s (%.0fms, %zu bytes)",
             LOG_LEVEL_DEBUG, 4,
             engine->name[0] ? engine->name : "unknown",
             engine->model[0] ? engine->model : "unknown",
             engine->provider == CEC_PROVIDER_ANTHROPIC ? "Anthropic" :
             engine->provider == CEC_PROVIDER_OLLAMA ? "Ollama" : "OpenAI",
             proxy_result->total_time_ms,
             response_size);

    if (proxy_result->response_body && response_size > 0) {
        log_this(SR_WEBSOCKET_CHAT, "Raw response start (%zu bytes): %.300s", LOG_LEVEL_DEBUG, 2,
                 response_size, proxy_result->response_body);
        if (strstr(proxy_result->response_body, "\"retrieval\"")) {
            log_this(SR_WEBSOCKET_CHAT, "Raw response CONTAINS retrieval field", LOG_LEVEL_DEBUG, 0);
        } else {
            log_this(SR_WEBSOCKET_CHAT, "Raw response does NOT contain retrieval field", LOG_LEVEL_DEBUG, 0);
        }
    }

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
        return -1;
    }

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double response_time_ms = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                              (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    size_t content_length = parsed->content ? strlen(parsed->content) : 0;
    log_this(SR_WEBSOCKET_CHAT, "Sending response: content=%zu bytes, messages=%zu, finish=%s",
             LOG_LEVEL_DEBUG, 3,
             content_length, message_count,
             parsed->finish_reason ? parsed->finish_reason : "null");

    if (parsed->raw_response) {
        log_this(SR_WEBSOCKET_CHAT, "parsed->raw_response EXISTS, has %zu keys", LOG_LEVEL_DEBUG, 1,
                 json_object_size(parsed->raw_response));
    } else {
        log_this(SR_WEBSOCKET_CHAT, "parsed->raw_response is NULL!", LOG_LEVEL_ERROR, 0);
    }

    send_chat_done(wsi, request_id, parsed->content, parsed->model, parsed->finish_reason,
                   parsed->prompt_tokens, parsed->completion_tokens, parsed->total_tokens,
                   response_time_ms, parsed->raw_response);

    log_this(SR_WEBSOCKET_CHAT, "Final response sent to client for request %s (%.0fms total, %zu bytes content)",
             LOG_LEVEL_STATE, 2,
             request_id ? request_id : "unknown",
             response_time_ms,
             content_length);

    chat_parsed_response_destroy(parsed);
    chat_proxy_result_destroy(proxy_result);
    return 0;
}

ChatMessage* convert_json_messages_to_chat_messages(json_t *messages) {
    ChatMessage *chat_messages = NULL;
    if (!messages || !json_is_array(messages)) {
        return NULL;
    }

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

    return chat_messages;
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
        session->chat_database = strdup(jwt_result.claims->database);
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
    ChatMessage *chat_messages = convert_json_messages_to_chat_messages(messages);

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
        // cppcheck-suppress nullPointerRedundantCheck - session validated at function entry
        if (session->chat_stream_active || session->multi_stream_ctx) {
            log_this(SR_WEBSOCKET_CHAT, "Waiting for previous stream to complete before starting new one",
                     LOG_LEVEL_DEBUG, 0);
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
        stream_ctx->multi_stream_ctx = NULL;
        clock_gettime(CLOCK_MONOTONIC, &stream_ctx->start_time);

        // Mark stream as active on session BEFORE starting the stream
        // cppcheck: session guaranteed non-null due to check at function entry
        if (session) session->chat_stream_active = true;

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
            NULL,
            NULL,
            NULL,
            NULL
        );

        if (!multi_ctx) {
            send_chat_error(wsi, "Failed to start streaming", request_id);
            log_this(SR_WEBSOCKET_CHAT, "Failed to start multi-stream for request %s",
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

        stream_ctx->multi_stream_ctx = multi_ctx;
        session->multi_stream_ctx = multi_ctx;

        lws_callback_on_writable(wsi);

        log_this(SR_WEBSOCKET_CHAT, "Multi-stream started for request %s",
                 LOG_LEVEL_DEBUG, 1, request_id ? request_id : "unknown");

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

        if (send_chat_proxy_result(wsi, request_id, engine, proxy_result, start_time, message_count) != 0) {
            free(request_json_str);
            free(engine_name);
            chat_context_free_hash_array(context_hashes, context_hash_count);
            json_decref(request_json);
            return -1;
        }
    }

    // Cleanup
    free(request_json_str);
    free(engine_name);
    chat_context_free_hash_array(context_hashes, context_hash_count);
    json_decref(request_json);

    return 0;
}

int chat_subsystem_init(void) {
    MultiStreamManager* manager = chat_proxy_get_multi_manager();
    if (!manager) {
        log_this(SR_WEBSOCKET_CHAT, "Failed to get multi-stream manager", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    if (!chat_proxy_multi_init(manager, NULL)) {
        log_this(SR_WEBSOCKET_CHAT, "Failed to initialize multi-stream manager", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    log_this(SR_WEBSOCKET_CHAT, "Chat subsystem initialized with multi-stream manager", LOG_LEVEL_STATE, 0);
    return 0;
}

void chat_subsystem_cleanup(void) {
    MultiStreamManager* manager = chat_proxy_get_multi_manager();
    if (manager) {
        chat_proxy_multi_cleanup(manager);
    }
    log_this(SR_WEBSOCKET_CHAT, "Chat subsystem cleaned up", LOG_LEVEL_DEBUG, 0);
}

void chat_session_cleanup(WebSocketSessionData *session, struct lws *wsi) {
    if (!session) return;

    session->chat_stream_active = false;
    session->connection_valid = false;

    if (session->multi_stream_ctx) {
        MultiStreamManager* manager = chat_proxy_get_multi_manager();
        if (manager) {
            chat_proxy_multi_stream_stop(manager, session->multi_stream_ctx);
        }
        session->multi_stream_ctx = NULL;
    }

    if (session->chat_database) {
        free(session->chat_database);
        session->chat_database = NULL;
    }

    session->chat_claims = NULL;

    (void)wsi;
}

void handle_chat_writable(struct lws *wsi, WebSocketSessionData *session) {
    (void)wsi;

    if (!session || !session->multi_stream_ctx) {
        return;
    }

    MultiStreamContext* ctx = session->multi_stream_ctx;
    int written = chat_proxy_multi_drain_queue(ctx);

    if (written > 0) {
        log_this(SR_WEBSOCKET_CHAT, "Drained %d chunks from queue to WebSocket", LOG_LEVEL_TRACE, 1, written);
    }
}
