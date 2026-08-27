#ifndef HYDROGEN_MCP_DISPATCH_H
#define HYDROGEN_MCP_DISPATCH_H

#include <microhttpd.h>

#include <src/config/config_mcp.h>
#include <src/mcp/mcp_auth.h>
#include <src/mcp/mcp_rpc.h>
#include <src/scripting/scripting_invoke.h>

typedef char *(*McpDispatchSubmitFn)(const char *script_name, const char *params_json);
typedef ScriptingWaitResult (*McpDispatchWaitFn)(const char *job_id, int timeout_seconds,
                                                 char **result_json_out);
typedef char *(*McpDispatchLoadSourceFn)(const char *script_name);

void mcp_dispatch_set_submit_hook(McpDispatchSubmitFn fn);
void mcp_dispatch_set_wait_hook(McpDispatchWaitFn fn);
void mcp_dispatch_set_load_source_hook(McpDispatchLoadSourceFn fn);
void mcp_dispatch_set_protocol_source(const char *source);
void mcp_dispatch_clear_hooks(void);
char *mcp_dispatch_load_protocol_source(const MCPConfig *cfg);

const char *mcp_dispatch_auth_kind_name(McpAuthKind kind);
int mcp_dispatch_worker_cap(void);
char *mcp_dispatch_build_params(const McpRpcEnvelope *env, const McpAuthResult *auth,
                                const char *session_id);
char *mcp_dispatch_submit_job(const char *script_name, const char *params_json);
ScriptingWaitResult mcp_dispatch_wait_job(const char *job_id, int timeout_seconds,
                                          char **result_json_out);
enum MHD_Result mcp_dispatch_queue_error(struct MHD_Connection *connection, const json_t *id,
                                         int code, const char *message, const char *session_id);

enum MHD_Result mcp_dispatch_submit_protocol(struct MHD_Connection *connection,
                                             const MCPConfig *cfg,
                                             const McpAuthResult *auth,
                                             const McpRpcEnvelope *env,
                                             const char *session_id);

#endif
