/*
 * MCP atomic counters and snapshot.
 */

#include <src/hydrogen.h>
#include <src/mcp/mcp_stats.h>

extern AppConfig *app_config;

unsigned long long mcp_stat_sessions_active = 0;
unsigned long long mcp_stat_sessions_total = 0;
unsigned long long mcp_stat_sessions_expired = 0;
unsigned long long mcp_stat_rpc_received = 0;
unsigned long long mcp_stat_rpc_succeeded = 0;
unsigned long long mcp_stat_rpc_failed = 0;
unsigned long long mcp_stat_rpc_in_flight = 0;
unsigned long long mcp_stat_auth_rejected = 0;
unsigned long long mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_REASON_COUNT];
unsigned long long mcp_stat_origin_rejected = 0;
unsigned long long mcp_stat_dispatch_timeouts = 0;
unsigned long long mcp_stat_bytes_in = 0;
unsigned long long mcp_stat_bytes_out = 0;
time_t mcp_stat_last_rpc_at = 0;

void mcp_stats_reset(void) {
    int i;

    __atomic_store_n(&mcp_stat_sessions_active, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_sessions_total, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_sessions_expired, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_rpc_received, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_rpc_succeeded, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_rpc_failed, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_rpc_in_flight, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_auth_rejected, 0, __ATOMIC_SEQ_CST);
    for (i = 0; i < MCP_AUTH_REJECT_REASON_COUNT; i++) {
        __atomic_store_n(&mcp_stat_auth_rejected_reason[i], 0, __ATOMIC_SEQ_CST);
    }
    __atomic_store_n(&mcp_stat_origin_rejected, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_dispatch_timeouts, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_bytes_in, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_bytes_out, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&mcp_stat_last_rpc_at, 0, __ATOMIC_SEQ_CST);
}

void mcp_stats_inc_sessions_total(void) {
    __atomic_fetch_add(&mcp_stat_sessions_total, 1, __ATOMIC_RELAXED);
}

void mcp_stats_add_sessions_active(int delta) {
    if (delta > 0) {
        __atomic_fetch_add(&mcp_stat_sessions_active, (unsigned long long)delta, __ATOMIC_RELAXED);
        return;
    }
    if (delta < 0) {
        unsigned long long sub = (unsigned long long)(-delta);
        unsigned long long current = __atomic_load_n(&mcp_stat_sessions_active, __ATOMIC_RELAXED);
        if (current < sub) {
            __atomic_store_n(&mcp_stat_sessions_active, 0, __ATOMIC_RELAXED);
            return;
        }
        __atomic_fetch_sub(&mcp_stat_sessions_active, sub, __ATOMIC_RELAXED);
    }
}

void mcp_stats_inc_sessions_expired(void) {
    __atomic_fetch_add(&mcp_stat_sessions_expired, 1, __ATOMIC_RELAXED);
}

void mcp_stats_inc_rpc_received(void) {
    __atomic_fetch_add(&mcp_stat_rpc_received, 1, __ATOMIC_RELAXED);
}

void mcp_stats_inc_rpc_succeeded(void) {
    __atomic_fetch_add(&mcp_stat_rpc_succeeded, 1, __ATOMIC_RELAXED);
}

void mcp_stats_inc_rpc_failed(void) {
    __atomic_fetch_add(&mcp_stat_rpc_failed, 1, __ATOMIC_RELAXED);
}

void mcp_stats_add_rpc_in_flight(int delta) {
    if (delta > 0) {
        __atomic_fetch_add(&mcp_stat_rpc_in_flight, (unsigned long long)delta, __ATOMIC_RELAXED);
        return;
    }
    if (delta < 0) {
        unsigned long long sub = (unsigned long long)(-delta);
        unsigned long long current = __atomic_load_n(&mcp_stat_rpc_in_flight, __ATOMIC_RELAXED);
        if (current < sub) {
            __atomic_store_n(&mcp_stat_rpc_in_flight, 0, __ATOMIC_RELAXED);
            return;
        }
        __atomic_fetch_sub(&mcp_stat_rpc_in_flight, sub, __ATOMIC_RELAXED);
    }
}

