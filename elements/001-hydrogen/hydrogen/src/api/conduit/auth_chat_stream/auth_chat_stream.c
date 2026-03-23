/*
 * Authenticated Chat Stream API Endpoint Implementation
 *
 * Handles streaming chat requests with JWT authentication.
 * Phase 11: Streaming support via Server-Sent Events.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>

// Local includes
#include "auth_chat_stream.h"
#include "../auth_chat/auth_chat.h"
#include "../chat_common/chat_engine_cache.h"
#include "../chat_common/chat_request_builder.h"
#include "../chat_common/chat_response_parser.h"
#include "../chat_common/chat_proxy.h"
#include "../chat_common/chat_metrics.h"
#include "../chat_common/chat_storage.h"
#include "../chat_common/chat_context_hashing.h"
#include "../chat_common/chat_lru_cache.h"
#include "../conduit_service.h"

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Logging source for streaming
static const char* SR_AUTH_CHAT_STREAM = "AUTH_CHAT_STREAM";

// Forward declarations for internal helpers
static enum MHD_Result send_sse_error_response(struct MHD_Connection *connection,
                                               const char *error_message,
                                               unsigned int status_code);

static enum MHD_Result send_sse_response_headers(struct MHD_Connection *connection);

static enum MHD_Result stream_chat_response(struct MHD_Connection *connection,
                                            const ChatEngineConfig *engine,
                                            const char *request_json_str,
                                            const char *database);

/**
 * Handle POST /api/conduit/auth_chat/stream request
 */
