#ifndef HYDROGEN_MCP_H
#define HYDROGEN_MCP_H

#include <stdbool.h>

#include <src/mcp/mcp_http.h>
#include <src/mcp/mcp_stats.h>

typedef struct {
    bool enabled;
    bool initialized;
    const char *listen_interface;
    int listen_port;
    const char *listen_path;
    const char *protocol;
    bool accept_hydrogen_jwt;
    bool accept_oidc_idp;
    bool accept_oidc_rp;
    const char *resource;
    int thread_count;
    int thread_pool_size;
    McpMetrics metrics;
} McpStatusSnapshot;

void mcp_init_state(void);
void mcp_shutdown(void);
bool mcp_is_initialized(void);
bool mcp_get_status(McpStatusSnapshot *out);

#endif
