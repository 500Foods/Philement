/*
 * System Info API endpoint for the Hydrogen Project.
 * Provides system-level information.
 */

#ifndef HYDROGEN_SYSTEM_INFO_H
#define HYDROGEN_SYSTEM_INFO_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>

// Third-party includes
#include <microhttpd.h>
#include <jansson.h>

// Project includes
#include <src/config/config.h>
#include <src/state/state.h>
#include <src/websocket/websocket_server_internal.h>

/**
 * Handles the /api/system/info endpoint request.
 * Returns system information in JSON format including:
 * - Hardware information
 * - Operating system details
 * - Runtime statistics
 * - Version information
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/info
//@ swagger:method GET
//@ swagger:operationId getSystemInfo
//@ swagger:tags "System Service"
//@ swagger:summary System information endpoint
//@ swagger:description Returns comprehensive system information in JSON format including hardware details, operating system information, runtime statistics, and version information.
//@ swagger:response 200 application/json {"type":"object","properties":{"hardware":{"type":"object"},"os":{"type":"object"},"runtime":{"type":"object"},"version":{"type":"object"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection);

/**
 * Extract WebSocket metrics for testing purposes.
 * This function is made non-static to enable unit testing.
 *
 * @param metrics Pointer to WebSocketMetrics structure to fill
 */
void extract_websocket_metrics(WebSocketMetrics *metrics);

/* ----------------------------------------------------------------------------
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 * -------------------------------------------------------------------------- */
bool system_info_has_valid_jwt(struct MHD_Connection *connection);

/**
  * Build the system info JSON object using the shared C collectors.
  *
  * When include_scripting is true, the scripting scoreboard snapshot is
  * attached as the "scripting" key.
  *
  * When has_terminal is true (a valid JWT with the terminal role, the
  * WebSocket server running, and Terminal.Enabled), a "terminal" object
  * with the absolute WebSocket URL, the terminal protocol, and the
  * terminal key is attached. The terminal key is ws_context->terminal_auth_key
  * (Terminal.Key), never ws_context->auth_key (the chat key). Unauthenticated,
  * invalid-JWT, and valid-but-non-terminal callers receive no terminal object.
  *
  * auth_mode controls the public-vs-JWT shape:
  *   NULL           — public (no/invalid JWT): short payload, no FD/system
  *                    enumeration, version.auth absent, status.server_running
  *                    only. The Lua H.system.info() path passes NULL so it
  *                    never sees version.auth.
  *   "none"         — same as NULL but for the REST path (explicit).
  *   "jwt"          — valid JWT without terminal role: full dump + scripting.
  *   "jwt, terminal"— valid JWT with terminal role: full dump + scripting +
  *                    terminal object. version.auth = "jwt, terminal".
  *
  * This is the single function both handle_system_info_request (REST) and
  * H.system.info() (Lua) call; the Lua path passes auth_mode=NULL and
  * include_scripting=true so script sandboxes never receive version.auth or
  * terminal data. The caller owns the returned json_t*.
  */
json_t* system_info_build_json(bool include_scripting, bool has_terminal, const char *auth_mode);

/*
 * Check whether JWT claims carry the terminal role (role_id 32).
 * Returns true only when claims are present and `roles` contains the
 * terminal role token; never treats admin/chat/wildcard as a grant.
 */
bool system_info_has_terminal_role(const jwt_claims_t *claims);

#endif /* HYDROGEN_SYSTEM_INFO_H */
