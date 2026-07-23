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
#include "auth_stream.h"
#include "../auth_chat/auth_chat.h"
#include "../helpers/engine_cache.h"
#include "../helpers/req_builder.h"
#include "../helpers/resp_parser.h"
#include "../helpers/proxy.h"
#include "../helpers/metrics.h"
#include "../helpers/storage.h"
#include "../helpers/context_hashing.h"
#include "../helpers/lru_cache.h"
#include <src/api/conduit/conduit_service.h>

// Unity test mocks (header-only, guarded by USE_MOCK_* defined by the Unity
// build). Remap JWT/API-buffer/DBQUEUE calls so the handler can be exercised
// without a live HTTP stack or database.
#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif
#if defined(USE_MOCK_DBQUEUE)
#include <unity/mocks/mock_dbqueue.h>
#endif
#if defined(USE_MOCK_API_UTILS)
#include <unity/mocks/mock_api_utils.h>
#endif

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Logging source for streaming
static const char* SR_AUTH_CHAT_STREAM = "AUTH_CHAT_STREAM";

// Forward declarations for internal helpers (non-static for Unity tests)
enum MHD_Result auth_stream_send_sse_error_response(struct MHD_Connection *connection,
                                                    const char *error_message,
                                                    unsigned int status_code);

enum MHD_Result auth_stream_send_sse_response_headers(struct MHD_Connection *connection);

enum MHD_Result auth_stream_chat_response(struct MHD_Connection *connection,
                                          const ChatEngineConfig *engine,
                                          const char *request_json_str,
                                          const char *database);

/**
 * Extract Bearer token and validate JWT. Local helper so Unity can mock
 * validate_jwt via USE_MOCK_AUTH_SERVICE_JWT (same pattern as auth_chat).
 */
jwt_validation_result_t auth_stream_validate_jwt_from_header(const char *auth_header) {
    jwt_validation_result_t result = {0};

    if (!auth_header) {
        result.valid = false;
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        result.valid = false;
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    return validate_jwt(auth_header + 7, NULL);
}

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
        // api_send_json_response takes ownership of error and decrefs it
        return api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    // Extract and validate JWT (local helper so mocks apply under Unity)
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    jwt_validation_result_t jwt_result = auth_stream_validate_jwt_from_header(auth_header);
    if (!jwt_result.valid) {
        api_free_post_buffer(con_cls);
        const char *error_msg = jwt_result.claims ? "Invalid token" : "Authentication required";
        json_t *error = auth_chat_stream_build_error_response(error_msg);
        free_jwt_validation_result(&jwt_result);
        return api_send_json_response(connection, error, MHD_HTTP_UNAUTHORIZED);
    }

    // Validate claims (shared helper — pure claim checks, no network)
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
        free_jwt_validation_result(&jwt_result);
        return api_send_json_response(connection, error, MHD_HTTP_FORBIDDEN);
    }

    // Parse request JSON
    json_error_t json_error;
    json_t *request_json = json_loads(buffer->data, 0, &json_error);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        free_jwt_validation_result(&jwt_result);
        json_t *error_response = auth_chat_stream_build_error_response("Invalid JSON in request body");
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
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
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
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
        return auth_stream_send_sse_error_response(connection, "Database not found", MHD_HTTP_BAD_REQUEST);
    }

    // Get CEC
    ChatEngineCache *cec = db_queue->chat_engine_cache;
    if (!cec) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return auth_stream_send_sse_error_response(connection, "Chat not enabled for this database", MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    // Look up engine
    const ChatEngineConfig *engine = NULL;
    if (engine_name) {
        engine = chat_engine_cache_lookup_by_name(cec, engine_name);
    } else {
        engine = chat_engine_cache_get_default(cec);
    }

    if (!engine) {
        const char *engine_err = engine_name ? "Engine not found" : "No default engine configured";
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return auth_stream_send_sse_error_response(connection, engine_err, MHD_HTTP_BAD_REQUEST);
    }

    // Check engine health
    if (!engine->is_healthy) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return auth_stream_send_sse_error_response(connection, "Engine is currently unavailable", MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    // Prepare request JSON string for proxy
    char *request_json_str = json_dumps(request_json, JSON_COMPACT);
    json_decref(request_json);
    if (!request_json_str) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        return auth_stream_send_sse_error_response(connection, "Failed to serialize request", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Stream the response (single MHD_queue_response — headers + body together).
    // MHD allows only one queued response per request; do not queue headers first.
    enum MHD_Result ret = auth_stream_chat_response(connection, engine, request_json_str, database);

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

enum MHD_Result auth_stream_send_sse_error_response(struct MHD_Connection *connection,
                                                    const char *error_message,
                                                    unsigned int status_code) {
    // For streaming endpoint, we still send JSON error (not SSE) because stream hasn't started.
    // api_send_json_response takes ownership of error and decrefs it.
    json_t *error = auth_chat_stream_build_error_response(error_message);
    return api_send_json_response(connection, error, status_code);
}

void auth_stream_add_sse_headers(struct MHD_Response *response) {
    if (!response) {
        return;
    }
    MHD_add_response_header(response, "Content-Type", "text/event-stream");
    MHD_add_response_header(response, "Cache-Control", "no-cache");
    MHD_add_response_header(response, "Connection", "keep-alive");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Authorization, Content-Type");
    MHD_add_response_header(response, "Access-Control-Expose-Headers", "Content-Type");
}

enum MHD_Result auth_stream_send_sse_response_headers(struct MHD_Connection *connection) {
    // Queues an empty SSE response with standard headers. Used by unit tests and
    // as a building block; the request handler uses auth_stream_chat_response
    // which attaches the same headers to the body in a single queue_response.
    struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }

    auth_stream_add_sse_headers(response);

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

enum MHD_Result auth_stream_chat_response(struct MHD_Connection *connection,
                                          const ChatEngineConfig *engine,
                                          const char *request_json_str,
                                          const char *database) {
    (void)engine;
    (void)request_json_str;
    (void)database;
    /* REST SSE multi-chunk proxy is not wired (use WebSocket chat stream).
     * Single SSE error event keeps the endpoint reachable for clients. */
    const char *event = "event: error\ndata: {\"error\": \"Streaming not yet implemented\"}\n\n";
    size_t event_len = strlen(event);
    struct MHD_Response *response = MHD_create_response_from_buffer(event_len, (void*)event, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }
    auth_stream_add_sse_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}
