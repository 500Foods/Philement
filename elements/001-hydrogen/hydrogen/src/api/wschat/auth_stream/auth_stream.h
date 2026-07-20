/*
 * Authenticated Chat Stream API Endpoint
 *
 * Handles streaming chat requests with JWT authentication.
 * Extracts database from JWT claims, supports multimodal content.
 * Streams responses via Server-Sent Events (SSE).
 */

#ifndef AUTH_CHAT_STREAM_H
#define AUTH_CHAT_STREAM_H

// Project includes
#include <src/hydrogen.h>
#include <microhttpd.h>
#include <src/api/auth/auth_service.h>
#include <src/api/wschat/helpers/engine_cache.h>

/**
 * Handle POST /api/conduit/auth_chat/stream request
 *
 * Request body:
 * {
 *   "messages": [
 *     {"role": "system", "content": "You are helpful."},
 *     {"role": "user", "content": "Hello!"}
 *   ],
 *   "engine": "gpt-4",
 *   "temperature": 0.7,
 *   "max_tokens": 1000,
 *   "stream": true
 * }
 *
 * Response: Server-Sent Events stream
 *   data: {"id":"chatcmpl-123","model":"gpt-4","content":"Hello"}
 *   data: {"id":"chatcmpl-123","model":"gpt-4","content":" there!"}
 *   data: {"id":"chatcmpl-123","model":"gpt-4","finish_reason":"stop"}
 *   data: [DONE]
 */
enum MHD_Result handle_auth_chat_stream_request(struct MHD_Connection *connection,
                                                const char *url,
                                                const char *method,
                                                const char *upload_data,
                                                size_t *upload_data_size,
                                                void **con_cls);

/**
 * Parse and validate auth_chat_stream request JSON
 * Returns true if valid, false if error response was sent
 * Note: context_hashes is optional - if provided, used for bandwidth optimization
 */
bool auth_chat_stream_parse_request(json_t *request_json,
                                    char **engine,
                                    json_t **messages,
                                    char ***context_hashes,
                                    size_t *context_hash_count,
                                    double *temperature,
                                    int *max_tokens,
                                    bool *stream,
                                    char **error_message);

/**
 * Build auth_chat_stream error response (JSON, not streamed)
 */
json_t* auth_chat_stream_build_error_response(const char *error_message);

/**
 * Extract Bearer token and validate JWT (Unity-testable helper).
 */
jwt_validation_result_t auth_stream_validate_jwt_from_header(const char *auth_header);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are exposed
 * (non-static) solely so the Unity test framework can call them directly.
 * -------------------------------------------------------------------------- */
void auth_stream_add_sse_headers(struct MHD_Response *response);

enum MHD_Result auth_stream_send_sse_response_headers(struct MHD_Connection *connection);

enum MHD_Result auth_stream_send_sse_error_response(struct MHD_Connection *connection,
                                                    const char *error_message,
                                                    unsigned int status_code);

enum MHD_Result auth_stream_chat_response(struct MHD_Connection *connection,
                                          const ChatEngineConfig *engine,
                                          const char *request_json_str,
                                          const char *database);

#endif // AUTH_CHAT_STREAM_H
