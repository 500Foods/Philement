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

#endif // AUTH_CHATS_H
