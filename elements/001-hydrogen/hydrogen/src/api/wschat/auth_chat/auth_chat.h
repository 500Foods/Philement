/*
 * Authenticated Chat API Endpoint
 *
 * Handles single-model chat requests with JWT authentication.
 * Extracts database from JWT claims, supports multimodal content.
 */

#ifndef AUTH_CHAT_H
#define AUTH_CHAT_H

// Project includes
#include <src/hydrogen.h>
#include <microhttpd.h>
#include <src/api/auth/auth_service.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/req_builder.h>

/**
 * Handle POST /api/conduit/auth_chat request
 *
 * Request body:
 * {
 *   "messages": [
 *     {"role": "system", "content": "You are helpful."},
 *     {"role": "user", "content": "Hello!"}
 *   ],
 *   "engine": "gpt-4",
 *   "temperature": 0.7,
 *   "max_tokens": 1000
 * }
 *
 * Response:
 * {
 *   "success": true,
 *   "database": "Acuranzo",
 *   "engine": "gpt-4",
 *   "model": "gpt-4-turbo",
 *   "content": "Hello! How can I help?",
 *   "tokens": {"prompt": 25, "completion": 12, "total": 37},
 *   "response_time_ms": 850
 * }
 */
enum MHD_Result handle_auth_chat_request(struct MHD_Connection *connection,
                                          const char *url,
                                          const char *method,
                                          const char *upload_data,
                                          size_t *upload_data_size,
                                          void **con_cls);

/**
 * Validate Authorization Bearer header via validate_jwt().
 * Returns a jwt_validation_result_t (caller must free_jwt_validation_result).
 */
jwt_validation_result_t auth_chat_validate_jwt_from_header(const char *auth_header);

/**
 * Resolve ChatRequestParams from optional request fields and engine defaults.
 */
ChatRequestParams auth_chat_resolve_request_params(const ChatEngineConfig *engine,
                                                    double temperature,
                                                    int max_tokens,
                                                    bool stream);

/**
 * Convert a messages JSON array into a ChatMessage linked list.
 * Handles string content and multimodal array content (including media: refs).
 */
ChatMessage *auth_chat_messages_json_to_list(const char *database, json_t *messages);

/**
 * Extract message content as a newly allocated string (string or JSON array dump).
 * Returns NULL if content is missing/unsupported. Caller frees.
 */
char *auth_chat_content_to_string(const json_t *content_obj);

/**
 * Resolve multimodal content, expanding media:hash references when present.
 * On success returns a newly allocated string; caller frees.
 */
char *auth_chat_resolve_content_string(const char *database, json_t *content_obj);

/**
 * Check whether a serialized request exceeds the engine payload limit.
 * When exceeded, writes a human-readable message into error_buf.
 */
bool auth_chat_payload_exceeds_limit(size_t request_size,
                                      int max_payload_mb,
                                      char *error_buf,
                                      size_t error_buf_size);

/**
 * Result of collecting/storing conversation segment hashes.
 */
typedef struct AuthChatSegmentStats {
    const char **segment_hashes;
    size_t segment_hash_count;
    size_t hashes_used;
    size_t hashes_missed;
    size_t bandwidth_saved_bytes;
    double bandwidth_saved_percent;
} AuthChatSegmentStats;

/**
 * Store message/response segments (reusing client context hashes when valid).
 * On success fills stats (caller must auth_chat_free_segment_stats).
 * Returns false on allocation failure.
 */
bool auth_chat_collect_segment_stats(const char *database,
                                      json_t *messages,
                                      char **context_hashes,
                                      size_t context_hash_count,
                                      const char *response_content,
                                      AuthChatSegmentStats *stats);

/**
 * Free segment hash array allocated by auth_chat_collect_segment_stats.
 */
void auth_chat_free_segment_stats(AuthChatSegmentStats *stats);

/**
 * Attach context_hashing stats object onto a success response when hashes used.
 */
void auth_chat_attach_context_hashing_stats(json_t *response,
                                            const char *database,
                                            size_t hashes_used,
                                            size_t hashes_missed,
                                            size_t bandwidth_saved_bytes,
                                            double bandwidth_saved_percent);

/**
 * Parse and validate auth_chat request JSON
 * Returns true if valid, false if error response was sent
 * Note: context_hashes is optional - if provided, used for bandwidth optimization
 */
bool auth_chat_parse_request(json_t *request_json,
                              char **engine,
                              json_t **messages,
                              char ***context_hashes,
                              size_t *context_hash_count,
                              double *temperature,
                              int *max_tokens,
                              bool *stream,
                              char **error_message);

/**
 * Build auth_chat success response
 */
json_t* auth_chat_build_response(const char *database,
                                  const char *engine,
                                  const char *model,
                                  const char *content,
                                  int prompt_tokens,
                                  int completion_tokens,
                                  int total_tokens,
                                  const char *finish_reason,
                                  double response_time_ms,
                                  int convos_id,
                                  const char *convos_ref);

/**
 * Build auth_chat error response
 */
json_t* auth_chat_build_error_response(const char *error_message);

#endif // AUTH_CHAT_H
