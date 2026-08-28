/*
 * MCP subsystem counters (atomics). Snapshot into ServiceMetrics.
 */

#ifndef HYDROGEN_MCP_STATS_H
#define HYDROGEN_MCP_STATS_H

#include <stdbool.h>
#include <time.h>

typedef enum {
    MCP_AUTH_REJECT_MISSING = 0,
    MCP_AUTH_REJECT_MALFORMED,
    MCP_AUTH_REJECT_HYDROGEN_JWT,
    MCP_AUTH_REJECT_OIDC_IDP,
    MCP_AUTH_REJECT_OIDC_RP,
    MCP_AUTH_REJECT_AUD,
    MCP_AUTH_REJECT_SCOPE,
    MCP_AUTH_REJECT_UNAVAILABLE,
    MCP_AUTH_REJECT_REASON_COUNT
} McpAuthRejectReason;

typedef struct {
    bool enabled;
    unsigned long long sessions_active;
    unsigned long long sessions_total;
    unsigned long long sessions_expired;
    unsigned long long rpc_received;
    unsigned long long rpc_succeeded;
    unsigned long long rpc_failed;
    unsigned long long rpc_in_flight;
    unsigned long long auth_rejected;
    unsigned long long auth_rejected_missing;
    unsigned long long auth_rejected_malformed;
    unsigned long long auth_rejected_hydrogen_jwt;
    unsigned long long auth_rejected_oidc_idp;
    unsigned long long auth_rejected_oidc_rp;
    unsigned long long auth_rejected_aud;
    unsigned long long auth_rejected_scope;
    unsigned long long origin_rejected;
    unsigned long long dispatch_timeouts;
    unsigned long long bytes_in;
    unsigned long long bytes_out;
    time_t last_rpc_at;
} McpMetrics;

void mcp_stats_reset(void);
void mcp_stats_inc_sessions_total(void);
void mcp_stats_add_sessions_active(int delta);
void mcp_stats_inc_sessions_expired(void);
void mcp_stats_inc_rpc_received(void);
void mcp_stats_inc_rpc_succeeded(void);
void mcp_stats_inc_rpc_failed(void);
void mcp_stats_add_rpc_in_flight(int delta);
unsigned long long mcp_stats_get_rpc_in_flight(void);
void mcp_stats_inc_auth_rejected(McpAuthRejectReason reason);
void mcp_stats_inc_origin_rejected(void);
void mcp_stats_inc_dispatch_timeouts(void);
void mcp_stats_add_bytes_in(unsigned long long n);
void mcp_stats_add_bytes_out(unsigned long long n);
void mcp_stats_touch_rpc(void);
void mcp_collect_metrics(McpMetrics *metrics);

#endif /* HYDROGEN_MCP_STATS_H */
