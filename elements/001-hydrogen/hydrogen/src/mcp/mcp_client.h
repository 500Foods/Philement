#ifndef HYDROGEN_MCP_CLIENT_H
#define HYDROGEN_MCP_CLIENT_H

#include <jansson.h>
#include <stdbool.h>
#include <stddef.h>

#define MCP_CLIENT_PROTOCOL_VERSION "2025-03-26"
#define MCP_CLIENT_MAX_BODY (16 * 1024 * 1024)

int mcp_client_next_id(void);
char *mcp_client_rpc_request(int id, const char *method, json_t *params);
char *mcp_client_unwrap_body(const char *body);
bool mcp_client_rpc_parse_result(const char *body, json_t **out_result, char **out_error);
json_t *mcp_client_mcp_schema(json_t *mcp_tool);
json_t *mcp_client_tools_map(json_t *mcp_tools, json_t *(*convert)(json_t *));

char *mcp_client_http_post(const char *url,
                           const char *authorization,
                           const char *session_id,
                           const char *body,
                           char **out_session_id);

bool mcp_client_initialize(const char *url,
                           const char *authorization,
                           char **out_session_id,
                           char **out_error);

bool mcp_client_tools_list(const char *url,
                           const char *authorization,
                           const char *session_id,
                           json_t **out_tools,
                           char **out_error);

bool mcp_client_tools_call(const char *url,
                           const char *authorization,
                           const char *session_id,
                           const char *name,
                           json_t *arguments,
                           json_t **out_result,
                           char **out_error);

bool mcp_client_tool_allowed(const char *name,
                             char **allowed_tools,
                             size_t allowed_count);

json_t *mcp_client_tools_filter(json_t *tools,
                                char **allowed_tools,
                                size_t allowed_count);

json_t *mcp_client_tool_to_openai(json_t *mcp_tool);
json_t *mcp_client_tool_to_responses(json_t *mcp_tool);
json_t *mcp_client_tool_to_anthropic(json_t *mcp_tool);
json_t *mcp_client_tools_to_openai(json_t *mcp_tools);
json_t *mcp_client_tools_to_responses(json_t *mcp_tools);
json_t *mcp_client_tools_to_anthropic(json_t *mcp_tools);

json_t *mcp_client_fetch_tools(const char *url,
                               const char *authorization,
                               char **allowed_tools,
                               size_t allowed_count,
                               const char *correlation_id);

#endif
