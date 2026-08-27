#ifndef HYDROGEN_MCP_RPC_H
#define HYDROGEN_MCP_RPC_H

#include <stdbool.h>
#include <stddef.h>
#include <jansson.h>

#define MCP_RPC_PARSE_ERROR -32700
#define MCP_RPC_INVALID_REQUEST -32600
#define MCP_RPC_SESSION_LIMIT -32001
#define MCP_RPC_OVERLOAD -32000
#define MCP_RPC_INTERNAL_ERROR -32603
#define MCP_DEFAULT_PROTOCOL_VERSION "2025-03-26"

typedef enum {
    MCP_RPC_OK = 0,
    MCP_RPC_ERR_PARSE,
    MCP_RPC_ERR_INVALID,
    MCP_RPC_ERR_OVERSIZE
} McpRpcStatus;

typedef struct McpRpcEnvelope {
    json_t *root;
    json_t *id;
    json_t *params;
    char *method;
    char *protocol_version;
    bool is_notification;
} McpRpcEnvelope;

void mcp_rpc_envelope_cleanup(McpRpcEnvelope *env);
McpRpcStatus mcp_rpc_parse(const char *body, size_t body_len, int max_body_bytes,
                           const char *protocol_version_header, McpRpcEnvelope *out);
char *mcp_rpc_make_error(const json_t *id, int code, const char *message);
bool mcp_rpc_is_initialize(const McpRpcEnvelope *env);
const char *mcp_rpc_status_message(McpRpcStatus status);
int mcp_rpc_status_code(McpRpcStatus status);

#endif
