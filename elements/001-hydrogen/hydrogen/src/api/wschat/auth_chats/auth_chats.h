/*
 * Authenticated Multi-Model Chat API Endpoint
 *
 * Handles broadcast chat requests to multiple AI models simultaneously.
 * JWT authentication required. Database extracted from token claims.
 */

#ifndef AUTH_CHATS_H
#define AUTH_CHATS_H

// Project includes
#include <src/hydrogen.h>
#include <microhttpd.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/req_builder.h>
#include <src/api/auth/auth_service.h>

/**
 * Handle POST /api/conduit/auth_chats request
 *
 * Broadcasts a single conversation to multiple AI models in parallel.
 *
 * Request body:
 * {
 *   "messages": [
 *     {"role": "user", "content": "Explain HTTP vs WebSocket"}
 *   ],
 *   "engines": ["gpt-4", "claude-3", "llama-3"],
 *   "temperature": 0.5,
 *   "max_tokens": 500
 * }
 *
 * Response:
 * {
 *   "success": true,
 *   "database": "Acuranzo",
 *   "broadcast_id": "uuid",
 *   "engines_requested": 3,
 *   "engines_succeeded": 3,
 *   "engines_failed": 0,
 *   "results": [
 *     {"engine": "gpt-4", "success": true, "content": "...", "tokens": {...}},
 *     {"engine": "claude-3", "success": true, "content": "...", "tokens": {...}}
 *   ],
 *   "timing": {"total_time_ms": 2100}
 * }
 */
enum MHD_Result handle_auth_chats_request(struct MHD_Connection *connection,
                                            const char *url,
                                            const char *method,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls);

/* ----------------------------------------------------------------------------
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 * -------------------------------------------------------------------------- */
void auth_chats_generate_broadcast_id(char *buffer, size_t buffer_size);

jwt_validation_result_t auth_chats_validate_jwt_from_header(const char *auth_header);

json_t *auth_chats_build_error_response(const char *error_message);

bool auth_chats_parse_request(json_t *request_json,
                              json_t **engines,
                              json_t **messages,
                              size_t *engine_count,
                              double *temperature,
                              int *max_tokens,
                              char **error_message);

ChatMessage *auth_chats_messages_json_to_list(json_t *messages);

ChatRequestParams auth_chats_resolve_request_params(const ChatEngineConfig *engine,
                                                     double temperature,
                                                     int max_tokens);

size_t auth_chats_build_multi_requests(ChatEngineCache *cache,
                                       json_t *engines,
                                       const ChatMessage *messages,
                                       double temperature,
                                       int max_tokens,
                                       ChatMultiRequest *requests);

void auth_chats_free_multi_requests(ChatMultiRequest *requests, size_t request_count);

char **auth_chats_copy_engine_names(const ChatMultiRequest *requests, size_t request_count);
void auth_chats_free_engine_names(char **engine_names, size_t engine_count);

json_t *auth_chats_build_response(const char *database,
                                  const char *broadcast_id,
                                  size_t engines_requested,
                                  char **engine_names,
                                  const ChatMultiResult *multi_result);

#endif // AUTH_CHATS_H
