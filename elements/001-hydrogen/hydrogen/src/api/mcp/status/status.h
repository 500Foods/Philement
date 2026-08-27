/*
 * MCP Status API Endpoint
 *
 * Implements GET /api/mcp/status returning listen config, accept flags, and counters.
 */

#ifndef MCP_API_STATUS_H
#define MCP_API_STATUS_H

#include <jansson.h>
#include <microhttpd.h>
#include <src/mcp/mcp.h>

/*
 * Handle GET /api/mcp/status requests.
 *
 * Requires a valid JWT Bearer token. Returns enabled/listen/protocol, Bearer
 * accept flags, Resource, thread counts, and MCP counters. No tokens or JWKS.
 */
//@ swagger:path /api/mcp/status
//@ swagger:method GET
//@ swagger:operationId mcpStatus
//@ swagger:tags "MCP Service"
//@ swagger:summary Get MCP status counters
//@ swagger:description Returns a snapshot of MCP listen configuration, token accept flags, Resource URI, thread counts, and RPC/session counters. Requires a valid JWT Bearer token with a database claim. Distinct from unauthenticated GET on the MCP daemon Path/healthz. Does not include tokens or JWKS.
//@ swagger:security bearerAuth
//@ swagger:response 200 application/json {"type":"object","required":["success","enabled","initialized","listen","protocol","accept_hydrogen_jwt","accept_oidc_idp","accept_oidc_rp","resource","thread_count","thread_pool_size","counters"],"properties":{"success":{"type":"boolean","example":true},"enabled":{"type":"boolean","description":"Whether MCP is enabled in config","example":false},"initialized":{"type":"boolean","description":"Whether the MCP runtime has been initialized","example":false},"listen":{"type":"object","required":["interface","port","path"],"properties":{"interface":{"type":"string","example":"127.0.0.1"},"port":{"type":"integer","example":3100},"path":{"type":"string","example":"/mcp"}}},"protocol":{"type":["string","null"],"description":"Lua protocol script Group.Name","example":"Mcp.Server"},"accept_hydrogen_jwt":{"type":"boolean","example":true},"accept_oidc_idp":{"type":"boolean","example":false},"accept_oidc_rp":{"type":"boolean","example":false},"resource":{"type":"string","description":"Canonical MCP resource URI","example":"http://127.0.0.1:3100/mcp"},"thread_count":{"type":"integer","description":"Live MCP service threads","example":0},"thread_pool_size":{"type":"integer","description":"Configured MHD thread pool size","example":4},"counters":{"type":"object","required":["sessions_active","sessions_total","sessions_expired","rpc_received","rpc_succeeded","rpc_failed","rpc_in_flight","auth_rejected","auth_rejected_reasons","origin_rejected","dispatch_timeouts","bytes_in","bytes_out","last_rpc_at"],"properties":{"sessions_active":{"type":"integer","example":0},"sessions_total":{"type":"integer","example":0},"sessions_expired":{"type":"integer","example":0},"rpc_received":{"type":"integer","example":0},"rpc_succeeded":{"type":"integer","example":0},"rpc_failed":{"type":"integer","example":0},"rpc_in_flight":{"type":"integer","example":0},"auth_rejected":{"type":"integer","example":0},"auth_rejected_reasons":{"type":"object","properties":{"missing":{"type":"integer","example":0},"malformed":{"type":"integer","example":0},"hydrogen_jwt":{"type":"integer","example":0},"oidc_idp":{"type":"integer","example":0},"oidc_rp":{"type":"integer","example":0},"aud":{"type":"integer","example":0},"scope":{"type":"integer","example":0}}},"origin_rejected":{"type":"integer","example":0},"dispatch_timeouts":{"type":"integer","example":0},"bytes_in":{"type":"integer","example":0},"bytes_out":{"type":"integer","example":0},"last_rpc_at":{"type":"integer","description":"Unix epoch of last RPC (0 if none)","example":0}}}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"JWT validation failure","example":"Invalid or expired JWT token"}}}
//@ swagger:response 405 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Method not allowed"},"message":{"type":"string","example":"Only GET requests are supported"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Failed to build status response"}}}
enum MHD_Result handle_mcp_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

json_t *mcp_status_json_string_or_null(const char *value);
json_t *mcp_status_build_counters(const McpMetrics *metrics);
enum MHD_Result mcp_status_send_response(
    struct MHD_Connection *connection,
    const McpStatusSnapshot *snap);

#endif /* MCP_API_STATUS_H */
