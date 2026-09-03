#ifndef HYDROGEN_LOCAL_MCP_H
#define HYDROGEN_LOCAL_MCP_H

#include <jansson.h>
#include <stdbool.h>

#include "engine_cache.h"

#define CHAT_LOCAL_MCP_MAX_ROUNDS 3

struct ChatRequestParams;
struct MultiStreamContext;
struct MultiStreamManager;
struct ChatProxyResult;

bool chat_local_mcp_config_load(json_t *collection, ChatLocalMcpConfig *out);
void chat_local_mcp_config_cleanup(ChatLocalMcpConfig *cfg);
void chat_local_mcp_server_cleanup(ChatLocalMcpServer *server);
bool chat_local_mcp_load_allowed(json_t *allowed_obj, ChatLocalMcpServer *server);
json_t *chat_local_mcp_normalize_call(const char *id, const char *name, json_t *arguments);
json_t *chat_local_mcp_finalize_accumulated(json_t *acc);
const ChatLocalMcpServer *chat_local_mcp_find_server(const ChatEngineConfig *engine, const char *name);

json_t *chat_local_mcp_list_tools(const ChatEngineConfig *engine, const char *correlation_id);
void chat_request_append_local_mcp_tools(json_t *root, json_t *mcp_tools, ChatEngineProvider provider,
                                         bool use_responses_api);

json_t *chat_local_mcp_extract_openai(json_t *root);
json_t *chat_local_mcp_extract_anthropic(json_t *root);
json_t *chat_local_mcp_extract_responses(json_t *root);
json_t *chat_local_mcp_extract_tool_calls(const char *response_body, ChatEngineProvider provider);
json_t *chat_local_mcp_extract_tool_calls_json(json_t *root, ChatEngineProvider provider);
char *chat_local_mcp_tool_result_text(json_t *mcp_result);
json_t *chat_local_mcp_proxy_tool_calls(const ChatEngineConfig *engine,
                                        json_t *tool_calls,
                                        const char *correlation_id);
void chat_local_mcp_append_openai_results(json_t *root, json_t *tool_calls, json_t *results);
void chat_local_mcp_append_anthropic_results(json_t *root, json_t *tool_calls, json_t *results);
void chat_local_mcp_append_responses_results(json_t *root, json_t *tool_calls, json_t *results);
char *chat_local_mcp_append_tool_results(const char *request_json,
                                         json_t *tool_calls,
                                         json_t *results,
                                         ChatEngineProvider provider,
                                         bool use_responses_api);
void chat_local_mcp_accumulate_stream_tool_calls(json_t **acc, json_t *delta_tool_calls);

struct ChatProxyResult *chat_local_mcp_complete_request(const ChatEngineConfig *engine,
                                                        const char *request_body,
                                                        const char *correlation_id);

char *chat_local_mcp_stream_next_body(struct MultiStreamContext *stream_ctx);

#endif
