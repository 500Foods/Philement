/*
 * MCP subsystem lifecycle.
 */

#include <src/hydrogen.h>
#include <src/mcp/mcp.h>
#include <src/mcp/mcp_auth.h>
#include <src/mcp/mcp_dispatch.h>
#include <src/mcp/mcp_session.h>
#include <src/mcp/mcp_stats.h>
#include <src/threads/threads.h>

#include <string.h>

extern ServiceThreads mcp_threads;
extern volatile sig_atomic_t mcp_system_shutdown;

bool mcp_initialized = false;

void mcp_init_state(void) {
    mcp_system_shutdown = 0;
    init_service_threads(&mcp_threads, SR_MCP);
    mcp_stats_reset();
    mcp_session_init();
    mcp_initialized = true;
}

void mcp_shutdown(void) {
    mcp_system_shutdown = 1;
    mcp_stop_listen();
    mcp_dispatch_clear_hooks();
    mcp_session_shutdown();
    mcp_initialized = false;
}

bool mcp_is_initialized(void) {
    return mcp_initialized;
}

bool mcp_get_status(McpStatusSnapshot *out) {
    if (!out) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    if (app_config) {
        const MCPConfig *cfg = &app_config->mcp;
        out->enabled = cfg->Enabled;
        out->listen_interface = cfg->Interface;
        out->listen_port = cfg->Port;
        out->listen_path = cfg->Path;
        out->protocol = cfg->Protocol;
        out->accept_hydrogen_jwt = cfg->AcceptHydrogenJWT;
        out->accept_oidc_idp = cfg->AcceptOidcIdP;
        out->accept_oidc_rp = cfg->AcceptOidcRp;
        out->resource = mcp_auth_resource(cfg);
        out->thread_pool_size = cfg->ThreadPoolSize;
    }
    out->initialized = mcp_initialized;
    update_service_thread_metrics(&mcp_threads);
    out->thread_count = mcp_threads.thread_count;
    mcp_collect_metrics(&out->metrics);
    return true;
}
