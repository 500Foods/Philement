#ifndef HYDROGEN_MCP_SESSION_H
#define HYDROGEN_MCP_SESSION_H

#include <stdbool.h>
#include <time.h>

typedef enum {
    MCP_SESSION_OK = 0,
    MCP_SESSION_CREATED,
    MCP_SESSION_UNKNOWN,
    MCP_SESSION_HIJACK,
    MCP_SESSION_LIMIT,
    MCP_SESSION_DELETED
} McpSessionResult;

typedef struct McpSessionEntry {
    char *id;
    char *sub;
    time_t last_seen;
    struct McpSessionEntry *next;
} McpSessionEntry;

void mcp_session_init(void);
void mcp_session_shutdown(void);
void mcp_session_ensure(void);
void mcp_session_set_now(time_t now);
void mcp_session_clear_now(void);
time_t mcp_session_now(void);
int mcp_session_count(void);
int mcp_session_reap(int idle_timeout_seconds);
McpSessionResult mcp_session_resolve(const char *incoming_id, const char *sub,
                                     bool allow_create, int max_sessions,
                                     int idle_timeout_seconds, char **out_id);
McpSessionResult mcp_session_delete(const char *id, const char *sub);
void mcp_session_free_entry(McpSessionEntry *entry);
McpSessionEntry *mcp_session_find_locked(const char *id);
McpSessionResult mcp_session_create_locked(const char *sub, int max_sessions, char **out_id);

#endif
