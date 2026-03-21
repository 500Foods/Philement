/**
 * @file alt_chats.h
 * @brief Alternative Authenticated Multi-Model Chat API Endpoint
 *
 * Handles broadcast chat requests to multiple AI models simultaneously with
 * database override capability. Extracts database from request body instead
 * of JWT claims for cross-database operations.
 *
 * @author Hydrogen Framework
 * @date 2026-03-21
 */

#ifndef ALT_CHATS_H
#define ALT_CHATS_H

// Project includes
#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * Handle POST /api/conduit/alt_chats request
 *
 * Request body:
 * {
 *   "database": "ClientDB",
 *   "messages": [
 *     {"role": "user", "content": "Explain quantum computing"}
 *   ],
 *   "engines": ["gpt-4", "claude-3", "llama-3"],
 *   "temperature": 0.7,
 *   "max_tokens": 1000
 * }
 *
 * Response:
 * {
 *   "success": true,
 *   "database": "ClientDB",
 *   "broadcast_id": "uuid",
 *   "engines_requested": 3,
 *   "engines_succeeded": 3,
 *   "engines_failed": 0,
 *   "results": [...],
 *   "timing": {"total_time_ms": 2100}
 * }
 */
enum MHD_Result handle_alt_chats_request(struct MHD_Connection *connection,
                                          const char *url,
                                          const char *method,
                                          const char *upload_data,
                                          size_t *upload_data_size,
                                          void **con_cls);

/**
 * Parse and validate alt_chats request JSON
 * Returns true if valid, false if error response was sent
 */
bool alt_chats_parse_request(json_t *request_json,
                              char **database,
                              json_t **engines,
                              json_t **messages,
                              double *temperature,
                              int *max_tokens,
                              char **error_message);

/**
 * Build alt_chats success response
 */
json_t* alt_chats_build_response(const char *database,
                                  const char *broadcast_id,
                                  size_t engines_requested,
                                  size_t engines_succeeded,
                                  size_t engines_failed,
                                  json_t *results,
                                  double total_time_ms);

/**
 * Build alt_chats error response
 */
json_t* alt_chats_build_error_response(const char *error_message);

#endif // ALT_CHATS_H