enum MHD_Result handle_auth_chat_stream_request(struct MHD_Connection *connection,
                                                const char *url,
                                                const char *method,
                                                const char *upload_data,
                                                size_t *upload_data_size,
                                                void **con_cls) {
    (void)url;
    log_this(SR_AUTH_CHAT_STREAM, "Stream request received", LOG_LEVEL_DEBUG, 0);

    // Use common POST body buffering
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

    if (buf_result == API_BUFFER_CONTINUE) {
        return MHD_YES;
    }
    if (buf_result == API_BUFFER_ERROR) {
        return api_send_error_and_cleanup(connection, con_cls, "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Validate HTTP method
    if (strcmp(method, "POST") != 0) {
        api_free_post_buffer(con_cls);
        json_t *error = auth_chat_stream_build_error_response("Method not allowed - use POST");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
        json_decref(error);
        return ret;
    }

    // Extract and validate JWT
    jwt_validation_result_t jwt_result = {0};
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");

    if (!extract_and_validate_jwt(auth_header, &jwt_result)) {
        api_free_post_buffer(con_cls);
        const char *error_msg = jwt_result.claims ? "Invalid token" : "Authentication required";
        json_t *error = auth_chat_stream_build_error_response(error_msg);
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_UNAUTHORIZED);
        json_decref(error);
        free_jwt_validation_result(&jwt_result);
        return ret;
    }

    // Validate claims
    if (!validate_jwt_claims(&jwt_result, connection)) {
        api_free_post_buffer(con_cls);
        free_jwt_validation_result(&jwt_result);
        return MHD_NO;
    }

    // Extract database from JWT claim
    const char *database = jwt_result.claims->database;
    if (!database) {
        api_free_post_buffer(con_cls);
        json_t *error = auth_chat_stream_build_error_response("Token missing database claim");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_FORBIDDEN);
        json_decref(error);
        free_jwt_validation_result(&jwt_result);
        return ret;
    }

    // Parse request JSON
    json_error_t json_error;
    json_t *request_json = json_loads(buffer->data, 0, &json_error);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        free_jwt_validation_result(&jwt_result);
        json_t *error_response = auth_chat_stream_build_error_response("Invalid JSON in request body");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return ret;
    }

    // Parse request parameters (reuse auth_chat_parse_request)
    char *engine_name = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    // Note: auth_chat_parse_request expects a 'stream' parameter; we'll ignore it
    bool parse_ok = auth_chat_parse_request(request_json, &engine_name,
                                             &messages, &context_hashes,
                                             &context_hash_count,
                                             &temperature, &max_tokens,
                                             &stream, &error_message);

    if (!parse_ok) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        json_t *error_response = auth_chat_stream_build_error_response(error_message ? error_message : "Invalid request");
        free(error_message);
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return ret;
    }

    // Force stream = true for this endpoint
    stream = true;

    // Get database queue
    DatabaseQueue *db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return send_sse_error_response(connection, "Database not found", MHD_HTTP_BAD_REQUEST);
    }

    // Get CEC
    ChatEngineCache *cec = db_queue->chat_engine_cache;
    if (!cec) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return send_sse_error_response(connection, "Chat not enabled for this database", MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    // Look up engine
    const ChatEngineConfig *engine = NULL;
    if (engine_name) {
        engine = chat_engine_cache_lookup_by_name(cec, engine_name);
    } else {
        engine = chat_engine_cache_get_default(cec);
    }

    if (!engine) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return send_sse_error_response(connection, engine_name ? "Engine not found" : "No default engine configured", MHD_HTTP_BAD_REQUEST);
    }

    // Check engine health
    if (!engine->is_healthy) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return send_sse_error_response(connection, "Engine is currently unavailable", MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    // Prepare request JSON string for proxy
    char *request_json_str = json_dumps(request_json, JSON_COMPACT);
    json_decref(request_json);
    if (!request_json_str) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return send_sse_error_response(connection, "Failed to serialize request", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Send SSE headers
    enum MHD_Result ret = send_sse_response_headers(connection);
    if (ret != MHD_YES) {
        free(request_json_str);
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return ret;
    }

    // Stream the response
    ret = stream_chat_response(connection, engine, request_json_str, database);

    // Cleanup
    free(request_json_str);
    free(engine_name);
    free_jwt_validation_result(&jwt_result);
    chat_context_free_hash_array(context_hashes, context_hash_count);

    return ret;
}

/**
 * Parse and validate auth_chat_stream request JSON
 */
bool auth_chat_stream_parse_request(json_t *request_json,
                                    char **engine,
                                    json_t **messages,
                                    char ***context_hashes,
                                    size_t *context_hash_count,
                                    double *temperature,
                                    int *max_tokens,
                                    bool *stream,
                                    char **error_message) {
    // Reuse auth_chat_parse_request (same format)
    return auth_chat_parse_request(request_json, engine, messages, context_hashes,
                                   context_hash_count, temperature, max_tokens,
                                   stream, error_message);
}

/**
 * Build auth_chat_stream error response (JSON, not streamed)
 */
json_t* auth_chat_stream_build_error_response(const char *error_message) {
    // Reuse auth_chat_build_error_response
    return auth_chat_build_error_response(error_message);
}

// ============================================================================
// Internal helper functions
// ============================================================================

static enum MHD_Result send_sse_error_response(struct MHD_Connection *connection,
                                               const char *error_message,
                                               unsigned int status_code) {
    // For streaming endpoint, we still send JSON error (not SSE) because stream hasn't started
    json_t *error = auth_chat_stream_build_error_response(error_message);
    enum MHD_Result ret = api_send_json_response(connection, error, status_code);
    json_decref(error);
    return ret;
}

static enum MHD_Result send_sse_response_headers(struct MHD_Connection *connection) {
    struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", "text/event-stream");
    MHD_add_response_header(response, "Cache-Control", "no-cache");
    MHD_add_response_header(response, "Connection", "keep-alive");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Authorization, Content-Type");
    MHD_add_response_header(response, "Access-Control-Expose-Headers", "Content-Type");

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

static enum MHD_Result stream_chat_response(struct MHD_Connection *connection,
                                            const ChatEngineConfig *engine,
                                            const char *request_json_str,
                                            const char *database) {
    (void)engine;
    (void)request_json_str;
    (void)database;
    // TODO: Implement streaming proxy
    // For now, send a single error event and finish
    const char *event = "event: error\ndata: {\"error\": \"Streaming not yet implemented\"}\n\n";
    size_t event_len = strlen(event);
    struct MHD_Response *response = MHD_create_response_from_buffer(event_len, (void*)event, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", "text/event-stream");
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}
