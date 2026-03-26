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

#endif // AUTH_CHAT_STREAM_H
