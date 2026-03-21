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
 * Parse and validate auth_chat request JSON
 * Returns true if valid, false if error response was sent
 */
bool auth_chat_parse_request(json_t *request_json,
                              char **engine,
                              json_t **messages,
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
                                  double response_time_ms);

/**
 * Build auth_chat error response
 */
json_t* auth_chat_build_error_response(const char *error_message);

#endif // AUTH_CHAT_H