unsigned long long mcp_stats_get_rpc_in_flight(void) {
    return __atomic_load_n(&mcp_stat_rpc_in_flight, __ATOMIC_RELAXED);
}

void mcp_stats_inc_auth_rejected(McpAuthRejectReason reason) {
    if (reason < 0 || reason >= MCP_AUTH_REJECT_REASON_COUNT) {
        return;
    }
    __atomic_fetch_add(&mcp_stat_auth_rejected, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&mcp_stat_auth_rejected_reason[reason], 1, __ATOMIC_RELAXED);
}

void mcp_stats_inc_origin_rejected(void) {
    __atomic_fetch_add(&mcp_stat_origin_rejected, 1, __ATOMIC_RELAXED);
}

void mcp_stats_inc_dispatch_timeouts(void) {
    __atomic_fetch_add(&mcp_stat_dispatch_timeouts, 1, __ATOMIC_RELAXED);
}

void mcp_stats_add_bytes_in(unsigned long long n) {
    __atomic_fetch_add(&mcp_stat_bytes_in, n, __ATOMIC_RELAXED);
}

void mcp_stats_add_bytes_out(unsigned long long n) {
    __atomic_fetch_add(&mcp_stat_bytes_out, n, __ATOMIC_RELAXED);
}

void mcp_stats_touch_rpc(void) {
    __atomic_store_n(&mcp_stat_last_rpc_at, time(NULL), __ATOMIC_RELAXED);
}

void mcp_collect_metrics(McpMetrics *metrics) {
    if (!metrics) {
        return;
    }
    memset(metrics, 0, sizeof(*metrics));
    metrics->enabled = (app_config != NULL && app_config->mcp.Enabled);
    metrics->sessions_active = __atomic_load_n(&mcp_stat_sessions_active, __ATOMIC_RELAXED);
    metrics->sessions_total = __atomic_load_n(&mcp_stat_sessions_total, __ATOMIC_RELAXED);
    metrics->sessions_expired = __atomic_load_n(&mcp_stat_sessions_expired, __ATOMIC_RELAXED);
    metrics->rpc_received = __atomic_load_n(&mcp_stat_rpc_received, __ATOMIC_RELAXED);
    metrics->rpc_succeeded = __atomic_load_n(&mcp_stat_rpc_succeeded, __ATOMIC_RELAXED);
    metrics->rpc_failed = __atomic_load_n(&mcp_stat_rpc_failed, __ATOMIC_RELAXED);
    metrics->rpc_in_flight = __atomic_load_n(&mcp_stat_rpc_in_flight, __ATOMIC_RELAXED);
    metrics->auth_rejected = __atomic_load_n(&mcp_stat_auth_rejected, __ATOMIC_RELAXED);
    metrics->auth_rejected_missing = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_MISSING], __ATOMIC_RELAXED);
    metrics->auth_rejected_malformed = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_MALFORMED], __ATOMIC_RELAXED);
    metrics->auth_rejected_hydrogen_jwt = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_HYDROGEN_JWT], __ATOMIC_RELAXED);
    metrics->auth_rejected_oidc_idp = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_OIDC_IDP], __ATOMIC_RELAXED);
    metrics->auth_rejected_oidc_rp = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_OIDC_RP], __ATOMIC_RELAXED);
    metrics->auth_rejected_aud = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_AUD], __ATOMIC_RELAXED);
    metrics->auth_rejected_scope = __atomic_load_n(&mcp_stat_auth_rejected_reason[MCP_AUTH_REJECT_SCOPE], __ATOMIC_RELAXED);
    metrics->origin_rejected = __atomic_load_n(&mcp_stat_origin_rejected, __ATOMIC_RELAXED);
    metrics->dispatch_timeouts = __atomic_load_n(&mcp_stat_dispatch_timeouts, __ATOMIC_RELAXED);
    metrics->bytes_in = __atomic_load_n(&mcp_stat_bytes_in, __ATOMIC_RELAXED);
    metrics->bytes_out = __atomic_load_n(&mcp_stat_bytes_out, __ATOMIC_RELAXED);
    metrics->last_rpc_at = __atomic_load_n(&mcp_stat_last_rpc_at, __ATOMIC_RELAXED);
}
