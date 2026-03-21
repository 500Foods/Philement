/**
 * @file alt_chat.h
 * @brief Alternative Authenticated Chat API Endpoint
 *
 * Handles single-model chat requests with JWT authentication and database override.
 * Extracts database from request body instead of JWT claims, enabling cross-database
 * chat operations with centralized management database.
 *
 * @author Hydrogen Framework
 * @date 2026-03-21
 */

#ifndef ALT_CHAT_H
#define ALT_CHAT_H

// Project includes
#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * Handle POST /api/conduit/alt_chat request
 *
 * Request body:
 * {
 *   "database": "ClientDB",
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
 *   "database": "ClientDB",
 *   "engine": "gpt-4",
 *   "model": "gpt-4-turbo",
 *   "content": "Hello! How can I help?",
 *   "tokens": {"prompt": 25, "completion": 12, "total": 37},
 *   "response_time_ms": 850
 * }
 */
enum MHD_Result handle_alt_chat_request(struct MHD_Connection *connection,
                                         const char *url,
                                         const char *method,
                                         const char *upload_data,
                                         size_t *upload_data_size,
                                         void **con_cls);

/**
 * Parse and validate alt_chat request JSON
 * Returns true if valid, false if error response was sent
 */
bool alt_chat_parse_request(json_t *request_json,
                             char **database,
                             char **engine,
                             json_t **messages,
                             double *temperature,
                             int *max_tokens,
                             bool *stream,
                             char **error_message);

/**
 * Build alt_chat success response
 */
json_t* alt_chat_build_response(const char *database,
                                 const char *engine,
                                 const char *model,
                                 const char *content,
                                 int prompt_tokens,
                                 int completion_tokens,
                                 int total_tokens,
                                 const char *finish_reason,
                                 double response_time_ms);

/**
 * Build alt_chat error response
 */
json_t* alt_chat_build_error_response(const char *error_message);

#endif // ALT_CHAT_H
